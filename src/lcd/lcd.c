/**
 * LCD Display Implementation
 * Supports ST7789 and similar SPI displays
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "lcd.h"

// Display dimensions (common 2.8" display)
#define LCD_WIDTH  240
#define LCD_HEIGHT 320

// Colors
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_ORANGE      0xFD20

// SPI Configuration
#define LCD_HOST          SPI2_HOST
#define DMA_CHANNEL       2

static const char *TAG = "lcd";

typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} lcd_area_t;

static bool is_init = false;
static spi_device_handle_t spi = NULL;
static uint8_t rotation = 0;
static uint16_t display_width = LCD_WIDTH;
static uint16_t display_height = LCD_HEIGHT;

// Forward declaration
static void lcd_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * Send command to LCD
 */
static void lcd_cmd(const uint8_t cmd)
{
    gpio_set_level(GPIO_NUM_38, 0);  // DC = 0 for command
    spi_transaction_t t = {0};
    t.length = 8;
    t.tx_buffer = &cmd;
    spi_device_transmit(spi, &t);
}

/**
 * Send data to LCD
 */
static void lcd_data(const uint8_t *data, size_t len)
{
    gpio_set_level(GPIO_NUM_38, 1);  // DC = 1 for data
    spi_transaction_t t = {0};
    t.length = len * 8;
    t.tx_buffer = data;
    spi_device_transmit(spi, &t);
}

/**
 * Set LCD address window
 */
static void lcd_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t data[4];

    data[0] = x0 >> 8;
    data[1] = x0 & 0xFF;
    data[2] = x1 >> 8;
    data[3] = x1 & 0xFF;
    lcd_cmd(0x2A);  // Column address set
    lcd_data(data, 4);

    data[0] = y0 >> 8;
    data[1] = y0 & 0xFF;
    data[2] = y1 >> 8;
    data[3] = y1 & 0xFF;
    lcd_cmd(0x2B);  // Row address set
    lcd_data(data, 4);

    lcd_cmd(0x2C);  // Memory write
}

/**
 * Initialize LCD
 */
