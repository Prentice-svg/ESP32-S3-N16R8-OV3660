/**
 * Power Management Implementation
 * Handles battery monitoring and power source detection
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "power.h"
#include "camera_pins.h"

static const char *TAG = "power";

#define ADC_CHANNEL         ADC1_CHANNEL_0  // GPIO1
#define ADC_WIDTH           ADC_WIDTH_BIT_12
#define ADC_ATTEN           ADC_ATTEN_DB_11

#define VOLTAGE_DIVIDER     2.0f            // Voltage divider ratio
#define MAX_VOLTAGE         4.2f            // LiPo fully charged
#define MIN_VOLTAGE         3.0f            // LiPo cutoff
#define CRITICAL_VOLTAGE    3.3f            // Low battery warning

static bool is_init = false;
static esp_adc_cal_characteristics_t adc_chars;
static battery_status_t battery_status = {0};

// Forward declaration
static void update_status(void);

/**
 * Initialize power management
 */
esp_err_t power_init(void)
{
    if (is_init) return ESP_OK;

    ESP_LOGI(TAG, "Initializing power management...");

    // Configure ADC for battery monitoring
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);

    // Characterize ADC
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, 1100, &adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGI(TAG, "ADC calibrated using eFuse Two Point values");
    } else {
        ESP_LOGI(TAG, "ADC using default calibration values");
    }

    is_init = true;
    update_status();

    ESP_LOGI(TAG, "Power management initialized");
    return ESP_OK;
}

/**
 * Deinitialize power management
 */
esp_err_t power_deinit(void)
{
    is_init = false;
    return ESP_OK;
}

/**
 * Update battery status
 */
static void update_status(void)
{
    if (!is_init) return;

    // Read ADC
    uint32_t adc_reading = 0;
    for (int i = 0; i < 10; i++) {
        adc_reading += adc1_get_raw(ADC_CHANNEL);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    adc_reading /= 10;

    // Convert to voltage
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
    float battery_voltage = (voltage / 1000.0f) * VOLTAGE_DIVIDER;

    battery_status.voltage = battery_voltage;

    // Calculate percentage
    if (battery_voltage >= MAX_VOLTAGE) {
        battery_status.percentage = 100.0f;
    } else if (battery_voltage <= MIN_VOLTAGE) {
        battery_status.percentage = 0.0f;
    } else {
        battery_status.percentage = ((battery_voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * 100.0f;
    }

    // Check USB connection (GPIO 19 typically used for VBUS detection)
    battery_status.usb_connected = gpio_get_level(GPIO_NUM_19) == 1;

    // Determine power source
    if (battery_status.usb_connected) {
        battery_status.source = POWER_SOURCE_USB;
        battery_status.charging = (battery_voltage < MAX_VOLTAGE);
    } else {
        battery_status.source = POWER_SOURCE_BATTERY;
        battery_status.charging = false;
    }
}

/**
 * Get current power source
 */
power_source_t power_get_source(void)
{
    update_status();
    return battery_status.source;
}

/**
 * Get battery status
 */
void power_get_battery_status(battery_status_t *status)
{
    if (status == NULL) return;

    update_status();
    memcpy(status, &battery_status, sizeof(battery_status_t));
}

/**
 * Check if USB is connected
 */
bool power_usb_connected(void)
{
    return battery_status.usb_connected;
}

/**
 * Check if battery is low
 */
bool power_is_low_battery(void)
{
    update_status();
    return battery_status.voltage < CRITICAL_VOLTAGE;
}

/**
 * Get battery voltage
 */
float power_get_voltage(void)
{
    update_status();
    return battery_status.voltage;
}

/**
 * Enter deep sleep mode
 */
void power_deep_sleep(uint32_t seconds)
{
    ESP_LOGI(TAG, "Entering deep sleep for %lu seconds", (unsigned long)seconds);

    // Configure wakeup sources
    if (seconds > 0) {
        esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
    }

    // Enable wake on GPIO (BOOT button)
    esp_sleep_enable_ext1_wakeup(GPIO_NUM_0, ESP_EXT1_WAKEUP_ANY_LOW);

    // Enter deep sleep
    esp_deep_sleep_start();
}

/**
 * Get estimated battery life in seconds
 */
uint32_t power_get_estimated_life(void)
{
    update_status();

    // Estimate based on current draw and battery capacity
    // Assuming 3000mAh battery and 100mA average current
    float capacity_ah = 3.0f;  // 3000mAh
    float current_a = 0.1f;    // 100mA average
    float hours_remaining = (battery_status.percentage / 100.0f) * (capacity_ah / current_a);

    return (uint32_t)(hours_remaining * 3600);
}
