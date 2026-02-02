/**
 * Timelapse Engine Header
 */

#ifndef __TIMELAPSE_H
#define __TIMELAPSE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Image resolution options
 */
typedef enum {
    RES_CIF = 0,       // 352x288
    RES_VGA,           // 640x480
    RES_SVGA,          // 800x600
    RES_XGA,           // 1024x768
    RES_SXGA,          // 1280x1024
    RES_UXGA,          // 1600x1200
    RES_QVGA,          // 320x240
    RES_HD,            // 1280x720
    RES_FHD,           // 1920x1080
    RES_MAX
} resolution_t;

/**
 * JPEG quality options
 */
typedef enum {
    QUALITY_LOW = 10,      // Lower quality, smaller file
    QUALITY_MEDIUM = 30,
    QUALITY_HIGH = 50,
    QUALITY_MAX = 63
} quality_t;

/**
 * Timelapse state
 */
typedef enum {
    TIMELAPSE_IDLE,        // Not running
    TIMELAPSE_RUNNING,     // Currently shooting
    TIMELAPSE_PAUSED,      // Paused
    TIMELAPSE_COMPLETED,   // Finished all shots
    TIMELAPSE_ERROR        // Error occurred
} timelapse_state_t;

/**
 * Timelapse configuration
 */
typedef struct {
    uint32_t interval_sec;      // Shooting interval in seconds
    uint32_t total_shots;       // Total number of shots (0 = unlimited)
    uint32_t start_delay_sec;   // Delay before first shot
    resolution_t resolution;    // Image resolution
    uint8_t quality;            // JPEG quality (0-63)
    bool auto_start;            // Auto-start on boot
    bool overwrite_mode;        // Overwrite old files when full
    char filename_prefix[32];   // Filename prefix
} timelapse_config_t;

/**
 * Timelapse status
 */
typedef struct {
    timelapse_state_t state;    // Current state
    uint32_t current_shot;      // Current shot number
    uint32_t total_shots;       // Total shots configured
    uint32_t next_shot_sec;     // Seconds until next shot
    uint32_t elapsed_sec;       // Total elapsed time
    uint32_t saved_count;       // Number of files saved
    uint64_t saved_bytes;       // Total bytes saved
    uint64_t free_bytes;        // Free space on SD card
    float battery_voltage;      // Battery voltage (if monitoring)
    uint64_t start_time_sec;    // Session start epoch (seconds)
    uint64_t end_time_sec;      // Session end epoch (seconds, 0 if running)
} timelapse_status_t;

/**
 * Initialize timelapse engine
 */
void timelapse_init(void);

/**
 * Start timelapse shooting
 * @return ESP_OK on success
 */
esp_err_t timelapse_start(void);

/**
 * Stop timelapse shooting
 * @return ESP_OK on success
 */
esp_err_t timelapse_stop(void);

/**
 * Pause timelapse shooting
 * @return ESP_OK on success
 */
esp_err_t timelapse_pause(void);

/**
 * Resume timelapse shooting
 * @return ESP_OK on success
 */
esp_err_t timelapse_resume(void);

/**
 * Take a single photo (manual trigger)
 * @return ESP_OK on success
 */
esp_err_t timelapse_take_photo(void);

/**
 * Get current configuration
 * @return Pointer to configuration
 */
timelapse_config_t *timelapse_get_config(void);

/**
 * Update configuration
 * @param config New configuration
 * @return ESP_OK on success
 */
esp_err_t timelapse_set_config(timelapse_config_t *config);

/**
 * Get current state
 * @return Current state
 */
timelapse_state_t timelapse_get_state(void);

/**
 * Get current progress
 * @return Current shot number
 */
uint32_t timelapse_get_progress(void);

/**
 * Get current status
 * @param status Pointer to status structure
 */
void timelapse_get_status(timelapse_status_t *status);

/**
 * Save configuration to NVS
 * @return ESP_OK on success
 */
esp_err_t timelapse_save_config(void);

/**
 * Load configuration from NVS
 * @return ESP_OK on success
 */
esp_err_t timelapse_load_config(void);

#ifdef __cplusplus
}
#endif

#endif // __TIMELAPSE_H
