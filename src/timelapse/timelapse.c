/**
 * Timelapse Engine Implementation
 * Handles photo capture scheduling and file management
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "timelapse.h"
#include "camera.h"
#include "sdcard.h"

static const char *TAG = "timelapse";

// Event bits
#define TIMELAPSE_START_BIT      (1 << 0)
#define TIMELAPSE_STOP_BIT       (1 << 1)
#define TIMELAPSE_PAUSE_BIT      (1 << 2)
#define TIMELAPSE_RESUME_BIT     (1 << 3)
#define TIMELAPSE_SHOOT_BIT      (1 << 4)

// Static variables
static timelapse_state_t current_state = TIMELAPSE_IDLE;
static timelapse_config_t config = {0};
static timelapse_status_t status = {0};
static EventGroupHandle_t event_group = NULL;
static TimerHandle_t shoot_timer = NULL;
static uint32_t shot_count = 0;
static uint64_t total_bytes = 0;
static uint64_t last_shot_time = 0;
static uint32_t sequence_number = 0;
static uint64_t start_time_epoch = 0;
static uint64_t end_time_epoch = 0;

/**
 * Convert resolution enum to framesize_t
 */
static framesize_t resolution_to_framesize(resolution_t res)
{
    switch (res) {
        case RES_CIF:      return FRAMESIZE_CIF;
        case RES_VGA:      return FRAMESIZE_VGA;
        case RES_SVGA:     return FRAMESIZE_SVGA;
        case RES_XGA:      return FRAMESIZE_XGA;
        case RES_SXGA:     return FRAMESIZE_SXGA;
        case RES_UXGA:     return FRAMESIZE_UXGA;
        case RES_QVGA:     return FRAMESIZE_QVGA;
        case RES_HD:       return FRAMESIZE_HD;
        case RES_FHD:      return FRAMESIZE_FHD;
        default:           return FRAMESIZE_UXGA;
    }
}

/**
 * Save photo to SD card
 */
static esp_err_t save_photo(void)
{
    // Switch to high resolution for capture
    framesize_t capture_size = resolution_to_framesize(config.resolution);
    camera_set_framesize(capture_size);
    
    // Small delay to let sensor stabilize after resolution change
    vTaskDelay(pdMS_TO_TICKS(100));
    
    camera_fb_t *fb = camera_capture();
    if (fb == NULL) {
        ESP_LOGE(TAG, "Failed to capture photo");
        // Switch back to lower resolution for idle
        camera_set_framesize(FRAMESIZE_SVGA);
        return ESP_FAIL;
    }

    // Generate filename with timelapse directory
    char filename[128];
    time_t now;
    time(&now);
    struct tm *tm_info = localtime(&now);

    snprintf(filename, sizeof(filename), "timelapse/%s_%04d%02d%02d_%02d%02d%02d_%08lu.jpg",
             config.filename_prefix,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
             (unsigned long)sequence_number++);

    // Save to SD card
    esp_err_t ret = sdcard_write_file(filename, fb->buf, fb->len);

    if (ret == ESP_OK) {
        shot_count++;
        total_bytes += fb->len;
        status.saved_count = shot_count;
        status.saved_bytes = total_bytes;

        ESP_LOGI(TAG, "Photo saved: %s (%d bytes)", filename, fb->len);
    } else {
        ESP_LOGE(TAG, "Failed to save photo: %s", filename);
    }

    camera_free_fb(fb);
    
    // Switch back to lower resolution for idle (reduces FB-OVF)
    camera_set_framesize(FRAMESIZE_SVGA);
    
    return ret;
}

/**
 * Shooting timer callback
 */
static void shoot_timer_callback(TimerHandle_t xTimer)
{
    xEventGroupSetBits(event_group, TIMELAPSE_SHOOT_BIT);
}

/**
 * Timer task - handles the shooting loop
 */
