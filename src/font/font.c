/**
 * Chinese Font Implementation
 * Handles GB2312 font loading and character rendering
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "font.h"
#include "sdcard.h"

static const char *TAG = "font";

// Font state
static struct {
    bool initialized;
    bool font_available;
    int font_size;
    uint32_t font_file_size;
    uint32_t header_offset;  // File header offset (0 for standard, 64 for some variants)
    int index_adjust;        // Index adjustment for different HZK16 formats
    FILE *font_fp;
    char font_path[256];
    int char_width;
    int char_height;
    int char_bitmap_size;  // Size in bytes for one character
} font_state = {0};

// Font file structure info
// HZK12: 12x12 pixels = 24 bytes per character (12 rows × 2 bytes/row)
// HZK16: 16x16 pixels = 32 bytes per character (16 rows × 2 bytes/row)
// GB2312-24: 24x24 pixels = 72 bytes per character (24*24/8=72)
// GB2312-32: 32x32 pixels = 128 bytes per character (32*32/8=128)

static const struct {
    int size;
    int width;
    int height;
    int bitmap_size;
} font_info_table[] = {
    {12, 12, 12, 24},   // HZK12: 12 rows × 2 bytes = 24 bytes
    {16, 16, 16, 32},   // HZK16: 16 rows × 2 bytes = 32 bytes
    {24, 24, 24, 72},
    {32, 32, 32, 128},
};

#define FONT_INFO_COUNT (sizeof(font_info_table) / sizeof(font_info_table[0]))

/**
 * Calculate GB2312 character index
 * GB2312 encoding: high byte 0xA1-0xF7, low byte 0xA1-0xFE
 * Characters organized sequentially in font file
 */
static int32_t gb2312_char_index(uint8_t char_hi, uint8_t char_lo)
{
    // Valid range check
    if (char_hi < 0xA1 || char_hi > 0xF7)
        return -1;
    if (char_lo < 0xA1 || char_lo > 0xFE)
        return -1;

    // Calculate position in font file
    // Each first byte has 94 characters (0xA1-0xFE)
    int hi_offset = char_hi - 0xA1;
    int lo_offset = char_lo - 0xA1;

    int index = hi_offset * 94 + lo_offset;

    // Apply manual offset adjustment (for debugging different HZK16 formats)
    if (font_state.index_adjust != 0) {
        index += font_state.index_adjust;
        if (index < 0) {
            index = 0;
        }
    }

    return index;
}

