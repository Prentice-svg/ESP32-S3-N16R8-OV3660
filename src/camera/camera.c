/**
 * Camera Driver Implementation
 * Uses ESP-IDF ESP32 Camera Driver
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_camera.h"
#include "camera.h"

static const char *TAG = "camera";
static bool is_init = false;
static camera_config_t current_config;
static framesize_t current_framesize = FRAMESIZE_UXGA;

esp_err_t camera_init(const camera_config_t *config)
{
    if (is_init) {
        ESP_LOGW(TAG, "Camera already initialized");
        return ESP_OK;
    }

    memcpy(&current_config, config, sizeof(camera_config_t));

    ESP_LOGI(TAG, "Initializing camera with pin config:");
    ESP_LOGI(TAG, "  XCLK: %d, PCLK: %d", config->pin_xclk, config->pin_pclk);
    ESP_LOGI(TAG, "  VSYNC: %d, HREF: %d", config->pin_vsync, config->pin_href);
    ESP_LOGI(TAG, "  D0-D7: %d, %d, %d, %d, %d, %d, %d, %d",
             config->pin_d0, config->pin_d1, config->pin_d2, config->pin_d3,
             config->pin_d4, config->pin_d5, config->pin_d6, config->pin_d7);
    ESP_LOGI(TAG, "  SIOC: %d, SIOD: %d", config->pin_sccb_scl, config->pin_sccb_sda);

    // Initialize camera driver (handles XCLK/LEDC internally)
    esp_err_t ret = esp_camera_init(config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Get sensor handle and configure
    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor == NULL) {
        ESP_LOGE(TAG, "Failed to get sensor handle");
        return ESP_ERR_NOT_FOUND;
    }

    // Log detected sensor info for debugging
    ESP_LOGI(TAG, "Camera initialized successfully");
    ESP_LOGI(TAG, "Sensor PID: 0x%02X, VER: 0x%02X", sensor->id.PID, sensor->id.VER);

    // Set sensor settings (matching Freenove reference code)
    sensor->set_vflip(sensor, 1);      // Flip vertically
    sensor->set_brightness(sensor, 1); // Slightly increase brightness
    sensor->set_saturation(sensor, 0); // Default saturation

    // Enable automatic exposure/AE with mild boost for dark scenes
    sensor->set_exposure_ctrl(sensor, 1);
    sensor->set_gain_ctrl(sensor, 1);
    sensor->set_aec2(sensor, 1);       // Secondary AEC for smoother response
    sensor->set_ae_level(sensor, 1);   // Range -2..2, 0=default

    current_framesize = config->frame_size;
    is_init = true;

    // Warm up camera - discard first few frames to avoid NO-SOI errors
    ESP_LOGI(TAG, "Warming up camera...");
    for (int i = 0; i < 5; i++) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) {
            esp_camera_fb_return(fb);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGI(TAG, "Camera warm-up complete");

    return ESP_OK;
}

esp_err_t camera_deinit(void)
{
    if (!is_init) {
        return ESP_OK;
    }

    esp_err_t ret = esp_camera_deinit();
    if (ret == ESP_OK) {
        is_init = false;
        ESP_LOGI(TAG, "Camera deinitialized");
    }
    return ret;
}

camera_fb_t *camera_capture(void)
{
    if (!is_init) {
        ESP_LOGE(TAG, "Camera not initialized");
        return NULL;
    }

    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == NULL) {
        ESP_LOGE(TAG, "Failed to capture frame");
        return NULL;
    }

    return fb;
}

void camera_free_fb(camera_fb_t *fb)
{
    if (fb == NULL) return;
    esp_camera_fb_return(fb);
}

camera_fb_t *camera_get_preview(void)
{
    // Lower resolution for preview
    framesize_t original = current_framesize;
    camera_set_framesize(FRAMESIZE_QVGA);  // 320x240

    camera_fb_t *preview = camera_capture();

    // Restore original resolution
    camera_set_framesize(original);

    return preview;
}

bool camera_is_ready(void)
{
    return is_init;
}

esp_err_t camera_set_framesize(framesize_t size)
{
    if (!is_init) return ESP_ERR_INVALID_STATE;

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor == NULL) return ESP_ERR_NOT_FOUND;

    esp_err_t ret = sensor->set_framesize(sensor, size);
    if (ret == ESP_OK) {
        current_framesize = size;
        ESP_LOGI(TAG, "Frame size set to: %d", size);
    }
    return ret;
}

esp_err_t camera_set_quality(uint8_t quality)
{
    if (!is_init) return ESP_ERR_INVALID_STATE;

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor == NULL) return ESP_ERR_NOT_FOUND;

    return sensor->set_quality(sensor, quality);
}

sensor_t *camera_get_sensor(void)
{
    if (!is_init) return NULL;
    return esp_camera_sensor_get();
}