static void timelapse_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Timelapse task started");

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(event_group,
            TIMELAPSE_START_BIT | TIMELAPSE_STOP_BIT | TIMELAPSE_PAUSE_BIT |
            TIMELAPSE_RESUME_BIT | TIMELAPSE_SHOOT_BIT,
            pdTRUE, pdFALSE, portMAX_DELAY);

        if (bits & TIMELAPSE_START_BIT) {
            ESP_LOGI(TAG, "Starting timelapse...");
            current_state = TIMELAPSE_RUNNING;
            shot_count = 0;
            last_shot_time = esp_timer_get_time() / 1000000;
            start_time_epoch = (uint64_t)time(NULL);
            end_time_epoch = 0;

            // Configure camera quality (resolution is set in save_photo)
            camera_set_quality(config.quality);

            // Start shooting timer
            xTimerStart(shoot_timer, 0);

            // Check if we need to wait before first shot
            if (config.start_delay_sec > 0) {
                vTaskDelay(pdMS_TO_TICKS(config.start_delay_sec * 1000));
            }

            // Take first photo immediately
            if (config.start_delay_sec == 0) {
                save_photo();
            }
        }

        if (bits & TIMELAPSE_STOP_BIT) {
            ESP_LOGI(TAG, "Stopping timelapse...");
            current_state = TIMELAPSE_IDLE;
            xTimerStop(shoot_timer, 0);
            end_time_epoch = (uint64_t)time(NULL);

            // Save config to NVS
            timelapse_save_config();
        }

        if (bits & TIMELAPSE_PAUSE_BIT) {
            ESP_LOGI(TAG, "Pausing timelapse...");
            current_state = TIMELAPSE_PAUSED;
            xTimerStop(shoot_timer, 0);
        }

        if (bits & TIMELAPSE_RESUME_BIT) {
            ESP_LOGI(TAG, "Resuming timelapse...");
            current_state = TIMELAPSE_RUNNING;
            xTimerStart(shoot_timer, 0);
        }

        if (bits & TIMELAPSE_SHOOT_BIT) {
            if (current_state == TIMELAPSE_RUNNING) {
                // Check if we've reached the shot limit
                if (config.total_shots > 0 && shot_count >= config.total_shots) {
                    ESP_LOGI(TAG, "Completed %lu shots", (unsigned long)shot_count);
                    current_state = TIMELAPSE_COMPLETED;
                    end_time_epoch = (uint64_t)time(NULL);
                    timelapse_save_config();
                    continue;
                }

                // Take photo
                save_photo();

                // Update status
                status.current_shot = shot_count;
                status.next_shot_sec = config.interval_sec;
                status.elapsed_sec = (esp_timer_get_time() / 1000000) - last_shot_time;
            }
        }
    }
}

/**
 * Initialize timelapse engine
 */
void timelapse_init(void)
{
    ESP_LOGI(TAG, "Initializing timelapse engine...");

    // Load saved configuration FIRST (before creating timer)
    timelapse_load_config();

    // Ensure interval is valid (minimum 1 second)
    if (config.interval_sec == 0) {
        config.interval_sec = 60;  // Default 60 seconds
        ESP_LOGW(TAG, "Invalid interval, using default: %lu seconds", (unsigned long)config.interval_sec);
    }

    // Create timelapse directory on SD card
    if (sdcard_is_ready()) {
        esp_err_t ret = sdcard_mkdir("timelapse");
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Timelapse directory created/verified");
        }
    }

    // Create event group
    event_group = xEventGroupCreate();
    if (event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return;
    }

    // Create shooting timer with valid period from config
    uint32_t timer_period_ms = config.interval_sec * 1000;
    if (timer_period_ms == 0) {
        timer_period_ms = 60000;  // Fallback to 60 seconds
    }

    shoot_timer = xTimerCreate("shoot_timer",
                               pdMS_TO_TICKS(timer_period_ms),
                               pdTRUE,               // auto-reload enabled
                               (void *)0,
                               shoot_timer_callback);
    if (shoot_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create shoot timer");
        return;
    }

    // Create timelapse task
    xTaskCreate(timelapse_task, "timelapse", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Timelapse engine initialized (interval: %lu sec)", (unsigned long)config.interval_sec);
}

/**
 * Start timelapse shooting
 */
esp_err_t timelapse_start(void)
{
    if (current_state == TIMELAPSE_RUNNING) {
        ESP_LOGW(TAG, "Timelapse already running");
        return ESP_OK;
    }

    xEventGroupSetBits(event_group, TIMELAPSE_START_BIT);
    return ESP_OK;
}

/**
 * Stop timelapse shooting
 */
esp_err_t timelapse_stop(void)
{
    xEventGroupSetBits(event_group, TIMELAPSE_STOP_BIT);
    return ESP_OK;
}

/**
 * Pause timelapse shooting
 */
