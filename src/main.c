/**
 * ESP32-S3 Timelapse Camera - Main Application
 *
 * Hardware: ESP32-S3-N16R8-OV3660 Development Board
 * Features:
 *   - OV3660 3MP Camera
 *   - 64GB SD Card storage
 *   - SPI LCD Display (optional)
 *   - WiFi Remote Control (AP or Station mode)
 *   - Battery monitoring
 *   - Deep sleep for power saving
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_camera.h"
#include "driver/i2c.h"
#include "esp_timer.h"

#include "camera.h"
#include "sdcard.h"
#include "timelapse.h"
#include "config.h"
#include "wifi.h"
#include "webserver.h"
#include "power.h"
#include "lcd.h"
#include "oled.h"
#include "oled_chinese.h"
#include "font.h"
#include "camera_pins.h"

static const char *TAG = "main";

// Event bits for button handling
#define BTN_BOOT_PRESSED     (1 << 0)
#define BTN_KEY1_PRESSED     (1 << 1)
#define BTN_KEY2_PRESSED     (1 << 2)
#define BTN_KEY3_PRESSED     (1 << 3)
#define BTN_KEY4_PRESSED     (1 << 4)

static EventGroupHandle_t btn_event_group = NULL;
static bool lcd_init_success = false;
static bool oled_init_success = false;

// Menu system
typedef enum {
    SCREEN_STATUS = 0,
    SCREEN_MENU,
    SCREEN_SETTINGS,
    SCREEN_INFO,
    SCREEN_PREVIEW
} screen_mode_t;

static screen_mode_t current_screen = SCREEN_STATUS;
static int menu_selection = 0;
static const char *menu_items[] = {
    "\xbf\xaa\xca\xbc\xd1\xd3\xca\xb1",   // 开始延时 (Start Timelapse)
    "\xcd\xa3\xd6\xb9\xd1\xd3\xca\xb1",   // 停止延时 (Stop Timelapse)
    "\xb5\xa5\xd5\xc5\xc5\xc4\xc9\xe3",   // 单张拍摄 (Single Capture)
    "\xca\xb5\xca\xb1\xd4\xa4\xc0\xc0",   // 实时预览 (Live Preview)
    "\xcf\xb5\xcd\xb3\xd0\xc5\xcf\xa2",   // 系统信息 (System Info)
    "\xc9\xee\xb6\xc8\xcb\xaf\xc3\xdf"    // 深度睡眠 (Deep Sleep)
};
#define MENU_ITEM_COUNT 6

/**
 * Button interrupt handler for BOOT button
 */
static void btn_boot_isr_handler(void *arg)
{
    xEventGroupSetBitsFromISR(btn_event_group, BTN_BOOT_PRESSED, NULL);
}

/**
 * Button interrupt handler for K1-K4
 */
static void btn_key1_isr_handler(void *arg)
{
    xEventGroupSetBitsFromISR(btn_event_group, BTN_KEY1_PRESSED, NULL);
}

static void btn_key2_isr_handler(void *arg)
{
    xEventGroupSetBitsFromISR(btn_event_group, BTN_KEY2_PRESSED, NULL);
}

static void btn_key3_isr_handler(void *arg)
{
    xEventGroupSetBitsFromISR(btn_event_group, BTN_KEY3_PRESSED, NULL);
}

static void btn_key4_isr_handler(void *arg)
{
    xEventGroupSetBitsFromISR(btn_event_group, BTN_KEY4_PRESSED, NULL);
}

/**
 * Initialize GPIO buttons
 */
static void buttons_init(void)
{
    // Create event group first
    btn_event_group = xEventGroupCreate();
    
    // Install ISR service
    gpio_install_isr_service(0);
    
    // BOOT button on GPIO0
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOOT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf);
    gpio_isr_handler_add(BOOT_PIN, btn_boot_isr_handler, NULL);
    
    // K1-K4 buttons (Active LOW with internal pull-up)
    gpio_config_t key_conf = {
        .pin_bit_mask = (1ULL << KEY1_PIN) | (1ULL << KEY2_PIN) | 
                        (1ULL << KEY3_PIN) | (1ULL << KEY4_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&key_conf);
    
    gpio_isr_handler_add(KEY1_PIN, btn_key1_isr_handler, NULL);
    gpio_isr_handler_add(KEY2_PIN, btn_key2_isr_handler, NULL);
    gpio_isr_handler_add(KEY3_PIN, btn_key3_isr_handler, NULL);
    gpio_isr_handler_add(KEY4_PIN, btn_key4_isr_handler, NULL);
    
    ESP_LOGI(TAG, "Buttons initialized (BOOT=%d, K1=%d, K2=%d, K3=%d, K4=%d)",
             BOOT_PIN, KEY1_PIN, KEY2_PIN, KEY3_PIN, KEY4_PIN);
}