esp_err_t font_init(const char *font_path)
{
    if (!font_path) {
        ESP_LOGE(TAG, "Font path is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize font state
    memset(&font_state, 0, sizeof(font_state));
    font_state.initialized = true;
    strncpy(font_state.font_path, font_path, sizeof(font_state.font_path) - 1);
    font_state.font_path[sizeof(font_state.font_path) - 1] = '\0';

    // Remove /sdcard prefix if present (sdcard functions add it automatically)
    const char *rel_path = font_path;
    if (strncmp(rel_path, "/sdcard/", 8) == 0) {
        rel_path = font_path + 8;
    }

    // Try to open font file
    if (!sdcard_exists(rel_path)) {
        ESP_LOGW(TAG, "Font file not found: %s", font_path);
        ESP_LOGI(TAG, "System will use ASCII font only. Chinese display will not work.");
        font_state.font_available = false;
        return ESP_FAIL;
    }

    // Get file size
    int64_t file_size = sdcard_get_file_size(rel_path);
    if (file_size <= 0) {
        ESP_LOGE(TAG, "Cannot get font file size");
        font_state.font_available = false;
        return ESP_FAIL;
    }
    font_state.font_file_size = (uint32_t)file_size;

    // Determine font size from file size
    // Standard GB2312 font file contains 8178 characters (94*87 matrix)
    // Calculate expected file sizes:
    // 12x12 font: 18 bytes/char * 8178 = 147204 bytes
    // 16x16 font: 32 bytes/char * 8178 = 261696 bytes
    // 24x24 font: 72 bytes/char * 8178 = 588816 bytes
    // 32x32 font: 128 bytes/char * 8178 = 1046784 bytes

    // Try to detect font size by checking if file size matches expected character count
    // Allow some tolerance (±5%) for different font file formats
    for (int i = 0; i < FONT_INFO_COUNT; i++) {
        int expected_size = font_info_table[i].bitmap_size * 8178;  // Standard GB2312 char count

        // Check for exact match first
        if (file_size == expected_size) {
            font_state.font_size = font_info_table[i].size;
            font_state.char_width = font_info_table[i].width;
            font_state.char_height = font_info_table[i].height;
            font_state.char_bitmap_size = font_info_table[i].bitmap_size;
            font_state.font_available = true;

            ESP_LOGI(TAG, "Font detected: %dx%d (%d bytes/char), file size: %llu",
                     font_state.char_width, font_state.char_height,
                     font_state.char_bitmap_size, (unsigned long long)file_size);
            return ESP_OK;
        }
    }

    // Special case: accept known HZK16 variants with extended character sets
    // Some HZK16 files contain additional GBK characters beyond standard GB2312
    // 267616 bytes = 32 * 8363 chars (includes GBK extension, has 64-byte header)
    // 261696 bytes = 32 * 8178 chars (standard GB2312)
    if (file_size == 267616 || file_size == 261696) {
        font_state.font_size = 16;
        font_state.char_width = 16;
        font_state.char_height = 16;
        font_state.char_bitmap_size = 32;
        font_state.font_available = true;

        // Some HZK16 variants have a 64-byte header
        if (file_size == 267616) {
            font_state.header_offset = 64;
            font_state.index_adjust = 0;  // Start with 0, can be adjusted via font_set_index_offset()
            ESP_LOGI(TAG, "HZK16 font detected: 16x16 (32 bytes/char), file size: %llu (with 64-byte header)",
                     (unsigned long long)file_size);
            ESP_LOGI(TAG, "If Chinese chars display wrong, try calling font_set_index_offset() with different values");
        } else {
            font_state.header_offset = 0;
            font_state.index_adjust = 0;  // Standard format
            ESP_LOGI(TAG, "HZK16 font detected: 16x16 (32 bytes/char), file size: %llu",
                     (unsigned long long)file_size);
        }
        return ESP_OK;
    }

    // HZK12 font: 12x12 pixels, 18 bytes per character
    // 196272 bytes = 18 * 10872 chars + 576-byte header
    if (file_size == 196272) {
        font_state.font_size = 12;
        font_state.char_width = 12;
        font_state.char_height = 12;
        font_state.char_bitmap_size = 18;
        font_state.header_offset = 576;
        font_state.index_adjust = 0;
        font_state.font_available = true;

        ESP_LOGI(TAG, "HZK12 font detected: 12x12 (18 bytes/char), file size: %llu (with 576-byte header)",
                 (unsigned long long)file_size);
        return ESP_OK;
    }

    ESP_LOGW(TAG, "Font file size doesn't match known formats. File size: %llu",
             (unsigned long long)file_size);
    font_state.font_available = false;
    return ESP_FAIL;
}

esp_err_t font_deinit(void)
{
    if (font_state.font_fp) {
        fclose(font_state.font_fp);
        font_state.font_fp = NULL;
    }

    font_state.initialized = false;
    font_state.font_available = false;
    return ESP_OK;
}

bool font_is_chinese_available(void)
{
    return font_state.initialized && font_state.font_available;
}

void font_set_index_offset(int offset)
{
    font_state.index_adjust = offset;
    ESP_LOGI(TAG, "Font index offset set to: %d", offset);
}

int font_get_char_byte_size(int size)
{
    for (int i = 0; i < FONT_INFO_COUNT; i++) {
        if (font_info_table[i].size == size) {
            return font_info_table[i].bitmap_size;
        }
    }
    return -1;
}

int font_load_char_bitmap(uint8_t char_hi, uint8_t char_lo, int size,
                          uint8_t *buffer, size_t buffer_size)
{
    if (!font_state.initialized || !font_state.font_available) {
        ESP_LOGW(TAG, "Font not available");
        return 0;
    }

    // Check if requested size matches loaded font size
    if (size != font_state.font_size) {
        ESP_LOGW(TAG, "Requested size %d but font is %d", size, font_state.font_size);
        return 0;
    }

    // Check buffer size
    if (buffer_size < (size_t)font_state.char_bitmap_size) {
        ESP_LOGE(TAG, "Buffer too small. Need %d, got %zu",
                 font_state.char_bitmap_size, buffer_size);
        return 0;
    }

    // Validate character code
    if (!font_is_valid_gb2312(char_hi, char_lo)) {
        return 0;
    }

    // Calculate character position in file
    int32_t char_idx = gb2312_char_index(char_hi, char_lo);
    if (char_idx < 0) {
        return 0;
    }

    int64_t file_offset = font_state.header_offset + char_idx * (int64_t)font_state.char_bitmap_size;

    // Debug logging for first few characters
    static int debug_count = 0;
    if (debug_count < 5) {
        ESP_LOGI(TAG, "Char 0x%02X%02X: idx=%ld, offset=%lld", char_hi, char_lo, (long)char_idx, file_offset);
        debug_count++;
    }

    // Read character bitmap directly at offset
    size_t read_size = font_state.char_bitmap_size;
    
    // Remove /sdcard prefix if present (sdcard functions add it)
    const char *rel_path = font_state.font_path;
    if (strncmp(rel_path, "/sdcard/", 8) == 0) {
        rel_path = font_state.font_path + 8;
    }
    
    esp_err_t ret = sdcard_read_file_offset(rel_path, (size_t)file_offset, buffer, &read_size);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read font file at offset %lld", file_offset);
        return 0;
    }

    if (read_size != (size_t)font_state.char_bitmap_size) {
        ESP_LOGW(TAG, "Incomplete read: got %zu, expected %d", read_size, font_state.char_bitmap_size);
        return 0;
    }

    return font_state.char_bitmap_size;
}

void font_get_char_size(int size, int *width, int *height)
{
    for (int i = 0; i < FONT_INFO_COUNT; i++) {
        if (font_info_table[i].size == size) {
            if (width) *width = font_info_table[i].width;
            if (height) *height = font_info_table[i].height;
            return;
        }
    }

    // Default to loaded font size if available
    if (font_state.font_available) {
        if (width) *width = font_state.char_width;
        if (height) *height = font_state.char_height;
    } else {
        if (width) *width = 0;
        if (height) *height = 0;
    }
}

bool font_is_valid_gb2312(uint8_t char_hi, uint8_t char_lo)
{
    // GB2312 valid ranges
    if (char_hi >= 0xA1 && char_hi <= 0xF7 &&
        char_lo >= 0xA1 && char_lo <= 0xFE) {
        return true;
    }
    return false;
}

esp_err_t font_get_info(int *size_out, uint32_t *file_size)
{
    if (!font_state.font_available) {
        return ESP_FAIL;
    }

    if (size_out) *size_out = font_state.font_size;
    if (file_size) *file_size = font_state.font_file_size;

    return ESP_OK;
}
