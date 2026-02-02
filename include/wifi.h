/**
 * WiFi Remote Control Header
 */

#ifndef __WIFI_H
#define __WIFI_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

// Use wifi_mode_t from esp_wifi.h directly
typedef wifi_mode_t wifi_mode_t;

/**
 * Initialize WiFi
 * @param mode WiFi mode (WIFI_MODE_STA or WIFI_MODE_AP)
 * @param ssid SSID (for AP mode or STA network)
 * @param password Password
 * @return ESP_OK on success
 */
esp_err_t wifi_init(wifi_mode_t mode, const char *ssid, const char *password);

/**
 * Deinitialize WiFi
 */
void wifi_module_deinit(void);

/**
 * Get WiFi connection status
 * @return true if connected
 */
bool wifi_is_connected(void);

/**
 * Get IP address
 * @return IP address string
 */
const char *wifi_get_ip_address(void);

/**
 * Start WiFi in station mode
 * @param ssid Network SSID
 * @param password Network password
 * @return ESP_OK on success
 */
esp_err_t wifi_start_sta(const char *ssid, const char *password);

/**
 * Start WiFi in AP mode
 * @param ssid AP name
 * @param password AP password
 * @return ESP_OK on success
 */
esp_err_t wifi_start_ap(const char *ssid, const char *password);

/**
 * Stop WiFi
 */
void wifi_stop(void);

/**
 * Get WiFi mode
 * @return Current WiFi mode
 */
wifi_mode_t wifi_get_mode(void);

#ifdef __cplusplus
}
#endif

#endif // __WIFI_H
