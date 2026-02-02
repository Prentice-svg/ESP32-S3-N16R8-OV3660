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
static bool s_logged_first_char = false;

// Font scale factor (0.5 to 1.0)
static float s_font_scale = OLED_CHINESE_FONT_SCALE_DEFAULT;

/**
 * Set Chinese font scale factor
 */
void oled_set_chinese_font_scale(float scale)
{
    // Clamp scale between 0.5 and 1.0
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 1.0f) scale = 1.0f;
    s_font_scale = scale;
    ESP_LOGI(TAG, "Chinese font scale set to: %.2f", s_font_scale);
}

/**
 * Get current Chinese font scale factor
 */
float oled_get_chinese_font_scale(void)
{
    return s_font_scale;
}

/**
 * Draw a single GB2312 Chinese character
 * Standard HZK16 format (horizontal scanning):
 * - 16x16 pixels, 32 bytes per character
 * - Row-major: 2 bytes per row, 16 rows
 * - Left to right, top to bottom
 * - MSB of each byte is leftmost pixel
 *
 * With scaling support to reduce font size
 */
static void oled_draw_char_bitmap(int x, int y, const uint8_t *bitmap,
                                  int char_width, int char_height, bool on)
{
    if (!bitmap) return;

    // Apply scaling - sample from original bitmap
    int scaled_width = (int)(char_width * s_font_scale);
    int scaled_height = (int)(char_height * s_font_scale);

    // Ensure minimum size
    if (scaled_width < 8) scaled_width = 8;
    if (scaled_height < 8) scaled_height = 8;

    // Standard HZK16 row-major format
    int bytes_per_row = (char_width + 7) / 8;  // 2 for 16-pixel width

    for (int row = 0; row < scaled_height; row++) {
        for (int col = 0; col < scaled_width; col++) {
            // Calculate corresponding position in original bitmap
            int src_row = (int)(row / s_font_scale);
            int src_col = (int)(col / s_font_scale);

            if (src_row >= char_height || src_col >= char_width) {
                continue;
            }

            int byte_idx = src_row * bytes_per_row + (src_col / 8);
            int bit_idx = 7 - (src_col % 8);  // MSB is leftmost pixel

            if (byte_idx < char_height * bytes_per_row) {
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

    // Get actual loaded font size (not hardcoded)
    int font_size = 0;
    if (font_get_info(&font_size, NULL) != ESP_OK || font_size <= 0) {
        ESP_LOGW(TAG, "Cannot get font info");
        return 0;
    }
    
    int font_width = 0, font_height = 0;
    font_get_char_size(font_size, &font_width, &font_height);
    if (font_width <= 0 || font_height <= 0) {
        ESP_LOGW(TAG, "Invalid font dimensions");
        return 0;
    }

    // Load character bitmap
    uint8_t char_bitmap[256];  // Large enough for any size (32x32 = 128 bytes)
    int bitmap_size = font_load_char_bitmap(char_hi, char_lo, font_size,
                                            char_bitmap, sizeof(char_bitmap));

    if (bitmap_size <= 0) {
        ESP_LOGD(TAG, "Character 0x%02X%02X not found in font", char_hi, char_lo);
        return 0;
    }

    bool log_this_char = false;
    if (!s_logged_first_char) {
        log_this_char = true;
    }

    int before_on = 0;
    if (log_this_char) {
        char hexbuf[64] = {0};
        int hexlen = 0;
        int dump = bitmap_size < 16 ? bitmap_size : 16;
        for (int i = 0; i < dump; i++) {
            hexlen += snprintf(hexbuf + hexlen, sizeof(hexbuf) - hexlen, "%02X ", char_bitmap[i]);
            if (hexlen >= (int)sizeof(hexbuf)) {
                break;
            }
        }
        ESP_LOGI(TAG, "First Chinese char 0x%02X%02X size=%d bytes, bytes=%s", char_hi, char_lo, bitmap_size, hexbuf);

        for (int row = 0; row < font_height; row++) {
            for (int col = 0; col < font_width; col++) {
                if (oled_get_pixel_state(x + col, y + row)) {
                    before_on++;
                }
            }
        }
    }

    // Draw the character bitmap
    oled_draw_char_bitmap(x, y, char_bitmap, font_width, font_height, on);

    if (log_this_char) {
        char rowbuf[65];
        int after_on = 0;
        int scaled_width = (int)(font_width * s_font_scale);
        int scaled_height = (int)(font_height * s_font_scale);
        if (scaled_width < 8) scaled_width = 8;
        if (scaled_height < 8) scaled_height = 8;
        int max_cols = scaled_width;
        if (max_cols >= (int)sizeof(rowbuf)) {
            max_cols = sizeof(rowbuf) - 1;
        }

        for (int row = 0; row < scaled_height; row++) {
            for (int col = 0; col < max_cols; col++) {
                bool pixel_on = oled_get_pixel_state(x + col, y + row);
                after_on += pixel_on ? 1 : 0;
                rowbuf[col] = pixel_on ? '#' : '.';
            }
            rowbuf[max_cols] = '\0';
            ESP_LOGI(TAG, "Row %02d (%d): %s", row, y + row, rowbuf);
        }

        ESP_LOGI(TAG, "Chinese block (%d,%d) %dx%d (scaled from %dx%d) on-before=%d on-after=%d",
                 x, y, scaled_width, scaled_height, font_width, font_height, before_on, after_on);
        s_logged_first_char = true;
    }

    // Return scaled width for positioning
    int scaled_width = (int)(font_width * s_font_scale);
    if (scaled_width < 8) scaled_width = 8;
    return scaled_width;  // Return character width for positioning
}

int oled_draw_chinese_string(int x, int y, const char *str, bool on)
{
    int font_size = 0;
    font_get_info(&font_size, NULL);
    if (font_size <= 0) font_size = 16;  // fallback
    return oled_draw_mixed_string(x, y, str, font_size, on);
}

/**
 * Check if a byte sequence looks like GB2312 encoding (not UTF-8)
 * GB2312: both bytes in range 0xA1-0xFE
 * UTF-8 Chinese: starts with 0xE0-0xEF, followed by 0x80-0xBF continuation bytes
 */
static bool is_likely_gb2312(const uint8_t *str)
{
    if (!str) return false;
    
    while (*str) {
        if (*str < 0x80) {
            str++;
            continue;
        }
        
        // Check for UTF-8 pattern (Chinese starts with 0xE0-0xEF)
        if ((*str & 0xE0) == 0xE0 && (str[1] & 0xC0) == 0x80) {
            return false;  // Looks like UTF-8
        }
        
        // Check for GB2312 pattern (both bytes 0xA1-0xFE)
        if (*str >= 0xA1 && *str <= 0xF7 && str[1] >= 0xA1 && str[1] <= 0xFE) {
            return true;  // Looks like GB2312
        }
        
        // Unknown high-byte pattern, skip
        str++;
    }
    return false;
}

int oled_draw_mixed_string(int x, int y, const char *str, int font_size, bool on)
{
    if (!str) {
        return 0;
    }

    // Get actual loaded font size if available
    int actual_font_size = 0;
    if (font_get_info(&actual_font_size, NULL) == ESP_OK && actual_font_size > 0) {
        font_size = actual_font_size;
    } else if (font_size <= 0) {
        font_size = 16;  // fallback default
    }

    int chinese_width = font_size;
    font_get_char_size(font_size, &chinese_width, NULL);
    if (chinese_width <= 0) {
        chinese_width = font_size;
    }

    // Apply scaling to get actual display width
    int scaled_chinese_width = (int)(chinese_width * s_font_scale);
    if (scaled_chinese_width < 8) scaled_chinese_width = 8;

    int current_x = x;
    int current_y = y;
    const uint8_t *ptr = (const uint8_t *)str;

    // Detect encoding: GB2312 or UTF-8
    bool use_gb2312_direct = is_likely_gb2312(ptr);

    while (*ptr) {
        if (*ptr == '\n') {
            current_x = x;
            current_y += scaled_chinese_width;
            ptr++;
            continue;
        }

        if (*ptr < 0x80) {
            // ASCII character
            if (current_x + 8 > OLED_WIDTH) {
                current_x = x;
                current_y += scaled_chinese_width;
                if (current_y + scaled_chinese_width > OLED_HEIGHT) {
                    break;
                }
            }
            current_x += oled_draw_char_color(current_x, current_y, (char)*ptr, 1, on);
            ptr++;
            continue;
        }

        // Check line wrapping before drawing Chinese character
        if (current_x + scaled_chinese_width > OLED_WIDTH) {
            current_x = x;
            current_y += scaled_chinese_width;
            if (current_y + scaled_chinese_width > OLED_HEIGHT) {
                break;
            }
        }

        if (use_gb2312_direct) {
            // Direct GB2312: two consecutive high bytes
            uint8_t char_hi = *ptr++;
            uint8_t char_lo = *ptr ? *ptr++ : 0;
            
            if (font_is_chinese_available() && font_is_valid_gb2312(char_hi, char_lo)) {
                int drawn = oled_draw_chinese_char(current_x, current_y, char_hi, char_lo, on);
                if (drawn > 0) {
                    current_x += drawn;
                } else {
                    // Draw failed, show placeholder and advance
                    current_x += oled_draw_char_color(current_x, current_y, '#', 1, on);
                    current_x += oled_draw_char_color(current_x, current_y, '#', 1, on);
                }
            } else {
                // Font not available or invalid GB2312, show placeholder
                current_x += oled_draw_char_color(current_x, current_y, '*', 1, on);
                current_x += oled_draw_char_color(current_x, current_y, '*', 1, on);
            }
        } else {
            // UTF-8: decode and convert to GB2312
            const uint8_t *before = ptr;
            uint8_t char_hi = 0;
            uint8_t char_lo = 0;
            bool converted = font_utf8_to_gb2312(&ptr, &char_hi, &char_lo);

            if (converted && font_is_chinese_available()) {
                int drawn = oled_draw_chinese_char(current_x, current_y, char_hi, char_lo, on);
                if (drawn > 0) {
                    current_x += drawn;
                } else {
                    // Draw failed, show placeholder
                    current_x += oled_draw_char_color(current_x, current_y, '#', 1, on);
                    current_x += oled_draw_char_color(current_x, current_y, '#', 1, on);
                }
            } else {
                // Conversion failed or font unavailable, fallback to placeholder
                current_x += oled_draw_char_color(current_x, current_y, '?', 1, on);
                if (!converted && ptr == before) {
                    // Avoid stalling if decoder failed without consuming input
                    ptr = before + 1;
                }
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

    // Calculate line height based on scaled font
    int font_size = 16;  // default
    font_get_info(&font_size, NULL);
    int scaled_height = (int)(font_size * s_font_scale);
    if (scaled_height < 8) scaled_height = 8;

    // Add small padding between lines
    int line_spacing = scaled_height + 4;

    if (line1) {
        oled_draw_mixed_string(0, 0, line1, 16, true);
    }

    if (line2) {
        oled_draw_mixed_string(0, line_spacing, line2, 16, true);
    }

    if (line3) {
        oled_draw_mixed_string(0, line_spacing * 2, line3, 16, true);
    }

    oled_update();
    return ESP_OK;
}
