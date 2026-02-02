/**
 * Web Server Header for Remote Control
 */

#ifndef __WEBSERVER_H
#define __WEBSERVER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize web server
 * @param port HTTP port (default 80)
 * @return ESP_OK on success
 */
esp_err_t webserver_init(int port);

/**
 * Deinitialize web server
 */
void webserver_deinit(void);

/**
 * Start web server
 */
esp_err_t webserver_start(void);

/**
 * Stop web server
 */
esp_err_t webserver_stop(void);

/**
 * Check if web server is running
 */
bool webserver_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // __WEBSERVER_H