/**
 * Update OLED display based on current screen
 */
static void update_oled_display(void)
{
    if (!oled_init_success) return;
    
    timelapse_status_t tl_status;
    timelapse_get_status(&tl_status);
    
    battery_status_t bat_status;
    power_get_battery_status(&bat_status);
    
    sdcard_info_t sd_info;
    sdcard_get_info(&sd_info);
    
    timelapse_config_t *tl_config = timelapse_get_config();
    
    switch (current_screen) {
        case SCREEN_STATUS:
            oled_show_timelapse_status(
                tl_status.state == TIMELAPSE_RUNNING,
                tl_status.current_shot,
                tl_status.total_shots,
                tl_config ? tl_config->interval_sec : 0,
                tl_status.next_shot_sec
            );
            break;
            
        case SCREEN_MENU:
            oled_show_menu(menu_items, MENU_ITEM_COUNT, menu_selection);
            break;
            
        case SCREEN_INFO:
            oled_show_system_info(
                (int)bat_status.percentage,
                bat_status.charging,
                wifi_is_connected(),
                sd_info.initialized,
                wifi_get_ip_address()
            );
            break;
            
        case SCREEN_PREVIEW:
            {
                // Capture and display preview
                camera_fb_t *fb = esp_camera_fb_get();
                if (fb) {
                    oled_show_preview(fb->buf, fb->len);
                    esp_camera_fb_return(fb);
                } else {
                    oled_show_message("Preview", "Failed!", "Press any key");
                }
            }
            break;
            
        default:
            break;
    }
}

/**
 * Execute menu action
 */
