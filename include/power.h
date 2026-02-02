/**
 * Power Management Header
 */

#ifndef __POWER_H
#define __POWER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Power source type
 */
typedef enum {
    POWER_SOURCE_USB,      // USB power
    POWER_SOURCE_BATTERY,  // Battery power
    POWER_SOURCE_UNKNOWN
} power_source_t;

/**
 * Battery status
 */
typedef struct {
    float voltage;              // Battery voltage
    float percentage;           // Battery percentage (0-100)
    bool charging;              // Is charging
    bool usb_connected;         // USB connected
    power_source_t source;      // Current power source
} battery_status_t;

/**
 * Initialize power management
 */
esp_err_t power_init(void);

/**
 * Deinitialize power management
 */
esp_err_t power_deinit(void);

/**
 * Get current power source
 * @return Current power source
 */
power_source_t power_get_source(void);

/**
 * Get battery status
 * @param status Pointer to store status
 */
void power_get_battery_status(battery_status_t *status);

/**
 * Check if USB is connected
 * @return true if USB connected
 */
bool power_usb_connected(void);

/**
 * Check if battery is low
 * @return true if battery is low
 */
bool power_is_low_battery(void);

/**
 * Get battery voltage
 * @return Battery voltage in volts
 */
float power_get_voltage(void);

/**
 * Enter deep sleep mode
 * @param seconds Time to sleep (0 = indefinite until wakeup)
 */
void power_deep_sleep(uint32_t seconds);

/**
 * Get estimated battery life in seconds
 * @return Estimated seconds remaining
 */
uint32_t power_get_estimated_life(void);

#ifdef __cplusplus
}
#endif

#endif // __POWER_H
