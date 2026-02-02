/**
 * OLED Display Driver Header
 * Supports SSD1306/SSD1315 I2C 0.96" 128x64 OLED displays
 */

#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Display dimensions
#define OLED_WIDTH  128
#define OLED_HEIGHT 64

// Default I2C address (0x3C for most modules, some use 0x3D)
#define OLED_I2C_ADDR  0x3C

/**
 * Initialize OLED display
 * @param sda_pin I2C SDA pin
 * @param scl_pin I2C SCL pin
 * @param i2c_addr I2C address (use OLED_I2C_ADDR or 0x3D)
 * @return ESP_OK on success
 */
esp_err_t oled_init(int sda_pin, int scl_pin, uint8_t i2c_addr);

/**
 * Deinitialize OLED
 */
esp_err_t oled_deinit(void);

/**
 * Check if OLED is initialized
 */
bool oled_is_init(void);

/**
 * Clear the display buffer
 */
void oled_clear(void);

/**
 * Update display with buffer content
 * Call this after drawing to show changes
 */
void oled_update(void);

/**
 * Set display contrast
 * @param contrast 0-255
 */
void oled_set_contrast(uint8_t contrast);

/**
 * Turn display on/off
 * @param on true=on, false=off
 */
void oled_display_on(bool on);

/**
 * Invert display colors
 * @param invert true=inverted, false=normal
 */
void oled_invert(bool invert);

/**
 * Set a single pixel
 * @param x X position (0-127)
 * @param y Y position (0-63)
 * @param on true=white, false=black
 */
void oled_set_pixel(int x, int y, bool on);

/**
 * Draw a horizontal line
 */
void oled_draw_hline(int x, int y, int w, bool on);

/**
 * Draw a vertical line
 */
void oled_draw_vline(int x, int y, int h, bool on);

/**
 * Draw a rectangle (outline)
 */
void oled_draw_rect(int x, int y, int w, int h, bool on);

/**
 * Fill a rectangle
 */
void oled_fill_rect(int x, int y, int w, int h, bool on);

/**
 * Draw a single character (8x8 font)
 * @param x X position
 * @param y Y position
 * @param c Character to draw
 * @param size Font size multiplier (1=8x8, 2=16x16, etc.)
 * @return Width of character drawn
 */
int oled_draw_char(int x, int y, char c, int size);

/**
 * Draw a single character with color
 * @param x X position
 * @param y Y position
 * @param c Character to draw
 * @param size Font size multiplier
 * @param on true=white text, false=black text (for inverted bars)
 */
int oled_draw_char_color(int x, int y, char c, int size, bool on);

/**
 * Draw a string
 * @param x X position
 * @param y Y position
 * @param str String to draw
 * @param size Font size multiplier
 */
void oled_draw_string(int x, int y, const char *str, int size);

/**
 * Draw a string with color
 * @param x X position
 * @param y Y position
 * @param str String to draw
 * @param size Font size multiplier
 * @param on true=white text, false=black text
 */
void oled_draw_string_color(int x, int y, const char *str, int size, bool on);

/**
 * Draw a large number (for prominent display)
 * @param x X position
 * @param y Y position
 * @param num Number to display
 * @param digits Number of digits (0=auto)
 */
void oled_draw_number(int x, int y, int num, int digits);

/**
 * Draw progress bar
 * @param x X position
 * @param y Y position
 * @param w Width
 * @param h Height
 * @param percent Progress percentage (0-100)
 */
void oled_draw_progress(int x, int y, int w, int h, int percent);

/**
 * Draw battery icon
 * @param x X position
 * @param y Y position
 * @param percent Battery percentage (0-100)
 * @param charging true if charging
 */
void oled_draw_battery(int x, int y, int percent, bool charging);

/**
 * Draw WiFi signal icon
 * @param x X position
 * @param y Y position
 * @param connected true if connected
 */
void oled_draw_wifi(int x, int y, bool connected);

/**
 * Draw camera icon
 * @param x X position
 * @param y Y position
 * @param recording true if recording/timelapse running
 */
void oled_draw_camera(int x, int y, bool recording);

/**
 * Draw SD card icon
 * @param x X position
 * @param y Y position
 * @param mounted true if SD mounted
 */
void oled_draw_sdcard(int x, int y, bool mounted);

/**
 * Show timelapse status screen
 * @param running Is timelapse running
 * @param current Current shot number
 * @param total Total shots
 * @param interval Interval in seconds
 * @param next_sec Seconds until next shot
 */
void oled_show_timelapse_status(bool running, uint32_t current, uint32_t total,
                                 uint32_t interval, uint32_t next_sec);

/**
 * Show system info screen
 * @param battery_pct Battery percentage
 * @param charging Is charging
 * @param wifi_connected WiFi status
 * @param sd_mounted SD card status
 * @param ip_addr IP address string (can be NULL)
 */
void oled_show_system_info(int battery_pct, bool charging, bool wifi_connected,
                           bool sd_mounted, const char *ip_addr);

/**
 * Show menu screen
 * @param items Array of menu item strings
 * @param count Number of items
 * @param selected Currently selected index
 */
void oled_show_menu(const char **items, int count, int selected);

/**
 * Show a brief message
 * @param line1 First line (can be NULL)
 * @param line2 Second line (can be NULL)
 * @param line3 Third line (can be NULL)
 */
void oled_show_message(const char *line1, const char *line2, const char *line3);

/**
 * Display camera preview (grayscale thumbnail)
 * Decodes JPEG and displays as dithered monochrome image
 * @param jpeg_data JPEG image data
 * @param jpeg_len JPEG data length
 * @return ESP_OK on success
 */
esp_err_t oled_show_preview(const uint8_t *jpeg_data, size_t jpeg_len);

/**
 * Display a grayscale buffer as dithered monochrome
 * @param gray_data Grayscale image data (8-bit per pixel)
 * @param width Image width
 * @param height Image height
 */
void oled_draw_grayscale(const uint8_t *gray_data, int width, int height);

#ifdef __cplusplus
}
#endif

#endif // __OLED_H
