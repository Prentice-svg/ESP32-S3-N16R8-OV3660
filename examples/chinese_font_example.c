/**
 * Chinese Font Integration Example
 * Shows how to integrate GB2312 font support into your ESP32-S3 OLED application
 *
 * This is a reference implementation - integrate the relevant parts into your main.c
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Include font system headers
#include "font.h"
#include "oled.h"
#include "oled_chinese.h"
#include "sdcard.h"

static const char *TAG = "app_chinese_example";

// ============================================================================
// STEP 1: Initialize font system in your startup code
// ============================================================================

void app_init_chinese_font(void)
{
    ESP_LOGI(TAG, "Initializing Chinese font system...");

    // Initialize SD card first (if not already done)
    if (sdcard_init() != ESP_OK) {
        ESP_LOGE(TAG, "SD Card initialization failed");
        return;
    }

    // Try to load GB2312 font from SD card
    // Supports: GB2312-12.fon, GB2312-16.fon, GB2312-24.fon, GB2312-32.fon
    esp_err_t ret = font_init("/font/GB2312-16.fon");

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Chinese font loaded successfully!");

        // Show a Chinese welcome message
        oled_show_chinese_message(
            "系统启动",
            "中文字库已加载",
            "ESP32-S3"
        );

        vTaskDelay(pdMS_TO_TICKS(2000));
    } else {
        ESP_LOGW(TAG, "Chinese font not found - system will use ASCII only");

        // Fallback to ASCII message
        oled_show_message(
            "System Started",
            "No Chinese font",
            "ASCII mode"
        );

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ============================================================================
// STEP 2: Display Chinese status messages
// ============================================================================

void app_show_status_chinese(const char *status, int percentage)
{
    if (!font_is_chinese_available()) {
        // Fallback to ASCII
        char buf[64];
        snprintf(buf, sizeof(buf), "Status: %s", status);
        oled_show_message(buf, NULL, NULL);
        return;
    }

    oled_clear();

    // Display title with Chinese
    oled_draw_mixed_string(0, 0, "状态: ", 16, true);
    oled_draw_mixed_string(45, 0, status, 16, true);

    // Display progress bar with Chinese label
    oled_draw_mixed_string(0, 20, "进度: ", 16, true);
    oled_draw_progress(50, 20, 78, 8, percentage);

    // Display completion percentage
    char percent_buf[8];
    snprintf(percent_buf, sizeof(percent_buf), "%d%%", percentage);
    oled_draw_mixed_string(0, 40, percent_buf, 16, true);

    oled_update();
}

// ============================================================================
// STEP 3: Display Chinese menu items
// ============================================================================

void app_show_menu_chinese(int selected_idx)
{
    if (!font_is_chinese_available()) {
        // Use ASCII menu as fallback
        const char *menu_items[] = {
            "Start Timelapse",
            "Stop Timelapse",
            "Single Capture",
            "Live Preview",
            "System Info",
            "Deep Sleep"
        };
        oled_show_menu(menu_items, 6, selected_idx);
        return;
    }

    // Chinese menu items
    const char *menu_items_cn[] = {
        "开始延时摄影",
        "停止延时摄影",
        "单张拍摄",
        "实时预览",
        "系统信息",
        "深度睡眠"
    };

    oled_clear();

    // Draw menu title
    oled_draw_mixed_string(0, 0, "菜单", 16, true);
    oled_draw_hline(0, 12, 128, true);

    // Draw menu items
    for (int i = 0; i < 6; i++) {
        int y = 16 + i * 8;
        bool is_selected = (i == selected_idx);

        if (is_selected) {
            // Highlight selected item
            oled_fill_rect(0, y - 1, 128, 8, true);
            oled_draw_mixed_string(2, y, menu_items_cn[i], 16, false);
        } else {
            oled_draw_mixed_string(2, y, menu_items_cn[i], 16, true);
        }
    }

    oled_update();
}

// ============================================================================
// STEP 4: Display system information with Chinese labels
// ============================================================================

void app_show_sysinfo_chinese(int battery_pct, bool charging,
                               bool wifi_connected, bool sd_mounted)
{
    if (!font_is_chinese_available()) {
        oled_show_system_info(battery_pct, charging, wifi_connected, sd_mounted, NULL);
        return;
    }

    oled_clear();

    // Title
    oled_draw_mixed_string(0, 0, "系统信息", 16, true);
    oled_draw_hline(0, 12, 128, true);

    // Battery status
    char bat_buf[32];
    const char *bat_status = charging ? "充电中" : "使用中";
    snprintf(bat_buf, sizeof(bat_buf), "电池: %d%% %s", battery_pct, bat_status);
    oled_draw_mixed_string(0, 16, bat_buf, 16, true);

    // WiFi status
    const char *wifi_status = wifi_connected ? "已连接" : "断开";
    oled_draw_mixed_string(0, 28, "WiFi: ", 16, true);
    oled_draw_mixed_string(45, 28, wifi_status, 16, true);

    // SD card status
    const char *sd_status = sd_mounted ? "正常" : "未装载";
    oled_draw_mixed_string(0, 40, "SD卡: ", 16, true);
    oled_draw_mixed_string(45, 40, sd_status, 16, true);

    // System status
    oled_draw_mixed_string(0, 52, "状态: 正常运行", 16, true);

    oled_update();
}

// ============================================================================
// STEP 5: Example task that updates display with Chinese text
// ============================================================================

void app_oled_update_task(void *pvParameters)
{
    int counter = 0;

    while (1) {
        // Example: Update display with Chinese text and counter
        if (font_is_chinese_available()) {
            char count_str[32];
            snprintf(count_str, sizeof(count_str), "已拍: %d张", counter);

            oled_clear();
            oled_draw_mixed_string(0, 0, "延时摄影进行中", 16, true);
            oled_draw_mixed_string(0, 20, count_str, 16, true);
            oled_draw_mixed_string(0, 40, "按键停止", 16, true);

            oled_update();
        }

        counter++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ============================================================================
// STEP 6: Integration points in main()
// ============================================================================

/*
 * Add these function calls to your main() function:
 *
 * void app_main(void) {
 *     // ... existing initialization code ...
 *
 *     // Initialize OLED
 *     if (oled_init(GPIO_NUM_2, GPIO_NUM_3, 0x3C) == ESP_OK) {
 *         // Initialize Chinese font system
 *         app_init_chinese_font();
 *     }
 *
 *     // ... rest of your application ...
 *
 *     // To display Chinese menu:
 *     app_show_menu_chinese(current_menu_index);
 *
 *     // To display Chinese status:
 *     app_show_status_chinese("初始化中", 50);
 *
 *     // To display system info:
 *     app_show_sysinfo_chinese(85, false, true, true);
 *
 *     // Create update task
 *     xTaskCreate(app_oled_update_task, "oled_update", 4096, NULL, 5, NULL);
 *
 *     // ... rest of your application ...
 * }
 */

