/**
 * Configuration Management Implementation
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "config.h"

static const char *TAG = "config";
static system_config_t sys_config = {0};

/**
 * Load configuration from NVS
 */
esp_err_t load_config(void)
{
    nvs_handle_t nvs;
    esp_err_t ret = nvs_open("system", NVS_READONLY, &nvs);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No saved config found, using defaults");
        reset_config();
        return ESP_OK;
    }

    size_t size = sizeof(sys_config);
    ret = nvs_get_blob(nvs, "config", &sys_config, &size);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load config: %s", esp_err_to_name(ret));
        reset_config();
    } else {
        ESP_LOGI(TAG, "Configuration loaded from NVS");
        print_config();
    }

    nvs_close(nvs);
    return ret;
}

/**
 * Save configuration to NVS
 */
esp_err_t save_config(void)
{
    nvs_handle_t nvs;
    esp_err_t ret = nvs_open("system", NVS_READWRITE, &nvs);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_set_blob(nvs, "config", &sys_config, sizeof(sys_config));
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
 * Get system configuration
 */
system_config_t *get_config(void)
{
    return &sys_config;
}

/**
 * Reset configuration to defaults
 */
void reset_config(void)
{
    memset(&sys_config, 0, sizeof(sys_config));

    // Default system settings
    strcpy(sys_config.device_name, "TimelapseCam");
    sys_config.auto_start_delay = 0;

    // Default camera settings
    sys_config.brightness = 0;
    sys_config.contrast = 0;
    sys_config.saturation = 0;
    sys_config.vflip = false;
    sys_config.hmirror = false;

    // Default WiFi settings
    sys_config.wifi_enabled = true;
    strcpy(sys_config.wifi_ssid, "TimelapseCam");
    strcpy(sys_config.wifi_password, "12345678");
    sys_config.ap_mode = true;

    // Default power settings
    sys_config.battery_monitoring = false;
    sys_config.low_battery_threshold = 3.3f;

    // Default display settings
    sys_config.display_rotation = 0;
    sys_config.display_on = true;
    sys_config.display_timeout = 30;

    ESP_LOGI(TAG, "Configuration reset to defaults");
}

/**
 * Print current configuration
 */
void print_config(void)
{
    ESP_LOGI(TAG, "=== System Configuration ===");
    ESP_LOGI(TAG, "Device Name: %s", sys_config.device_name);
    ESP_LOGI(TAG, "WiFi Enabled: %s", sys_config.wifi_enabled ? "Yes" : "No");
    ESP_LOGI(TAG, "WiFi Mode: %s", sys_config.ap_mode ? "AP" : "Station");
    ESP_LOGI(TAG, "Battery Monitoring: %s", sys_config.battery_monitoring ? "Yes" : "No");
    ESP_LOGI(TAG, "Display Timeout: %d seconds", sys_config.display_timeout);
    ESP_LOGI(TAG, "Camera - Brightness: %d, Contrast: %d, Saturation: %d",
             sys_config.brightness, sys_config.contrast, sys_config.saturation);
}
