/**
 * Camera Sensor Header
 * Wrapper for ESP-IDF camera sensor types
 */

#ifndef __SENSOR_H
#define __SENSOR_H

#include <stdint.h>
#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

// Re-export sensor_t from esp_camera component
typedef sensor_t sensor_t;

#ifdef __cplusplus
}
#endif

#endif // __SENSOR_H
