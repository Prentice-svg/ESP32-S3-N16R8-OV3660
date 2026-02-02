/**
 * WiFi Remote Control Implementation
 * Provides WiFi access for remote camera control
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/ip4_addr.h"
#include "wifi.h"

static const char *TAG = "wifi";
static bool is_init = false;
static bool is_connected = false;
static wifi_mode_t current_mode = WIFI_MODE_NULL;
static char ip_address[16] = "0.0.0.0";
static esp_netif_t *sta_netif = NULL;
static esp_netif_t *ap_netif = NULL;

// Forward declaration
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);

/**
 * Initialize WiFi
 */
esp_err_t wifi_init(wifi_mode_t mode, const char *ssid, const char *password)
{
    if (is_init) {
        wifi_module_deinit();
    }

    ESP_LOGI(TAG, "Initializing WiFi in %s mode", mode == WIFI_MODE_AP ? "AP" : "STA");

    // Initialize event loop
    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default netifs
    sta_netif = esp_netif_create_default_wifi_sta();
    ap_netif = esp_netif_create_default_wifi_ap();

    // Initialize WiFi
    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));

    // Register WiFi and IP event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // Start based on mode
    if (mode == WIFI_MODE_STA) {
        ret = wifi_start_sta(ssid, password);
        if (ret != ESP_OK) return ret;
    } else if (mode == WIFI_MODE_AP) {
        ret = wifi_start_ap(ssid, password ? password : "");
        if (ret != ESP_OK) return ret;
    }

    is_init = true;
    return ESP_OK;
}

/**
 * Deinitialize WiFi
 */
void wifi_module_deinit(void)
{
    if (!is_init) return;

    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();

    if (sta_netif) {
        esp_netif_destroy(sta_netif);
        sta_netif = NULL;
    }
    if (ap_netif) {
        esp_netif_destroy(ap_netif);
        ap_netif = NULL;
    }

    is_init = false;
    is_connected = false;
    current_mode = WIFI_MODE_NULL;

    ESP_LOGI(TAG, "WiFi deinitialized");
}

/**
 * Start WiFi in station mode
 */
esp_err_t wifi_start_sta(const char *ssid, const char *password)
{
    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password) {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Wait for connection
    int retries = 30;
    while (retries-- > 0 && !is_connected) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (!is_connected) {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
        return ESP_FAIL;
    }

    current_mode = WIFI_MODE_STA;
    ESP_LOGI(TAG, "Connected to WiFi, IP: %s", ip_address);

    return ESP_OK;
}

/**
 * Start WiFi in AP mode
 */
esp_err_t wifi_start_ap(const char *ssid, const char *password)
{
    ESP_LOGI(TAG, "Starting WiFi AP: %s", ssid);

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = strlen(ssid);

    if (password && strlen(password) >= 8) {
        strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    wifi_config.ap.max_connection = 4;
    wifi_config.ap.beacon_interval = 100;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    // Configure IP address for AP
    esp_netif_ip_info_t ip_info = {0};
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    esp_netif_dhcps_stop(ap_netif);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif));

    ESP_ERROR_CHECK(esp_wifi_start());

    // Wait for AP to be fully started
    vTaskDelay(pdMS_TO_TICKS(100));

    current_mode = WIFI_MODE_AP;
    is_connected = true;  // AP mode is always "connected"
    strcpy(ip_address, "192.168.4.1");

    ESP_LOGI(TAG, "AP started, IP: %s", ip_address);
    ESP_LOGI(TAG, "SSID: %s", ssid);
    ESP_LOGI(TAG, "Password: %s", password ? password : "Open");

    return ESP_OK;
}

/**
 * Stop WiFi
 */
void wifi_stop(void)
{
    if (is_init) {
        esp_wifi_disconnect();
        esp_wifi_stop();
        is_connected = false;
    }
}

/**
 * Get WiFi connection status
 */
bool wifi_is_connected(void)
{
    return is_connected;
}

/**
 * Get IP address
 */
const char *wifi_get_ip_address(void)
{
    return ip_address;
}

/**
 * Get WiFi mode
 */
wifi_mode_t wifi_get_mode(void)
{
    return current_mode;
}

/**
 * Event handler for WiFi events
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "Station started");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Connected to AP");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "Disconnected from AP");
                is_connected = false;
                break;
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "AP started");
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "Station connected to AP");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG, "Station disconnected from AP");
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            snprintf(ip_address, sizeof(ip_address), IPSTR, IP2STR(&event->ip_info.ip));
            is_connected = true;
            ESP_LOGI(TAG, "Got IP: %s", ip_address);
        }
    }
}