esp_err_t timelapse_pause(void)
{
    if (current_state != TIMELAPSE_RUNNING) {
        return ESP_ERR_INVALID_STATE;
    }
    xEventGroupSetBits(event_group, TIMELAPSE_PAUSE_BIT);
    return ESP_OK;
}

/**
 * Resume timelapse shooting
 */
esp_err_t timelapse_resume(void)
{
    if (current_state != TIMELAPSE_PAUSED) {
        return ESP_ERR_INVALID_STATE;
    }
    xEventGroupSetBits(event_group, TIMELAPSE_RESUME_BIT);
    return ESP_OK;
}

/**
 * Take a single photo (manual trigger)
 */
esp_err_t timelapse_take_photo(void)
{
    return save_photo();
}

/**
 * Get current configuration
 */
timelapse_config_t *timelapse_get_config(void)
{
    return &config;
}

/**
 * Update configuration
 */
esp_err_t timelapse_set_config(timelapse_config_t *new_config)
{
    memcpy(&config, new_config, sizeof(timelapse_config_t));

    // Ensure interval is valid
    if (config.interval_sec == 0) {
        config.interval_sec = 60;
    }

    // Update timer period if running
    if (current_state == TIMELAPSE_PAUSED || current_state == TIMELAPSE_RUNNING) {
        xTimerChangePeriod(shoot_timer, pdMS_TO_TICKS(config.interval_sec * 1000), 0);
    }

    return ESP_OK;
}

/**
 * Get current state
 */
timelapse_state_t timelapse_get_state(void)
{
    return current_state;
}

/**
 * Get current progress
 */
uint32_t timelapse_get_progress(void)
{
    return shot_count;
}

/**
 * Get current status
 */
void timelapse_get_status(timelapse_status_t *new_status)
{
    if (new_status == NULL) return;

    new_status->state = current_state;
    new_status->current_shot = shot_count;
    new_status->total_shots = config.total_shots;
    new_status->saved_count = shot_count;
    new_status->saved_bytes = total_bytes;
    new_status->elapsed_sec = (esp_timer_get_time() / 1000000) - last_shot_time;
    new_status->start_time_sec = start_time_epoch;
    new_status->end_time_sec = end_time_epoch;

    // Calculate time until next shot
    if (current_state == TIMELAPSE_RUNNING) {
        int64_t elapsed = (esp_timer_get_time() / 1000000) - last_shot_time;
        if (elapsed < config.interval_sec) {
            new_status->next_shot_sec = config.interval_sec - elapsed;
        } else {
            new_status->next_shot_sec = 0;
        }
    } else {
        new_status->next_shot_sec = 0;
    }

    // Get free space
    sdcard_info_t sd_info;
    sdcard_get_info(&sd_info);
    new_status->free_bytes = sd_info.free_space;
}

/**
 * Save configuration to NVS
 */
esp_err_t timelapse_save_config(void)
{
    nvs_handle_t nvs;
    esp_err_t ret = nvs_open("timelapse", NVS_READWRITE, &nvs);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_set_blob(nvs, "config", &config, sizeof(config));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save config: %s", esp_err_to_name(ret));
    } else {
        ret = nvs_commit(nvs);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Configuration saved to NVS");
        }
    }

    nvs_close(nvs);
    return ret;
}

/**
 * Load configuration from NVS
 */
esp_err_t timelapse_load_config(void)
{
    nvs_handle_t nvs;
    esp_err_t ret = nvs_open("timelapse", NVS_READONLY, &nvs);
    if (ret != ESP_OK) {
        // Use defaults if NVS doesn't exist
        config.interval_sec = 60;           // 60 seconds default
        config.total_shots = 1000;          // 1000 shots default
        config.start_delay_sec = 0;
        config.resolution = RES_UXGA;       // Full resolution with PSRAM
        config.quality = 10;                // Good quality
        config.auto_start = false;
        config.overwrite_mode = false;
        strcpy(config.filename_prefix, "TIMELAPSE");

        ESP_LOGI(TAG, "Using default configuration");
        return ESP_OK;
    }

    size_t size = sizeof(config);
    ret = nvs_get_blob(nvs, "config", &config, &size);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No saved config found, using defaults");
    } else {
        ESP_LOGI(TAG, "Configuration loaded from NVS");
    }

    nvs_close(nvs);
    return ret;
}
