/**
 * SD Card Driver Header
 */

#ifndef __SDCARD_H
#define __SDCARD_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SD Card information structure
 */
typedef struct {
    char card_name[16];          // Card identification
    uint64_t card_size;          // Total size in bytes
    uint64_t free_space;         // Free space in bytes
    uint64_t used_space;         // Used space in bytes
    bool initialized;            // Initialization status
} sdcard_info_t;

/**
 * Initialize SD Card
 * @return ESP_OK on success
 */
esp_err_t sdcard_init(void);

/**
 * Deinitialize SD Card
 * @return ESP_OK on success
 */
esp_err_t sdcard_deinit(void);

/**
 * Get SD Card information
 * @param info Pointer to store card info
 */
void sdcard_get_info(sdcard_info_t *info);

/**
 * Check if SD Card is ready
 * @return true if ready
 */
bool sdcard_is_ready(void);

/**
 * Write data to a file
 * @param path File path (relative to SD root)
 * @param data Data to write
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t sdcard_write_file(const char *path, const uint8_t *data, size_t len);

/**
 * Append data to a file
 * @param path File path
 * @param data Data to append
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t sdcard_append_file(const char *path, const uint8_t *data, size_t len);

/**
 * Read data from a file
 * @param path File path
 * @param data Buffer to read into
 * @param len Buffer size (will be set to actual read size)
 * @return ESP_OK on success
 */
esp_err_t sdcard_read_file(const char *path, uint8_t *data, size_t *len);

/**
 * Read data from a file at specific offset
 * @param path File path
 * @param offset Byte offset to start reading from
 * @param data Buffer to read into
 * @param len Bytes to read (will be set to actual read size)
 * @return ESP_OK on success
 */
esp_err_t sdcard_read_file_offset(const char *path, size_t offset, uint8_t *data, size_t *len);

/**
 * Delete a file
 * @param path File path
 * @return ESP_OK on success
 */
esp_err_t sdcard_delete_file(const char *path);

/**
 * Check if file exists
 * @param path File path
 * @return true if exists
 */
bool sdcard_exists(const char *path);

/**
 * List files in a directory
 * @param path Directory path (NULL or "/" for root)
 * @param buffer Buffer to store listing
 * @param buffer_size Buffer size
 * @return Number of files found
 */
int sdcard_list_files(const char *path, char *buffer, size_t buffer_size);

/**
 * Get file size
 * @param path File path
 * @return File size in bytes, or -1 on error
 */
int64_t sdcard_get_file_size(const char *path);

/**
 * Create a directory
 * @param path Directory path
 * @return ESP_OK on success
 */
esp_err_t sdcard_mkdir(const char *path);

/**
 * Get the next sequential filename
 * @param prefix Filename prefix
 * @param extension File extension (e.g., ".jpg")
 * @param buffer Buffer for result
 * @param buffer_size Buffer size
 * @param index Starting index (will be updated)
 * @return Full filename
 */
const char *sdcard_get_next_filename(const char *prefix, const char *extension,
                                      char *buffer, size_t buffer_size, uint32_t *index);

#ifdef __cplusplus
}
#endif

#endif // __SDCARD_H
