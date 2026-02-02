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
    FILE *font_fp;
    char font_path[256];
    int char_width;
    int char_height;
    int char_bitmap_size;  // Size in bytes for one character
} font_state = {0};

// Font file structure info
// GB2312-12: 12x12 pixels = 18 bytes per character (12*12/8=18)
// GB2312-16: 16x16 pixels = 32 bytes per character (16*16/8=32)
// GB2312-24: 24x24 pixels = 72 bytes per character (24*24/8=72)
// GB2312-32: 32x32 pixels = 128 bytes per character (32*32/8=128)

static const struct {
    int size;
    int width;
    int height;
    int bitmap_size;
} font_info_table[] = {
    {12, 12, 12, 18},
    {16, 16, 16, 32},
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

    return hi_offset * 94 + lo_offset;
}

esp_err_t font_init(const char *font_path)
{
    if (!font_path) {
        ESP_LOGE(TAG, "Font path is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    font_state.initialized = true;
    strncpy(font_state.font_path, font_path, sizeof(font_state.font_path) - 1);
    font_state.font_path[sizeof(font_state.font_path) - 1] = '\0';

    // Try to open font file
    if (!sdcard_exists(font_path)) {
        ESP_LOGW(TAG, "Font file not found: %s", font_path);
        ESP_LOGI(TAG, "System will use ASCII font only. Chinese display will not work.");
        font_state.font_available = false;
        return ESP_FAIL;
    }

    // Get file size
    int64_t file_size = sdcard_get_file_size(font_path);
    if (file_size <= 0) {
        ESP_LOGE(TAG, "Cannot get font file size");
        font_state.font_available = false;
        return ESP_FAIL;
    }
    font_state.font_file_size = (uint32_t)file_size;

    // Determine font size from file size
    // Each character takes font_size/8 * font_size bytes
    // GB2312 has 6763 characters total (but typically files contain subsets)
    // For standard fonts:
    // 12x12 font: 18 bytes/char, total ~121734 bytes
    // 16x16 font: 32 bytes/char, total ~216416 bytes
    // 24x24 font: 72 bytes/char, total ~486456 bytes

    // Try to detect font size
    for (int i = 0; i < FONT_INFO_COUNT; i++) {
        if (file_size > font_info_table[i].bitmap_size * 3400 &&
            file_size < font_info_table[i].bitmap_size * 7000) {
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

    int64_t file_offset = char_idx * (int64_t)font_state.char_bitmap_size;

    // Read character bitmap from file
    // Since sdcard_read_file reads entire file, we need to be careful with memory
    // For now, we use a static buffer approach for commonly used characters

    // Maximum safe allocation - don't read the entire file at once
    const size_t max_read_size = 65536;  // 64KB max per read
    uint8_t *temp_buffer = malloc(max_read_size);
    if (!temp_buffer) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return 0;
    }

    size_t read_size = max_read_size;
    esp_err_t ret = sdcard_read_file(font_state.font_path, temp_buffer, &read_size);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read font file");
        free(temp_buffer);
        return 0;
    }

    // Check if character is within the read data
    if (file_offset + (int64_t)font_state.char_bitmap_size > (int64_t)read_size) {
        ESP_LOGW(TAG, "Character at offset %lld is beyond readable file portion", file_offset);
        free(temp_buffer);
        return 0;
    }

    // Copy character bitmap to output buffer
    memcpy(buffer, &temp_buffer[file_offset], font_state.char_bitmap_size);
    free(temp_buffer);

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
