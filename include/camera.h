/**
 * Camera Driver Header
 */

#ifndef __CAMERA_H
#define __CAMERA_H

#include <stdint.h>
#include "esp_err.h"
#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

// Use types from esp_camera.h directly
typedef camera_config_t camera_config_t;
typedef camera_fb_t camera_fb_t;

/**
 * Initialize the camera
 * @param config Camera configuration
 * @return ESP_OK on success
 */
esp_err_t camera_init(const camera_config_t *config);

/**
 * Deinitialize the camera
 * @return ESP_OK on success
 */
esp_err_t camera_deinit(void);

/**
 * Take a photo and return the frame buffer
 * @return Pointer to frame buffer (NULL on error)
 */
camera_fb_t *camera_capture(void);

/**
 * Free a frame buffer
 * @param fb Frame buffer to free
 */
void camera_free_fb(camera_fb_t *fb);

/**
 * Get a preview frame (lower resolution)
 * @return Pointer to frame buffer
 */
camera_fb_t *camera_get_preview(void);

/**
 * Check if camera is initialized
 * @return true if initialized
 */
bool camera_is_ready(void);

/**
 * Set camera resolution
 * @param size Frame size to set
 * @return ESP_OK on success
 */
esp_err_t camera_set_framesize(framesize_t size);

/**
 * Set camera JPEG quality
 * @param quality Quality value (0-63, lower is better)
 * @return ESP_OK on success
 */
esp_err_t camera_set_quality(uint8_t quality);

/**
 * Get camera sensor information
 * @return Pointer to sensor descriptor
 */
sensor_t *camera_get_sensor(void);

#ifdef __cplusplus
}
#endif

#endif // __CAMERA_H
