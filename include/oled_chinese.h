/**
 * OLED Chinese Character Display Header
 * Extension to OLED driver for GB2312 Chinese character rendering
 */

#ifndef __OLED_CHINESE_H
#define __OLED_CHINESE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default font scale factor (1.0 = original size, 0.75 = 75% size)
#define OLED_CHINESE_FONT_SCALE_DEFAULT  0.75f

/**
 * Set Chinese font scale factor
 * @param scale Scale factor (0.5 to 1.0, where 1.0 = full 16x16 size)
 *             Example: 0.75 = 12x12 display, 0.5 = 8x8 display
 */
void oled_set_chinese_font_scale(float scale);

/**
 * Get current Chinese font scale factor
 * @return Current scale factor
 */
float oled_get_chinese_font_scale(void);

/**
 * Draw a single GB2312 Chinese character
 * @param x X position on screen
 * @param y Y position on screen
 * @param char_hi First byte of GB2312 character code
 * @param char_lo Second byte of GB2312 character code
 * @param on true=white text, false=black text
 * @return Character width in pixels, 0 if failed
 * @note Currently uses 16pt font (hardcoded)
 */
int oled_draw_chinese_char(int x, int y, uint8_t char_hi, uint8_t char_lo, bool on);

/**
 * Draw a string of GB2312 Chinese characters
 * @param x X position
 * @param y Y position
 * @param str String containing GB2312 encoded Chinese characters
 * @param on true=white text, false=black text
 * @return Width of drawn text in pixels
 */
int oled_draw_chinese_string(int x, int y, const char *str, bool on);

/**
 * Draw mixed ASCII and Chinese text
 * Automatically detects and renders both ASCII and GB2312 characters
 * @param x X position
 * @param y Y position
 * @param str String with mixed ASCII and GB2312 characters
 * @param font_size Font size for Chinese characters (12, 16, 24, 32)
 * @param on true=white text, false=black text
 * @return Width of drawn text in pixels
 */
int oled_draw_mixed_string(int x, int y, const char *str, int font_size, bool on);

/**
 * Show a message with Chinese character support
 * @param line1 First line text (can be NULL)
 * @param line2 Second line text (can be NULL)
 * @param line3 Third line text (can be NULL)
 * @return ESP_OK if Chinese font available, ESP_FAIL otherwise
 */
esp_err_t oled_show_chinese_message(const char *line1, const char *line2, const char *line3);

#ifdef __cplusplus
}
#endif

#endif // __OLED_CHINESE_H