static void execute_menu_action(int selection)
{
    switch (selection) {
        case 0:  // Start Timelapse
            if (timelapse_get_state() != TIMELAPSE_RUNNING) {
                timelapse_start();
                ESP_LOGI(TAG, "Timelapse started via menu");
                if (oled_init_success) {
                    oled_show_message("\xd1\xd3\xca\xb1\xc9\xe3\xd3\xb0", "\xd2\xd1\xc6\xf4\xb6\xaf", NULL);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }
            current_screen = SCREEN_STATUS;
            break;

        case 1:  // Stop Timelapse
            if (timelapse_get_state() == TIMELAPSE_RUNNING) {
                timelapse_stop();
                ESP_LOGI(TAG, "Timelapse stopped via menu");
                if (oled_init_success) {
                    oled_show_message("\xd1\xd3\xca\xb1\xc9\xe3\xd3\xb0", "\xd2\xd1\xcd\xa3\xd6\xb9", NULL);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }
            current_screen = SCREEN_STATUS;
            break;

        case 2:  // Single Capture
            {
                if (oled_init_success) {
                    oled_show_message("\xc5\xc4\xc9\xe3\xd6\xd0", NULL, NULL);
                }
                camera_fb_t *fb = esp_camera_fb_get();
                if (fb) {
                    char filename[64];
                    snprintf(filename, sizeof(filename), "/sdcard/capture_%lu.jpg",
                             (unsigned long)(esp_timer_get_time() / 1000000));
                    sdcard_write_file(filename, fb->buf, fb->len);

                    // Show preview on OLED before returning frame buffer
                    if (oled_init_success) {
                        oled_show_preview(fb->buf, fb->len);
                        vTaskDelay(pdMS_TO_TICKS(2000));  // Show preview for 2 seconds
                    }

                    esp_camera_fb_return(fb);
                    ESP_LOGI(TAG, "Single capture saved: %s", filename);
                }
            }
            current_screen = SCREEN_STATUS;
            break;
            
        case 3:  // Live Preview
            current_screen = SCREEN_PREVIEW;
            ESP_LOGI(TAG, "Entering preview mode");
            break;
            
        case 4:  // System Info
            current_screen = SCREEN_INFO;
            break;
            
        case 5:  // Deep Sleep
            ESP_LOGI(TAG, "Entering deep sleep via menu...");
            if (oled_init_success) {
                oled_show_message("\xc9\xee\xb6\xc8\xcb\xaf\xc3\xdf", "\xb0\xb4\x42\x4f\x4f\x54\xbb\xbd\xd0\xd1", NULL);
                vTaskDelay(pdMS_TO_TICKS(2000));
                oled_display_on(false);
            }
            if (lcd_init_success) {
                lcd_deinit();
            }
            power_deep_sleep(0);
            break;
    }
}

/**
 * Handle button press actions
 */
static void button_task(void *pvParameters)
{
    while (1) {
        EventBits_t bits = xEventGroupWaitBits(btn_event_group,
            BTN_BOOT_PRESSED | BTN_KEY1_PRESSED | BTN_KEY2_PRESSED | 
            BTN_KEY3_PRESSED | BTN_KEY4_PRESSED,
            pdTRUE, pdFALSE, portMAX_DELAY);

        // Debounce delay
        vTaskDelay(pdMS_TO_TICKS(50));
        
        // BOOT button - toggle timelapse (legacy behavior)
        if (bits & BTN_BOOT_PRESSED) {
            if (gpio_get_level(BOOT_PIN) == 0) {
                // Short press: toggle timelapse
                if (timelapse_get_state() == TIMELAPSE_RUNNING) {
                    timelapse_stop();
                    ESP_LOGI(TAG, "Timelapse stopped via BOOT button");
                } else {
                    timelapse_start();
                    ESP_LOGI(TAG, "Timelapse started via BOOT button");
                }
                update_oled_display();

                // Long press detection (>3s): deep sleep
                vTaskDelay(pdMS_TO_TICKS(100));
                if (gpio_get_level(BOOT_PIN) == 0) {
                    int hold_time = 0;
                    while (gpio_get_level(BOOT_PIN) == 0 && hold_time < 3000) {
                        vTaskDelay(pdMS_TO_TICKS(100));
                        hold_time += 100;
                    }
                    if (hold_time >= 3000) {
                        ESP_LOGI(TAG, "Entering deep sleep...");
                        if (oled_init_success) {
                            oled_show_message("\xc9\xee\xb6\xc8\xcb\xaf\xc3\xdf\x2e\x2e\x2e", NULL, NULL);
                            vTaskDelay(pdMS_TO_TICKS(500));
                            oled_display_on(false);
                        }
                        if (lcd_init_success) {
                            lcd_deinit();
                        }
                        power_deep_sleep(0);
                    }
                }
            }
        }
        
        // K1 - Menu/Up navigation
        if (bits & BTN_KEY1_PRESSED) {
            if (gpio_get_level(KEY1_PIN) == 0) {
                ESP_LOGI(TAG, "K1 pressed");
                if (current_screen == SCREEN_STATUS || current_screen == SCREEN_INFO) {
                    current_screen = SCREEN_MENU;
                    menu_selection = 0;
                } else if (current_screen == SCREEN_MENU) {
                    // Navigate up in menu
                    if (menu_selection > 0) {
                        menu_selection--;
                    } else {
                        menu_selection = MENU_ITEM_COUNT - 1;
                    }
                }
                update_oled_display();
            }
        }
        
        // K2 - Down navigation
        if (bits & BTN_KEY2_PRESSED) {
            if (gpio_get_level(KEY2_PIN) == 0) {
                ESP_LOGI(TAG, "K2 pressed (Down)");
                if (current_screen == SCREEN_PREVIEW) {
                    // Exit preview mode
                    current_screen = SCREEN_STATUS;
                } else if (current_screen == SCREEN_MENU) {
                    // Navigate down in menu
                    if (menu_selection < MENU_ITEM_COUNT - 1) {
                        menu_selection++;
                    } else {
                        menu_selection = 0;
                    }
                } else if (current_screen == SCREEN_STATUS) {
                    // Open menu
                    current_screen = SCREEN_MENU;
                    menu_selection = 0;
                } else if (current_screen == SCREEN_INFO) {
                    current_screen = SCREEN_STATUS;
                }
                update_oled_display();
            }
        }
        
        // K3 - Back/Cancel
        if (bits & BTN_KEY3_PRESSED) {
            if (gpio_get_level(KEY3_PIN) == 0) {
                ESP_LOGI(TAG, "K3 pressed (Back)");
                if (current_screen == SCREEN_PREVIEW) {
                    current_screen = SCREEN_STATUS;
                } else if (current_screen == SCREEN_MENU) {
                    // Back to status
                    current_screen = SCREEN_STATUS;
                } else if (current_screen == SCREEN_INFO) {
                    current_screen = SCREEN_STATUS;
                } else if (current_screen == SCREEN_STATUS) {
                    // Quick stop timelapse
                    if (timelapse_get_state() == TIMELAPSE_RUNNING) {
                        timelapse_stop();
                        ESP_LOGI(TAG, "Timelapse stopped via K3");
                    }
                }
                update_oled_display();
            }
        }
        
        // K4 - OK/Confirm/Select
        if (bits & BTN_KEY4_PRESSED) {
            if (gpio_get_level(KEY4_PIN) == 0) {
                ESP_LOGI(TAG, "K4 pressed (OK)");
                if (current_screen == SCREEN_PREVIEW) {
                    // Exit preview mode
                    current_screen = SCREEN_STATUS;
                } else if (current_screen == SCREEN_MENU) {
                    // Execute selected menu item
                    execute_menu_action(menu_selection);
                } else if (current_screen == SCREEN_INFO) {
                    current_screen = SCREEN_STATUS;
                } else if (current_screen == SCREEN_STATUS) {
                    // Quick start timelapse
                    if (timelapse_get_state() != TIMELAPSE_RUNNING) {
                        timelapse_start();
                        ESP_LOGI(TAG, "Timelapse started via K4");
                    }
                }
                update_oled_display();
            }
        }
    }
}

/**
 * Print system status
 */
static void print_status(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-S3 Timelapse Camera System Info:");
    ESP_LOGI(TAG, "  Chip: ESP32-S3");
    ESP_LOGI(TAG, "  Free Heap: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "  Total Heap: %lu bytes", esp_get_free_heap_size());

    timelapse_status_t tl_status;
    timelapse_get_status(&tl_status);
    ESP_LOGI(TAG, "  Timelapse: state=%d, shots=%lu/%lu",
             tl_status.state, (unsigned long)tl_status.current_shot, (unsigned long)tl_status.total_shots);

    battery_status_t bat_status;
    power_get_battery_status(&bat_status);
    ESP_LOGI(TAG, "  Battery: %.2fV (%.1f%%), USB=%s, Charging=%s",
             bat_status.voltage, bat_status.percentage,
             bat_status.usb_connected ? "Yes" : "No",
             bat_status.charging ? "Yes" : "No");

    sdcard_info_t sd_info;
    sdcard_get_info(&sd_info);
    ESP_LOGI(TAG, "  SD Card: Free=%llu MB",
             sd_info.free_space / (1024*1024));

    ESP_LOGI(TAG, "  WiFi: %s, IP=%s",
             wifi_is_connected() ? "Connected" : "Disconnected",
             wifi_get_ip_address());

    ESP_LOGI(TAG, "========================================");
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-S3 Timelapse Camera v1.0");
    ESP_LOGI(TAG, "========================================");

    // Print memory info
    ESP_LOGI(TAG, "Initial memory: %lu bytes", esp_get_free_heap_size());

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize power management
    power_init();

    // Initialize buttons
    buttons_init();

    // Initialize SD Card
    ret = sdcard_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD Card init failed: %s", esp_err_to_name(ret));
    } else {
        sdcard_info_t info;
        sdcard_get_info(&info);
        ESP_LOGI(TAG, "SD Card: %s, Size: %llu MB, Free: %llu MB",
                 info.card_name, info.card_size / (1024*1024),
                 info.free_space / (1024*1024));
    }

    // Initialize Camera
    // Note: Without PSRAM, use smaller frame size and single frame buffer
    camera_config_t cam_config = {
        .pin_pwdn = -1,
        .pin_reset = -1,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,
        .xclk_freq_hz = 10000000,           // 10MHz - slower for Octal PSRAM compatibility
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_SVGA,       // 800x600 default, switch to UXGA when capturing
        .jpeg_quality = 10,                 // Better quality with PSRAM available
        .fb_count = 2,                      // Double buffer with PSRAM
        .fb_location = CAMERA_FB_IN_PSRAM,  // Use PSRAM for frame buffers
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY, // Stop capture when buffer full (prevents FB-OVF)
    };

    ret = camera_init(&cam_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Camera initialized (Sensor ID: 0x%02X)",
                 camera_get_sensor()->id.PID);
    }

    // Initialize LCD (optional - won't fail if not present)
#if 0  // Set to 1 if LCD is connected
    ret = lcd_init(LCD_PIN_CS, LCD_PIN_DC, LCD_PIN_RST,
                   LCD_PIN_SCK, LCD_PIN_MOSI);
    if (ret == ESP_OK) {
        lcd_init_success = true;
        lcd_clear(0x0000);
        lcd_draw_text(10, 10, "Timelapse Cam", 0xFFFF, 0x0000);
        ESP_LOGI(TAG, "LCD initialized");
    } else {
        ESP_LOGW(TAG, "LCD not found or init failed");
    }
#endif

    // Initialize OLED display (0.96" I2C SSD1306/SSD1315)
    ret = oled_init(OLED_SDA_PIN, OLED_SCL_PIN, OLED_I2C_ADDR);
    if (ret == ESP_OK) {
        oled_init_success = true;
        oled_show_message("\xd1\xd3\xca\xb1\xcf\xe0\xbb\xfa", "\xb3\xf5\xca\xbc\xbb\xaf\xd6\xd0", NULL);
        ESP_LOGI(TAG, "OLED display initialized");

        // Initialize Chinese font for OLED menu
        ret = font_init("/font/GB2312-16.fon");
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Chinese font loaded successfully");
        } else {
            ESP_LOGW(TAG, "Chinese font not available: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGW(TAG, "OLED not found (SDA=%d, SCL=%d)", OLED_SDA_PIN, OLED_SCL_PIN);
    }

    // Load configuration
    load_config();
    system_config_t *cfg = get_config();

    // Initialize WiFi (if enabled)
    if (cfg->wifi_enabled) {
        wifi_init(cfg->ap_mode ? WIFI_MODE_AP : WIFI_MODE_STA,
                  cfg->wifi_ssid, cfg->wifi_password);

        // Initialize and start web server
        webserver_init(80);
        webserver_start();
        ESP_LOGI(TAG, "Web server started at http://%s", wifi_get_ip_address());
    }

    // Initialize timelapse engine
    timelapse_init();

    // Start button task
    xTaskCreate(button_task, "button", 4096, NULL, 10, NULL);

    // Print status
    print_status();

    // Show ready screen on OLED
    if (oled_init_success) {
        oled_show_message("\xbe\xcd\xd0\xf7",
                          wifi_is_connected() ? wifi_get_ip_address() : "\x57\x69\x46\x69\xb9\xd8\xb1\xd5",
                          "\x4b\x31\xb2\xcb\xb5\xa5\x20\x4b\x32\xbf\xaa\xca\xbc");
        vTaskDelay(pdMS_TO_TICKS(2000));
        update_oled_display();
    }

    ESP_LOGI(TAG, "Timelapse camera ready!");
    ESP_LOGI(TAG, "Press BOOT button: short=start/stop, long=deep sleep");
    ESP_LOGI(TAG, "Keys: K1=Menu, K2=Start/OK, K3=Stop/Down, K4=Info/Back");

    // Main loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Update every second

        // Update OLED display periodically (status screen only)
        if (oled_init_success && current_screen == SCREEN_STATUS) {
            update_oled_display();
        }

        // Periodic status log (every 5 seconds)
        static int log_counter = 0;
        log_counter++;
        if (log_counter >= 5 && timelapse_get_state() == TIMELAPSE_RUNNING) {
            log_counter = 0;
            timelapse_status_t status;
            timelapse_get_status(&status);

            ESP_LOGI(TAG, "Progress: %lu/%lu shots, next in %lus",
                     (unsigned long)status.current_shot, (unsigned long)status.total_shots,
                     (unsigned long)status.next_shot_sec);

            // Check low battery
            if (power_is_low_battery() && !power_usb_connected()) {
                ESP_LOGW(TAG, "Low battery! Stopping timelapse...");
                timelapse_stop();
                if (oled_init_success) {
                    oled_show_message("Low Battery!", "Timelapse", "Stopped");
                }
            }
        }
    }
}
