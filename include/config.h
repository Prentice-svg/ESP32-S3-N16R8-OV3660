/**
 * Configuration Management Header
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * General configuration structure
 */
typedef struct {
    // System settings
    char device_name[32];          // Device name for WiFi AP
    uint8_t auto_start_delay;      // Auto-start delay in seconds

    // Camera settings
    uint8_t brightness;            // Brightness (-2 to 2)
    uint8_t contrast;              // Contrast (-2 to 2)
    uint8_t saturation;            // Saturation (-2 to 2)
    bool vflip;                    // Vertical flip
    bool hmirror;                  // Horizontal mirror

    // WiFi settings
    char wifi_ssid[32];            // WiFi SSID
    char wifi_password[64];        // WiFi password
    bool wifi_enabled;             // WiFi enabled
    bool ap_mode;                  // Access point mode (vs station)

    // Power settings
    bool battery_monitoring;       // Enable battery monitoring
    float low_battery_threshold;   // Low battery threshold (volts)

    // Display settings
    uint8_t display_rotation;      // Display rotation (0, 90, 180, 270)
    bool display_on;               // Keep display on
    uint16_t display_timeout;      // Display timeout in seconds
} system_config_t;

/**
 * Load configuration from NVS
 */
esp_err_t load_config(void);

/**
 * Save configuration to NVS
 */
esp_err_t save_config(void);

/**
 * Get system configuration
 */
system_config_t *get_config(void);

/**
 * Reset configuration to defaults
 */
void reset_config(void);

/**
 * Print current configuration
 */
void print_config(void);

#ifdef __cplusplus
}
#endif

#endif // __CONFIG_H
