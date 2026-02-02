/**
 * Chinese Font Management System
 * Supports GB2312 font from SD card
 */

#ifndef __FONT_H
#define __FONT_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Font sizes (in pixels)
#define FONT_SIZE_12 12
#define FONT_SIZE_16 16
#define FONT_SIZE_24 24
#define FONT_SIZE_32 32

// GB2312 constants
#define GB2312_MAX_CHAR_SIZE 32  // Max font size in pixels

/**
 * Initialize font system
 * Loads GB2312 font from SD card if available
 * @param font_path Path to font file on SD card (e.g., "/font/GB2312-16.fon")
 * @return ESP_OK on success, ESP_FAIL if font not found but system still works
 */
esp_err_t font_init(const char *font_path);

/**
 * Manually set index offset for HZK16 format tuning
 * Use this for debugging different HZK16 file variants
 * @param offset Offset value to add/subtract from character index
 */
void font_set_index_offset(int offset);

/**
 * Deinitialize font system
 */
esp_err_t font_deinit(void);

/**
 * Check if Chinese font is available
 * @return true if GB2312 font loaded successfully
 */
bool font_is_chinese_available(void);

/**
 * Get the byte size of a GB2312 character in font
 * GB2312 uses 2-byte encoding for Chinese characters
 * @param size Font size (12, 16, 24, 32)
 * @return Size in bytes for single character bitmap
 */
int font_get_char_byte_size(int size);

/**
 * Load a single GB2312 character bitmap
 * @param char_hi First byte of GB2312 character code
 * @param char_lo Second byte of GB2312 character code
 * @param size Font size (12, 16, 24, 32)
 * @param buffer Buffer to store character bitmap (must be large enough)
 * @param buffer_size Size of buffer
 * @return Actual bytes read, 0 if character not found
 */
int font_load_char_bitmap(uint8_t char_hi, uint8_t char_lo, int size,
                          uint8_t *buffer, size_t buffer_size);

/**
 * Get width and height of character in font
 * @param size Font size
 * @param width Output parameter for width in pixels
 * @param height Output parameter for height in pixels
 */
void font_get_char_size(int size, int *width, int *height);

/**
 * Check if a character code is valid GB2312
 * @param char_hi First byte
 * @param char_lo Second byte
 * @return true if valid GB2312 code
 */
bool font_is_valid_gb2312(uint8_t char_hi, uint8_t char_lo);

/**
 * Get information about loaded font
 * @param size_out Output for font size (12, 16, 24, or 32)
 * @param file_size Output for total font file size in bytes
 * @return ESP_OK if font info available
 */
esp_err_t font_get_info(int *size_out, uint32_t *file_size);

/**
 * Convert Unicode code point to GB2312 bytes
 * @param unicode Unicode code point (typically from UTF-8 decoding)
 * @param char_hi Output for high byte of GB2312 code
 * @param char_lo Output for low byte of GB2312 code
 * @return true if conversion succeeded
 */
bool font_unicode_to_gb2312(uint16_t unicode, uint8_t *char_hi, uint8_t *char_lo);

/**
 * Decode next UTF-8 character and convert it to GB2312
 * @param utf8_ptr Pointer to a UTF-8 string cursor (will advance past the decoded char)
 * @param char_hi Output for high byte of GB2312 code (valid when function returns true)
 * @param char_lo Output for low byte of GB2312 code (valid when function returns true)
 * @return true if conversion succeeded and GB2312 character is available
 */
bool font_utf8_to_gb2312(const uint8_t **utf8_ptr, uint8_t *char_hi, uint8_t *char_lo);

#ifdef __cplusplus
}
#endif

#endif // __FONT_H
