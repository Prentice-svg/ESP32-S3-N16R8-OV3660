/**
 * LCD Display Header
 */

#ifndef __LCD_H
#define __LCD_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * LCD initialization
 * @param cs_pin Chip select pin
 * @param dc_pin Data/command pin
 * @param rst_pin Reset pin (-1 if not used)
 * @param sck_pin Clock pin
 * @param mosi_pin MOSI pin
 * @return ESP_OK on success
 */
esp_err_t lcd_init(int cs_pin, int dc_pin, int rst_pin, int sck_pin, int mosi_pin);

/**
 * Deinitialize LCD
 */
esp_err_t lcd_deinit(void);

/**
 * Clear display
 */
void lcd_clear(uint16_t color);

/**
 * Draw a pixel
 */
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * Draw a rectangle
 */
void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * Fill a rectangle
 */
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * Draw text
 * @param x X position
 * @param y Y position
 * @param text Text to draw
 * @param color Text color
 * @param bg_color Background color
 */
void lcd_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg_color);

/**
 * Draw a JPEG image from memory
 * @param x X position
 * @param y Y position
 * @param data JPEG data
 * @param len Data length
 */
esp_err_t lcd_draw_jpeg(uint16_t x, uint16_t y, const uint8_t *data, size_t len);

/**
 * Draw camera preview
 * @param fb Frame buffer
 * @param x X position
 * @param y Y position
 * @param w Width
 * @param h Height
 */
esp_err_t lcd_draw_preview(const uint8_t *fb, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/**
 * Set backlight brightness
 * @param brightness 0-255
 */
void lcd_set_brightness(uint8_t brightness);

/**
 * Set rotation
 * @param rotation 0-3
 */
void lcd_set_rotation(uint8_t rotation);

/**
 * Get display width
 * @return Width in pixels
 */
uint16_t lcd_get_width(void);

/**
 * Get display height
 * @return Height in pixels
 */
uint16_t lcd_get_height(void);

#ifdef __cplusplus
}
#endif

#endif // __LCD_H
