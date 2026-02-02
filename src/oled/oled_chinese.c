/**
 * OLED Chinese Character Display Extension
 * Provides functions to render GB2312 Chinese characters on SSD1306 OLED
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "oled_chinese.h"
#include "font.h"
#include "oled.h"

static const char *TAG = "oled_chinese";

/**
 * Draw a single GB2312 Chinese character
 * The character bitmap is expected to be in column-major format
 * (common for dot-matrix fonts)
 */
static void oled_draw_char_bitmap(int x, int y, const uint8_t *bitmap,
                                  int char_width, int char_height, bool on)
{
    if (!bitmap) return;

    // Draw bitmap on OLED buffer
    // Bitmap format: each row is packed into bytes
    for (int row = 0; row < char_height; row++) {
        for (int col = 0; col < char_width; col++) {
            // Calculate byte position in bitmap
            // Format: 1 byte per row (8 pixels wide), rows stacked
            int byte_idx = row * ((char_width + 7) / 8) + col / 8;
            int bit_idx = 7 - (col & 7);

            if (byte_idx < 256) {  // Safety check
                uint8_t pixel = (bitmap[byte_idx] >> bit_idx) & 1;
                if (pixel) {
                    oled_set_pixel(x + col, y + row, on);
                }
            }
        }
    }
}

int oled_draw_chinese_char(int x, int y, uint8_t char_hi, uint8_t char_lo, bool on)
{
    if (!font_is_chinese_available()) {
        ESP_LOGW(TAG, "Chinese font not available");
        return 0;
    }

    // Get font size - we're hardcoding to 16pt for now
    // To support other sizes, would need to change the API
    const int font_size = 16;
    int font_width, font_height;
    font_get_char_size(font_size, &font_width, &font_height);

    // Load character bitmap
    uint8_t char_bitmap[256];  // Large enough for any size (32x32 = 128 bytes)
    int bitmap_size = font_load_char_bitmap(char_hi, char_lo, font_size,
                                            char_bitmap, sizeof(char_bitmap));

    if (bitmap_size <= 0) {
        ESP_LOGW(TAG, "Character 0x%02X%02X not found in font", char_hi, char_lo);
        return 0;
    }

    // Draw the character bitmap
    oled_draw_char_bitmap(x, y, char_bitmap, font_width, font_height, on);

    return font_width;  // Return character width for positioning
}

int oled_draw_chinese_string(int x, int y, const char *str, bool on)
{
    if (!str || !font_is_chinese_available()) {
        return 0;
    }

    const int font_size = 16;
    int font_width = 0;
    font_get_char_size(font_size, &font_width, NULL);

    int current_x = x;
    int current_y = y;

    // Parse UTF-8 encoded Chinese string
    // In GB2312, each character is 2 bytes
    const uint8_t *ptr = (const uint8_t *)str;

    while (*ptr) {
        // Check for Chinese character (high byte indicates GB2312)
        if (*ptr >= 0xA1 && *ptr <= 0xF7) {
            uint8_t char_hi = *ptr;
            uint8_t char_lo = *(ptr + 1);

            if (char_lo >= 0xA1 && char_lo <= 0xFE) {
                // Valid GB2312 character
                oled_draw_chinese_char(current_x, current_y, char_hi, char_lo, on);
                current_x += font_width;
                ptr += 2;

                // Handle line wrapping
                if (current_x + font_width > OLED_WIDTH) {
                    current_x = x;
                    current_y += font_width;  // Use font_width for line height

                    if (current_y + font_width > OLED_HEIGHT) {
                        break;  // No more space
                    }
                }
                continue;
            }
        }

        // Skip invalid characters
        ptr++;
    }

    return current_x - x;
}

int oled_draw_mixed_string(int x, int y, const char *str, int font_size, bool on)
{
    if (!str) return 0;

    // Note: Currently only supports 16pt font for Chinese characters
    // The font_size parameter is mainly used for spacing/layout calculations
    const int chinese_font_size = 16;

    int current_x = x;
    int current_y = y;

    const uint8_t *ptr = (const uint8_t *)str;

    while (*ptr) {
        // Check for GB2312 Chinese character
        if (*ptr >= 0xA1 && *ptr <= 0xF7 && *(ptr + 1) >= 0xA1 && *(ptr + 1) <= 0xFE) {
            uint8_t char_hi = *ptr;
            uint8_t char_lo = *(ptr + 1);

            // Only draw if Chinese font is available
            if (font_is_chinese_available()) {
                oled_draw_chinese_char(current_x, current_y, char_hi, char_lo, on);
                current_x += chinese_font_size;
            } else {
                // Fallback: skip the character
                current_x += font_size;
            }
            ptr += 2;
        } else {
            // ASCII character
            if (*ptr >= 32 && *ptr < 127) {
                current_x += oled_draw_char(current_x, current_y, *ptr, 1);
            }
            ptr++;
        }

        // Handle line wrapping
        if (current_x + font_size > OLED_WIDTH) {
            current_x = x;
            current_y += font_size;

            if (current_y + font_size > OLED_HEIGHT) {
                break;
            }
        }
    }

    return current_x - x;
}

esp_err_t oled_show_chinese_message(const char *line1, const char *line2, const char *line3)
{
    if (!font_is_chinese_available()) {
        ESP_LOGW(TAG, "Chinese font not available, using ASCII message");
        oled_show_message(line1, line2, line3);
        return ESP_FAIL;
    }

    oled_clear();

    if (line1) {
        oled_draw_mixed_string(0, 0, line1, 16, true);
    }

    if (line2) {
        oled_draw_mixed_string(0, 20, line2, 16, true);
    }

    if (line3) {
        oled_draw_mixed_string(0, 40, line3, 16, true);
    }

    oled_update();
    return ESP_OK;
}