esp_err_t lcd_init(int cs_pin, int dc_pin, int rst_pin, int sck_pin, int mosi_pin)
{
    if (is_init) {
        ESP_LOGW(TAG, "LCD already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing LCD...");

    // Configure GPIO pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << dc_pin) | (1ULL << rst_pin) | (1ULL << cs_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_level(cs_pin, 0);   // Chip select active low

    // SPI configuration
    spi_bus_config_t buscfg = {
        .mosi_io_num = mosi_pin,
        .miso_io_num = -1,
        .sclk_io_num = sck_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2,
        .flags = SPICOMMON_BUSFLAG_MASTER
    };

    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .duty_cycle_pos = 128,
        .cs_ena_pretrans = 1,
        .cs_ena_posttrans = 1,
        .clock_speed_hz = 40 * 1000 * 1000,
        .spics_io_num = cs_pin,
        .queue_size = 7
    };

    esp_err_t ret = spi_bus_initialize(LCD_HOST, &buscfg, DMA_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = spi_bus_add_device(LCD_HOST, &devcfg, &spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI device add failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Reset display
    if (rst_pin >= 0) {
        gpio_set_level(rst_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(rst_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Initialize ST7789
    lcd_cmd(0x01);  // Software reset
    vTaskDelay(pdMS_TO_TICKS(150));

    lcd_cmd(0x11);  // Sleep out
    vTaskDelay(pdMS_TO_TICKS(500));

    lcd_cmd(0x36);  // Memory data access control
    lcd_data((uint8_t[]){0x00}, 1);

    lcd_cmd(0x3A);  // Interface pixel format
    lcd_data((uint8_t[]){0x55}, 1);  // 16-bit RGB

    lcd_cmd(0xB2);  // Porch setting
    lcd_data((uint8_t[]){0x0C, 0x0C, 0x00, 0x33, 0x33}, 5);

    lcd_cmd(0xB7);  // Gate control
    lcd_data((uint8_t[]){0x35}, 1);

    lcd_cmd(0xBB);  // VCOM setting
    lcd_data((uint8_t[]){0x19}, 1);

    lcd_cmd(0xC0);  // LCM control
    lcd_data((uint8_t[]){0x2C}, 1);

    lcd_cmd(0xC2);  // VDV and VRH command enable
    lcd_data((uint8_t[]){0x01}, 1);

    lcd_cmd(0xC3);  // VRH set
    lcd_data((uint8_t[]){0x12}, 1);

    lcd_cmd(0xC4);  // VDV set
    lcd_data((uint8_t[]){0x20}, 1);

    lcd_cmd(0xC6);  // Frame rate control
    lcd_data((uint8_t[]){0x0F}, 1);

    lcd_cmd(0xD0);  // Power control
    lcd_data((uint8_t[]){0xA4, 0xA1}, 2);

    lcd_cmd(0xE0);  // Positive voltage gamma control
    lcd_data((uint8_t[]){0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23}, 14);

    lcd_cmd(0xE1);  // Negative voltage gamma control
    lcd_data((uint8_t[]){0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23}, 14);

    lcd_cmd(0x21);  // Display inversion on

    lcd_cmd(0x29);  // Display on

    lcd_cmd(0x2C);  // Memory write

    is_init = true;
    ESP_LOGI(TAG, "LCD initialized successfully");

    // Clear to black
    lcd_clear(COLOR_BLACK);

    return ESP_OK;
}

/**
 * Deinitialize LCD
 */
esp_err_t lcd_deinit(void)
{
    if (!is_init) return ESP_OK;

    spi_bus_remove_device(spi);
    spi_bus_free(LCD_HOST);
    is_init = false;

    return ESP_OK;
}

/**
 * Clear display
 */
void lcd_clear(uint16_t color)
{
    lcd_fill_rect(0, 0, display_width, display_height, color);
}

/**
 * Draw a rectangle outline
 */
void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    lcd_draw_line(x, y, x + w - 1, y, color);
    lcd_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    lcd_draw_line(x, y, x, y + h - 1, color);
    lcd_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
}

/**
 * Fill a rectangle
 */
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (w == 0 || h == 0) return;

    // Clip to display bounds
    if (x >= display_width || y >= display_height) return;
    if (x + w > display_width) w = display_width - x;
    if (y + h > display_height) h = display_height - y;

    lcd_set_address_window(x, y, x + w - 1, y + h - 1);

    // Allocate buffer for pixel data
    uint16_t *pixels = (uint16_t *)malloc(w * sizeof(uint16_t));
    if (pixels == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for fill");
        return;
    }

    // Fill buffer with color (RGB565)
    for (uint16_t i = 0; i < w; i++) {
        pixels[i] = color;
    }

    gpio_set_level(GPIO_NUM_38, 1);  // DC = 1 for data

    // Send data row by row
    for (uint16_t row = 0; row < h; row++) {
        spi_transaction_t t = {0};
        t.length = w * 2 * 8;  // w pixels * 2 bytes per pixel * 8 bits
        t.tx_buffer = pixels;
        spi_device_transmit(spi, &t);
    }

    free(pixels);
}

/**
 * Draw a pixel (placeholder - use fill_rect for single pixel)
 */
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_fill_rect(x, y, 1, 1, color);
}

/**
 * Draw text (placeholder - would need font library)
 */
void lcd_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg_color)
{
    // Basic placeholder - would implement with font rendering
    ESP_LOGW(TAG, "Text drawing not fully implemented");
}

/**
 * Draw camera preview (placeholder - would need JPEG decoder)
 */
esp_err_t lcd_draw_preview(const uint8_t *fb, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    // Basic placeholder - would implement with image scaling
    ESP_LOGW(TAG, "Preview drawing not fully implemented");
    return ESP_ERR_NOT_SUPPORTED;
}

/**
 * Set backlight brightness (requires PWM)
 */
void lcd_set_brightness(uint8_t brightness)
{
    // Placeholder - would need LEDC PWM
    ESP_LOGI(TAG, "Brightness setting not implemented");
}

/**
 * Set rotation
 */
void lcd_set_rotation(uint8_t rot)
{
    rotation = rot % 4;

    uint8_t madctl = 0;
    switch (rotation) {
        case 0:  madctl = 0x00; display_width = LCD_WIDTH; display_height = LCD_HEIGHT; break;
        case 1:  madctl = 0x60; display_width = LCD_HEIGHT; display_height = LCD_WIDTH; break;
        case 2:  madctl = 0xC0; display_width = LCD_WIDTH; display_height = LCD_HEIGHT; break;
        case 3:  madctl = 0xA0; display_width = LCD_HEIGHT; display_height = LCD_WIDTH; break;
    }

    lcd_cmd(0x36);  // Memory data access control
    lcd_data(&madctl, 1);
}

/**
 * Get display width
 */
uint16_t lcd_get_width(void)
{
    return display_width;
}

/**
 * Get display height
 */
uint16_t lcd_get_height(void)
{
    return display_height;
}

/**
 * Draw a line using Bresenham's algorithm
 */
static void lcd_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;

    while (1) {
        lcd_draw_pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1) break;

        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// JPEG drawing placeholder
esp_err_t lcd_draw_jpeg(uint16_t x, uint16_t y, const uint8_t *data, size_t len)
{
    ESP_LOGW(TAG, "JPEG drawing not implemented - requires JPEG decoder");
    return ESP_ERR_NOT_SUPPORTED;
}