// ============================================================================
// STEP 7: Cleanup (call during shutdown)
// ============================================================================

void app_cleanup_chinese_font(void)
{
    ESP_LOGI(TAG, "Cleaning up Chinese font system");
    font_deinit();
}

// ============================================================================
// TESTING: Simple test function
// ============================================================================

void app_test_chinese_display(void)
{
    ESP_LOGI(TAG, "Testing Chinese character display...");

    if (!font_is_chinese_available()) {
        ESP_LOGW(TAG, "Chinese font not available - skipping test");
        return;
    }

    // Test 1: Single Chinese character
    ESP_LOGI(TAG, "Test 1: Displaying single character");
    oled_clear();
    oled_draw_chinese_char(10, 10, 0xD6, 0xD0, true);  // "中"
    oled_update();
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Test 2: Chinese string
    ESP_LOGI(TAG, "Test 2: Displaying Chinese string");
    oled_clear();
    oled_draw_chinese_string(0, 0, "你好世界", true);
    oled_update();
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Test 3: Mixed text
    ESP_LOGI(TAG, "Test 3: Displaying mixed ASCII and Chinese");
    oled_clear();
    oled_draw_mixed_string(0, 0, "中文 ABC 123", 16, true);
    oled_draw_mixed_string(0, 20, "ESP32 中文显示", 16, true);
    oled_draw_mixed_string(0, 40, "状态: OK", 16, true);
    oled_update();
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "All tests completed!");
}
