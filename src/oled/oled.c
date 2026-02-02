/**
 * OLED Display Driver Implementation
 * SSD1306/SSD1315 I2C 0.96" 128x64 OLED
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/i2c.h"
#include "jpeg_decoder.h"
#include "oled.h"
#include "font.h"
#include "oled_chinese.h"

static const char *TAG = "oled";

// I2C configuration
#define I2C_MASTER_NUM     I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 400000
#define I2C_TIMEOUT_MS     100

// SSD1306 Commands
#define SSD1306_DISPLAYOFF          0xAE
#define SSD1306_DISPLAYON           0xAF
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5
#define SSD1306_SETMULTIPLEX        0xA8
#define SSD1306_SETDISPLAYOFFSET    0xD3
#define SSD1306_SETSTARTLINE        0x40
#define SSD1306_CHARGEPUMP          0x8D
#define SSD1306_MEMORYMODE          0x20
#define SSD1306_SEGREMAP            0xA0
#define SSD1306_COMSCANDEC          0xC8
#define SSD1306_SETCOMPINS          0xDA
#define SSD1306_SETCONTRAST         0x81
#define SSD1306_SETPRECHARGE        0xD9
#define SSD1306_SETVCOMDETECT       0xDB
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_NORMALDISPLAY       0xA6
#define SSD1306_INVERTDISPLAY       0xA7
#define SSD1306_COLUMNADDR          0x21
#define SSD1306_PAGEADDR            0x22

// Display state
static bool is_init = false;
static uint8_t i2c_addr = OLED_I2C_ADDR;
static uint8_t display_buffer[OLED_WIDTH * OLED_HEIGHT / 8];

// Basic 8x8 font (ASCII 32-127)
static const uint8_t font_8x8[][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x06, 0x5F, 0x5F, 0x06, 0x00, 0x00}, // !
    {0x00, 0x03, 0x03, 0x00, 0x03, 0x03, 0x00, 0x00}, // "
    {0x14, 0x7F, 0x7F, 0x14, 0x7F, 0x7F, 0x14, 0x00}, // #
    {0x24, 0x2E, 0x6B, 0x6B, 0x3A, 0x12, 0x00, 0x00}, // $
    {0x46, 0x66, 0x30, 0x18, 0x0C, 0x66, 0x62, 0x00}, // %
    {0x30, 0x7A, 0x4F, 0x5D, 0x37, 0x7A, 0x48, 0x00}, // &
    {0x00, 0x04, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x3E, 0x63, 0x41, 0x00, 0x00, 0x00}, // (
    {0x00, 0x41, 0x63, 0x3E, 0x1C, 0x00, 0x00, 0x00}, // )
    {0x08, 0x2A, 0x3E, 0x1C, 0x3E, 0x2A, 0x08, 0x00}, // *
    {0x08, 0x08, 0x3E, 0x3E, 0x08, 0x08, 0x00, 0x00}, // +
    {0x00, 0x80, 0xE0, 0x60, 0x00, 0x00, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00}, // -
    {0x00, 0x00, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00}, // .
    {0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00}, // /
    {0x3E, 0x7F, 0x71, 0x59, 0x4D, 0x7F, 0x3E, 0x00}, // 0
    {0x40, 0x42, 0x7F, 0x7F, 0x40, 0x40, 0x00, 0x00}, // 1
    {0x62, 0x73, 0x59, 0x49, 0x6F, 0x66, 0x00, 0x00}, // 2
    {0x22, 0x63, 0x49, 0x49, 0x7F, 0x36, 0x00, 0x00}, // 3
    {0x18, 0x1C, 0x16, 0x53, 0x7F, 0x7F, 0x50, 0x00}, // 4
    {0x27, 0x67, 0x45, 0x45, 0x7D, 0x39, 0x00, 0x00}, // 5
    {0x3C, 0x7E, 0x4B, 0x49, 0x79, 0x30, 0x00, 0x00}, // 6
    {0x03, 0x03, 0x71, 0x79, 0x0F, 0x07, 0x00, 0x00}, // 7
    {0x36, 0x7F, 0x49, 0x49, 0x7F, 0x36, 0x00, 0x00}, // 8
    {0x06, 0x4F, 0x49, 0x69, 0x3F, 0x1E, 0x00, 0x00}, // 9
    {0x00, 0x00, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00}, // :
    {0x00, 0x80, 0xE6, 0x66, 0x00, 0x00, 0x00, 0x00}, // ;
    {0x08, 0x1C, 0x36, 0x63, 0x41, 0x00, 0x00, 0x00}, // <
    {0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x00, 0x00}, // =
    {0x00, 0x41, 0x63, 0x36, 0x1C, 0x08, 0x00, 0x00}, // >
    {0x02, 0x03, 0x51, 0x59, 0x0F, 0x06, 0x00, 0x00}, // ?
    {0x3E, 0x7F, 0x41, 0x5D, 0x5D, 0x1F, 0x1E, 0x00}, // @
    {0x7C, 0x7E, 0x13, 0x13, 0x7E, 0x7C, 0x00, 0x00}, // A
    {0x41, 0x7F, 0x7F, 0x49, 0x49, 0x7F, 0x36, 0x00}, // B
    {0x1C, 0x3E, 0x63, 0x41, 0x41, 0x63, 0x22, 0x00}, // C
    {0x41, 0x7F, 0x7F, 0x41, 0x63, 0x3E, 0x1C, 0x00}, // D
    {0x41, 0x7F, 0x7F, 0x49, 0x5D, 0x41, 0x63, 0x00}, // E
    {0x41, 0x7F, 0x7F, 0x49, 0x1D, 0x01, 0x03, 0x00}, // F
    {0x1C, 0x3E, 0x63, 0x41, 0x51, 0x73, 0x72, 0x00}, // G
    {0x7F, 0x7F, 0x08, 0x08, 0x7F, 0x7F, 0x00, 0x00}, // H
    {0x00, 0x41, 0x7F, 0x7F, 0x41, 0x00, 0x00, 0x00}, // I
    {0x30, 0x70, 0x40, 0x41, 0x7F, 0x3F, 0x01, 0x00}, // J
    {0x41, 0x7F, 0x7F, 0x08, 0x1C, 0x77, 0x63, 0x00}, // K
    {0x41, 0x7F, 0x7F, 0x41, 0x40, 0x60, 0x70, 0x00}, // L
    {0x7F, 0x7F, 0x0E, 0x1C, 0x0E, 0x7F, 0x7F, 0x00}, // M
    {0x7F, 0x7F, 0x06, 0x0C, 0x18, 0x7F, 0x7F, 0x00}, // N
    {0x1C, 0x3E, 0x63, 0x41, 0x63, 0x3E, 0x1C, 0x00}, // O
    {0x41, 0x7F, 0x7F, 0x49, 0x09, 0x0F, 0x06, 0x00}, // P
    {0x1E, 0x3F, 0x21, 0x71, 0x7F, 0x5E, 0x00, 0x00}, // Q
    {0x41, 0x7F, 0x7F, 0x09, 0x19, 0x7F, 0x66, 0x00}, // R
    {0x26, 0x6F, 0x4D, 0x59, 0x73, 0x32, 0x00, 0x00}, // S
    {0x03, 0x41, 0x7F, 0x7F, 0x41, 0x03, 0x00, 0x00}, // T
    {0x7F, 0x7F, 0x40, 0x40, 0x7F, 0x7F, 0x00, 0x00}, // U
    {0x1F, 0x3F, 0x60, 0x60, 0x3F, 0x1F, 0x00, 0x00}, // V
    {0x7F, 0x7F, 0x30, 0x18, 0x30, 0x7F, 0x7F, 0x00}, // W
    {0x43, 0x67, 0x3C, 0x18, 0x3C, 0x67, 0x43, 0x00}, // X
    {0x07, 0x4F, 0x78, 0x78, 0x4F, 0x07, 0x00, 0x00}, // Y
    {0x47, 0x63, 0x71, 0x59, 0x4D, 0x67, 0x73, 0x00}, // Z
    {0x00, 0x7F, 0x7F, 0x41, 0x41, 0x00, 0x00, 0x00}, // [
    {0x01, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00}, // backslash
    {0x00, 0x41, 0x41, 0x7F, 0x7F, 0x00, 0x00, 0x00}, // ]
    {0x08, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x08, 0x00}, // ^
    {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, // _
    {0x00, 0x00, 0x03, 0x07, 0x04, 0x00, 0x00, 0x00}, // `
    {0x20, 0x74, 0x54, 0x54, 0x3C, 0x78, 0x40, 0x00}, // a
    {0x41, 0x7F, 0x3F, 0x48, 0x48, 0x78, 0x30, 0x00}, // b
    {0x38, 0x7C, 0x44, 0x44, 0x6C, 0x28, 0x00, 0x00}, // c
    {0x30, 0x78, 0x48, 0x49, 0x3F, 0x7F, 0x40, 0x00}, // d
    {0x38, 0x7C, 0x54, 0x54, 0x5C, 0x18, 0x00, 0x00}, // e
    {0x48, 0x7E, 0x7F, 0x49, 0x03, 0x02, 0x00, 0x00}, // f
    {0x98, 0xBC, 0xA4, 0xA4, 0xF8, 0x7C, 0x04, 0x00}, // g
    {0x41, 0x7F, 0x7F, 0x08, 0x04, 0x7C, 0x78, 0x00}, // h
    {0x00, 0x44, 0x7D, 0x7D, 0x40, 0x00, 0x00, 0x00}, // i
    {0x60, 0xE0, 0x80, 0x80, 0xFD, 0x7D, 0x00, 0x00}, // j
    {0x41, 0x7F, 0x7F, 0x10, 0x38, 0x6C, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7F, 0x7F, 0x40, 0x00, 0x00, 0x00}, // l
    {0x7C, 0x7C, 0x18, 0x38, 0x1C, 0x7C, 0x78, 0x00}, // m
    {0x7C, 0x7C, 0x04, 0x04, 0x7C, 0x78, 0x00, 0x00}, // n
    {0x38, 0x7C, 0x44, 0x44, 0x7C, 0x38, 0x00, 0x00}, // o
    {0x84, 0xFC, 0xF8, 0xA4, 0x24, 0x3C, 0x18, 0x00}, // p
    {0x18, 0x3C, 0x24, 0xA4, 0xF8, 0xFC, 0x84, 0x00}, // q
    {0x44, 0x7C, 0x78, 0x4C, 0x04, 0x1C, 0x18, 0x00}, // r
    {0x48, 0x5C, 0x54, 0x54, 0x74, 0x24, 0x00, 0x00}, // s
    {0x00, 0x04, 0x3E, 0x7F, 0x44, 0x24, 0x00, 0x00}, // t
    {0x3C, 0x7C, 0x40, 0x40, 0x3C, 0x7C, 0x40, 0x00}, // u
    {0x1C, 0x3C, 0x60, 0x60, 0x3C, 0x1C, 0x00, 0x00}, // v
    {0x3C, 0x7C, 0x70, 0x38, 0x70, 0x7C, 0x3C, 0x00}, // w
    {0x44, 0x6C, 0x38, 0x10, 0x38, 0x6C, 0x44, 0x00}, // x
    {0x9C, 0xBC, 0xA0, 0xA0, 0xFC, 0x7C, 0x00, 0x00}, // y
    {0x4C, 0x64, 0x74, 0x5C, 0x4C, 0x64, 0x00, 0x00}, // z
    {0x08, 0x08, 0x3E, 0x77, 0x41, 0x41, 0x00, 0x00}, // {
    {0x00, 0x00, 0x00, 0x77, 0x77, 0x00, 0x00, 0x00}, // |
    {0x41, 0x41, 0x77, 0x3E, 0x08, 0x08, 0x00, 0x00}, // }
    {0x02, 0x03, 0x01, 0x03, 0x02, 0x03, 0x01, 0x00}, // ~
};

/**
 * Write command to SSD1306
 */
static esp_err_t oled_write_cmd(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};  // Co=0, D/C#=0 (command)
    return i2c_master_write_to_device(I2C_MASTER_NUM, i2c_addr, data, 2,
                                       pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

/**
 * Write data to SSD1306
 */
static esp_err_t oled_write_data(const uint8_t *data, size_t len)
{
    uint8_t *buffer = malloc(len + 1);
    if (!buffer) return ESP_ERR_NO_MEM;
    
    buffer[0] = 0x40;  // Co=0, D/C#=1 (data)
    memcpy(buffer + 1, data, len);
    
    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, i2c_addr, buffer, len + 1,
                                                pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    free(buffer);
    return ret;
}

esp_err_t oled_init(int sda_pin, int scl_pin, uint8_t addr)
{
    if (is_init) {
        ESP_LOGW(TAG, "OLED already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing OLED (SDA=%d, SCL=%d, Addr=0x%02X)", sda_pin, scl_pin, addr);
    i2c_addr = addr;

    // Initialize I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Test I2C communication
    uint8_t test = 0;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, i2c_addr, &test, 0,
                                      pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OLED not found at address 0x%02X", i2c_addr);
        i2c_driver_delete(I2C_MASTER_NUM);
        return ESP_ERR_NOT_FOUND;
    }

    // Initialize SSD1306/SSD1315
    // Turn off display
    oled_write_cmd(SSD1306_DISPLAYOFF);
    
    // Set display clock
    oled_write_cmd(SSD1306_SETDISPLAYCLOCKDIV);
    oled_write_cmd(0x80);
    
    // Set multiplex ratio
    oled_write_cmd(SSD1306_SETMULTIPLEX);
    oled_write_cmd(0x3F);  // 64-1
    
    // Set display offset
    oled_write_cmd(SSD1306_SETDISPLAYOFFSET);
    oled_write_cmd(0x00);
    
    // Set start line
    oled_write_cmd(SSD1306_SETSTARTLINE | 0x00);
    
    // Enable charge pump
    oled_write_cmd(SSD1306_CHARGEPUMP);
    oled_write_cmd(0x14);  // Enable
    
    // Set memory mode to horizontal
    oled_write_cmd(SSD1306_MEMORYMODE);
    oled_write_cmd(0x00);
    
    // Set segment re-map
    oled_write_cmd(SSD1306_SEGREMAP | 0x01);
    
    // Set COM output scan direction
    oled_write_cmd(SSD1306_COMSCANDEC);
    
    // Set COM pins
    oled_write_cmd(SSD1306_SETCOMPINS);
    oled_write_cmd(0x12);
    
    // Set contrast
    oled_write_cmd(SSD1306_SETCONTRAST);
    oled_write_cmd(0xCF);
    
    // Set pre-charge period
    oled_write_cmd(SSD1306_SETPRECHARGE);
    oled_write_cmd(0xF1);
    
    // Set VCOMH deselect level
    oled_write_cmd(SSD1306_SETVCOMDETECT);
    oled_write_cmd(0x40);
    
    // Resume display
    oled_write_cmd(SSD1306_DISPLAYALLON_RESUME);
    
    // Normal display
    oled_write_cmd(SSD1306_NORMALDISPLAY);
    
    // Turn on display
    oled_write_cmd(SSD1306_DISPLAYON);
    
    // Clear buffer and display
    memset(display_buffer, 0, sizeof(display_buffer));
    oled_update();
    
    is_init = true;
    ESP_LOGI(TAG, "OLED initialized successfully");
    return ESP_OK;
}

esp_err_t oled_deinit(void)
{
    if (!is_init) return ESP_OK;
    
    oled_display_on(false);
    i2c_driver_delete(I2C_MASTER_NUM);
    is_init = false;
    return ESP_OK;
}

bool oled_is_init(void)
{
    return is_init;
}

void oled_clear(void)
{
    memset(display_buffer, 0, sizeof(display_buffer));
}

void oled_update(void)
{
    if (!is_init) return;
    
    // Set column address
    oled_write_cmd(SSD1306_COLUMNADDR);
    oled_write_cmd(0);
    oled_write_cmd(OLED_WIDTH - 1);
    
    // Set page address
    oled_write_cmd(SSD1306_PAGEADDR);
    oled_write_cmd(0);
    oled_write_cmd((OLED_HEIGHT / 8) - 1);
    
    // Write buffer
    oled_write_data(display_buffer, sizeof(display_buffer));
}

void oled_set_contrast(uint8_t contrast)
{
    if (!is_init) return;
    oled_write_cmd(SSD1306_SETCONTRAST);
    oled_write_cmd(contrast);
}

void oled_display_on(bool on)
{
    if (!is_init) return;
    oled_write_cmd(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
}

void oled_invert(bool invert)
{
    if (!is_init) return;
    oled_write_cmd(invert ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
}

void oled_set_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    
    int idx = x + (y / 8) * OLED_WIDTH;
    if (on) {
        display_buffer[idx] |= (1 << (y & 7));
    } else {
        display_buffer[idx] &= ~(1 << (y & 7));
    }
}

void oled_draw_hline(int x, int y, int w, bool on)
{
    for (int i = 0; i < w; i++) {
        oled_set_pixel(x + i, y, on);
    }
}

void oled_draw_vline(int x, int y, int h, bool on)
{
    for (int i = 0; i < h; i++) {
        oled_set_pixel(x, y + i, on);
    }
}

void oled_draw_rect(int x, int y, int w, int h, bool on)
{
    oled_draw_hline(x, y, w, on);
    oled_draw_hline(x, y + h - 1, w, on);
    oled_draw_vline(x, y, h, on);
    oled_draw_vline(x + w - 1, y, h, on);
}

void oled_fill_rect(int x, int y, int w, int h, bool on)
{
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            oled_set_pixel(x + i, y + j, on);
        }
    }
}

int oled_draw_char(int x, int y, char c, int size)
{
    if (c < 32 || c > 126) c = '?';
    
    const uint8_t *glyph = font_8x8[c - 32];
    
    for (int col = 0; col < 8; col++) {
        uint8_t line = glyph[col];
        for (int row = 0; row < 8; row++) {
            if (line & (1 << row)) {
                if (size == 1) {
                    oled_set_pixel(x + col, y + row, true);
                } else {
                    oled_fill_rect(x + col * size, y + row * size, size, size, true);
                }
            }
        }
    }
    
    return 8 * size;
}

int oled_draw_char_color(int x, int y, char c, int size, bool on)
{
    if (c < 32 || c > 126) c = '?';

    const uint8_t *glyph = font_8x8[c - 32];

    for (int col = 0; col < 8; col++) {
        uint8_t line = glyph[col];
        for (int row = 0; row < 8; row++) {
            if (line & (1 << row)) {
                if (size == 1) {
                    oled_set_pixel(x + col, y + row, on);
                } else {
                    oled_fill_rect(x + col * size, y + row * size, size, size, on);
                }
            }
        }
    }

    return 8 * size;
}

void oled_draw_string(int x, int y, const char *str, int size)
{
    int orig_x = x;
    while (*str) {
        if (*str == '\n') {
            x = orig_x;
            y += 8 * size;
        } else {
            x += oled_draw_char(x, y, *str, size);
        }
        str++;
    }
}

void oled_draw_string_color(int x, int y, const char *str, int size, bool on)
{
    int orig_x = x;
    while (*str) {
        if (*str == '\n') {
            x = orig_x;
            y += 8 * size;
        } else {
            x += oled_draw_char_color(x, y, *str, size, on);
        }
        str++;
    }
}

void oled_draw_number(int x, int y, int num, int digits)
{
    char buf[16];
    if (digits > 0) {
        snprintf(buf, sizeof(buf), "%0*d", digits, num);
    } else {
        snprintf(buf, sizeof(buf), "%d", num);
    }
    oled_draw_string(x, y, buf, 2);  // Size 2 for larger display
}

void oled_draw_progress(int x, int y, int w, int h, int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    
    // Draw border
    oled_draw_rect(x, y, w, h, true);
    
    // Draw fill
    int fill_w = (w - 4) * percent / 100;
    if (fill_w > 0) {
        oled_fill_rect(x + 2, y + 2, fill_w, h - 4, true);
    }
}

void oled_draw_battery(int x, int y, int percent, bool charging)
{
    // Battery body (16x8)
    oled_draw_rect(x, y + 1, 14, 6, true);
    // Battery tip
    oled_fill_rect(x + 14, y + 2, 2, 4, true);
    
    // Fill level
    int fill = percent * 10 / 100;
    if (fill > 0) {
        oled_fill_rect(x + 2, y + 3, fill, 2, true);
    }
    
    // Charging indicator
    if (charging) {
        // Draw lightning bolt
        oled_set_pixel(x + 7, y, true);
        oled_set_pixel(x + 6, y + 1, true);
        oled_set_pixel(x + 5, y + 2, true);
        oled_set_pixel(x + 6, y + 3, true);
        oled_set_pixel(x + 7, y + 4, true);
        oled_set_pixel(x + 8, y + 5, true);
        oled_set_pixel(x + 9, y + 6, true);
    }
}

void oled_draw_wifi(int x, int y, bool connected)
{
    if (connected) {
        // WiFi arcs
        oled_set_pixel(x + 4, y, true);
        oled_draw_hline(x + 2, y + 2, 5, true);
        oled_draw_hline(x, y + 4, 9, true);
        oled_set_pixel(x + 4, y + 6, true);
    } else {
        // X mark
        oled_set_pixel(x + 1, y + 1, true);
        oled_set_pixel(x + 7, y + 1, true);
        oled_set_pixel(x + 2, y + 2, true);
        oled_set_pixel(x + 6, y + 2, true);
        oled_set_pixel(x + 3, y + 3, true);
        oled_set_pixel(x + 5, y + 3, true);
        oled_set_pixel(x + 4, y + 4, true);
        oled_set_pixel(x + 3, y + 5, true);
        oled_set_pixel(x + 5, y + 5, true);
        oled_set_pixel(x + 2, y + 6, true);
        oled_set_pixel(x + 6, y + 6, true);
    }
}

void oled_draw_camera(int x, int y, bool recording)
{
    // Camera body
    oled_draw_rect(x, y + 2, 12, 6, true);
    // Lens
    oled_fill_rect(x + 3, y + 3, 4, 4, true);
    // Viewfinder
    oled_fill_rect(x + 8, y, 3, 3, true);
    
    // Recording dot
    if (recording) {
        oled_fill_rect(x + 13, y + 3, 3, 3, true);
    }
}

void oled_draw_sdcard(int x, int y, bool mounted)
{
    // SD card shape
    oled_draw_vline(x, y + 2, 6, true);
    oled_draw_vline(x + 8, y, 8, true);
    oled_draw_hline(x, y + 7, 9, true);
    oled_draw_hline(x + 2, y, 7, true);
    oled_set_pixel(x + 1, y + 1, true);
    oled_set_pixel(x, y + 2, true);
    
    if (mounted) {
        // Checkmark
        oled_set_pixel(x + 2, y + 4, true);
        oled_set_pixel(x + 3, y + 5, true);
        oled_set_pixel(x + 4, y + 4, true);
        oled_set_pixel(x + 5, y + 3, true);
        oled_set_pixel(x + 6, y + 2, true);
    } else {
        // X mark
        oled_set_pixel(x + 2, y + 2, true);
        oled_set_pixel(x + 6, y + 2, true);
        oled_set_pixel(x + 3, y + 3, true);
        oled_set_pixel(x + 5, y + 3, true);
        oled_set_pixel(x + 4, y + 4, true);
        oled_set_pixel(x + 3, y + 5, true);
        oled_set_pixel(x + 5, y + 5, true);
    }
}

void oled_show_timelapse_status(bool running, uint32_t current, uint32_t total,
                                 uint32_t interval, uint32_t next_sec)
{
    oled_clear();

    // Title bar
    oled_draw_string(0, 0, "TIMELAPSE", 1);
    oled_draw_hline(0, 10, 128, true);

    // Status
    if (running) {
        oled_draw_string(100, 0, "RUN", 1);
        oled_draw_camera(85, 0, true);
    } else {
        oled_draw_string(92, 0, "STOP", 1);
    }

    // Progress
    char buf[32];
    snprintf(buf, sizeof(buf), "%lu/%lu", (unsigned long)current, (unsigned long)total);
    oled_draw_string(0, 16, buf, 2);

    // Progress bar
    int percent = total > 0 ? (current * 100 / total) : 0;
    oled_draw_progress(0, 38, 128, 10, percent);

    // Next shot countdown
    if (running && next_sec > 0) {
        snprintf(buf, sizeof(buf), "Next: %lus", (unsigned long)next_sec);
        oled_draw_string(0, 52, buf, 1);
    }

    // Interval
    snprintf(buf, sizeof(buf), "Int: %lus", (unsigned long)interval);
    oled_draw_string(72, 52, buf, 1);

    oled_update();
}

void oled_show_system_info(int battery_pct, bool charging, bool wifi_connected,
                           bool sd_mounted, const char *ip_addr)
{
    oled_clear();

    // Title
    oled_draw_string(0, 0, "SYSTEM INFO", 1);
    oled_draw_hline(0, 10, 128, true);

    // Icons row
    oled_draw_battery(0, 16, battery_pct, charging);
    oled_draw_wifi(20, 16, wifi_connected);
    oled_draw_sdcard(36, 16, sd_mounted);

    // Battery percentage
    char buf[32];
    snprintf(buf, sizeof(buf), "%d%%", battery_pct);
    oled_draw_string(50, 16, buf, 1);

    // IP address
    if (ip_addr && wifi_connected) {
        oled_draw_string(0, 32, "IP:", 1);
        oled_draw_string(24, 32, ip_addr, 1);
    }

    // Menu hint
    oled_draw_string(0, 52, "K1:Menu K2:Start K3:Stop", 1);

    oled_update();
}

void oled_show_menu(const char **items, int count, int selected)
{
    if (!items || count <= 0) return;

    oled_clear();

    // Check if menu items contain Chinese characters (non-ASCII)
    bool has_chinese = false;
    if (items[0]) {
        const unsigned char *s = (unsigned char *)items[0];
        while (*s) {
            if (*s >= 0x80) {  // UTF-8 extended character (likely Chinese)
                has_chinese = true;
                break;
            }
            s++;
        }
    }

    if (has_chinese) {
        // Chinese menu display - adapted for 16x16 Chinese characters
        // Title bar with divider
        oled_draw_string(0, 0, "MENU", 1);
        oled_draw_hline(0, 10, 128, true);

        // Calculate visible items (max 3 for Chinese with 16x16 font)
        int start = 0;
        int visible = 3;
        if (selected >= visible) {
            start = selected - visible + 1;
        }

        // Draw Chinese menu items with proper spacing for 16x16 characters
        // Each Chinese character is 16x16, so we use larger spacing
        for (int i = 0; i < visible && (start + i) < count; i++) {
            int y = 14 + i * 18;  // 18-pixel spacing for Chinese items
            bool is_sel = (start + i) == selected;

            if (is_sel) {
                oled_fill_rect(0, y - 1, 128, 18, true);  // highlight bar
                // Draw Chinese text in black on white bar
                if (font_is_chinese_available()) {
                    oled_draw_chinese_string(6, y, items[start + i], true);
                } else {
                    oled_draw_string_color(6, y, items[start + i], 1, false);
                }
            } else {
                // Draw Chinese text normally
                if (font_is_chinese_available()) {
                    oled_draw_chinese_string(6, y, items[start + i], false);
                } else {
                    oled_draw_string_color(6, y, items[start + i], 1, true);
                }
            }

            // Left indicator bar for selected
            if (is_sel) {
                oled_fill_rect(0, y - 1, 3, 18, false);
            }
        }

        // Scroll indicator on right side
        if (count > visible) {
            int bar_height = 8;
            int rail_height = visible * 18;
            int top = 14;
            int pos = (rail_height - bar_height) * selected / (count - 1);
            oled_draw_rect(123, top, 4, rail_height, true);
            oled_fill_rect(124, top + pos, 2, bar_height, true);
        }
    } else {
        // ASCII menu display - original code
        // Title bar with divider
        oled_draw_string(0, 0, "MENU", 1);
        oled_draw_hline(0, 10, 128, true);

        // Calculate visible items (max 5 rows for ASCII)
        int start = 0;
        int visible = 5;
        if (selected >= visible) {
            start = selected - visible + 1;
        }

        // Draw items with highlighted bar and inverted text
        for (int i = 0; i < visible && (start + i) < count; i++) {
            int y = 14 + i * 10;
            bool is_sel = (start + i) == selected;

            if (is_sel) {
                oled_fill_rect(0, y - 1, 128, 10, true);  // highlight bar
                oled_draw_string_color(6, y, items[start + i], 1, false); // black text on white bar
            } else {
                oled_draw_string_color(6, y, items[start + i], 1, true);  // normal white text
            }

            // Left indicator bar for selected
            if (is_sel) {
                oled_fill_rect(0, y - 1, 3, 10, false); // thin dark stripe for contrast
            }
        }

        // Scroll indicator on right side
        if (count > visible) {
            int bar_height = 8;
            int rail_height = visible * 10;
            int top = 14;
            int pos = (rail_height - bar_height) * selected / (count - 1);
            oled_draw_rect(123, top, 4, rail_height, true);
            oled_fill_rect(124, top + pos, 2, bar_height, true);
        }
    }

    oled_update();
}

void oled_show_message(const char *line1, const char *line2, const char *line3)
{
    oled_clear();
    
    int y = 16;
    if (line1) {
        oled_draw_string(0, y, line1, 1);
        y += 16;
    }
    if (line2) {
        oled_draw_string(0, y, line2, 1);
        y += 16;
    }
    if (line3) {
        oled_draw_string(0, y, line3, 1);
    }
    
    oled_update();
}

/**
 * Floyd-Steinberg dithering for grayscale to monochrome conversion
 * Produces better visual quality than simple thresholding
 */
void oled_draw_grayscale(const uint8_t *gray_data, int width, int height)
{
    if (!gray_data || width <= 0 || height <= 0) return;
    
    // Allocate error buffer for dithering
    int16_t *errors = heap_caps_calloc((width + 2) * 2, sizeof(int16_t), MALLOC_CAP_DEFAULT);
    if (!errors) {
        ESP_LOGE(TAG, "Failed to allocate dithering buffer");
        return;
    }
    
    int16_t *curr_row = errors;
    int16_t *next_row = errors + width + 2;
    
    // Calculate scaling factors
    float scale_x = (float)width / OLED_WIDTH;
    float scale_y = (float)height / OLED_HEIGHT;
    
    oled_clear();
    
    for (int oy = 0; oy < OLED_HEIGHT; oy++) {
        // Swap error rows
        int16_t *temp = curr_row;
        curr_row = next_row;
        next_row = temp;
        memset(next_row, 0, (width + 2) * sizeof(int16_t));
        
        int src_y = (int)(oy * scale_y);
        if (src_y >= height) src_y = height - 1;
        
        for (int ox = 0; ox < OLED_WIDTH; ox++) {
            int src_x = (int)(ox * scale_x);
            if (src_x >= width) src_x = width - 1;
            
            // Get pixel value and add accumulated error
            int pixel = gray_data[src_y * width + src_x] + curr_row[ox + 1];
            
            // Threshold to black or white
            bool white = pixel > 127;
            oled_set_pixel(ox, oy, white);
            
            // Calculate quantization error
            int error = pixel - (white ? 255 : 0);
            
            // Distribute error to neighbors (Floyd-Steinberg)
            curr_row[ox + 2] += error * 7 / 16;
            next_row[ox]     += error * 3 / 16;
            next_row[ox + 1] += error * 5 / 16;
            next_row[ox + 2] += error * 1 / 16;
        }
    }
    
    free(errors);
    oled_update();
}

esp_err_t oled_show_preview(const uint8_t *jpeg_data, size_t jpeg_len)
{
    if (!is_init) return ESP_ERR_INVALID_STATE;
    if (!jpeg_data || jpeg_len == 0) return ESP_ERR_INVALID_ARG;
    
    ESP_LOGI(TAG, "Decoding JPEG for preview (%d bytes)", jpeg_len);
    
    // Allocate grayscale buffer for OLED resolution
    uint8_t *gray_buf = heap_caps_calloc(OLED_WIDTH * OLED_HEIGHT, 1, MALLOC_CAP_DEFAULT);
    if (!gray_buf) {
        ESP_LOGE(TAG, "Failed to allocate grayscale buffer");
        return ESP_ERR_NO_MEM;
    }
    
    // Get JPEG info first
    esp_jpeg_image_cfg_t jpeg_cfg = {
        .indata = (uint8_t *)jpeg_data,
        .indata_size = jpeg_len,
        .outbuf = NULL,
        .outbuf_size = 0,
        .out_format = JPEG_IMAGE_FORMAT_RGB565,
        .out_scale = JPEG_IMAGE_SCALE_1_8,  // Scale down to 1/8 for faster decode
        .flags = {
            .swap_color_bytes = 0,
        }
    };
    
    esp_jpeg_image_output_t out_info;
    
    // Allocate decode buffer (at 1/8 scale, even 2048x1536 becomes 256x192)
    // Max buffer needed: 256*192*2 = ~98KB for RGB565
    size_t decode_buf_size = 320 * 240 * 2;  // Sufficient for 1/8 scaled images
    uint8_t *decode_buf = heap_caps_malloc(decode_buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!decode_buf) {
        decode_buf = heap_caps_malloc(decode_buf_size, MALLOC_CAP_DEFAULT);
    }
    
    if (!decode_buf) {
        ESP_LOGE(TAG, "Failed to allocate decode buffer");
        free(gray_buf);
        return ESP_ERR_NO_MEM;
    }
    
    jpeg_cfg.outbuf = decode_buf;
    jpeg_cfg.outbuf_size = decode_buf_size;
    
    // Decode JPEG
    esp_err_t ret = esp_jpeg_decode(&jpeg_cfg, &out_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "JPEG decode failed: %s", esp_err_to_name(ret));
        free(decode_buf);
        free(gray_buf);
        return ret;
    }
    
    ESP_LOGI(TAG, "Decoded image: %dx%d", out_info.width, out_info.height);
    
    // Convert RGB565 to grayscale and scale to OLED size
    uint16_t *rgb565 = (uint16_t *)decode_buf;
    float scale_x = (float)out_info.width / OLED_WIDTH;
    float scale_y = (float)out_info.height / OLED_HEIGHT;
    
    for (int oy = 0; oy < OLED_HEIGHT; oy++) {
        for (int ox = 0; ox < OLED_WIDTH; ox++) {
            int src_x = (int)(ox * scale_x);
            int src_y = (int)(oy * scale_y);
            
            if (src_x >= out_info.width) src_x = out_info.width - 1;
            if (src_y >= out_info.height) src_y = out_info.height - 1;
            
            uint16_t pixel = rgb565[src_y * out_info.width + src_x];
            
            // Extract RGB and convert to grayscale
            uint8_t r = ((pixel >> 11) & 0x1F) << 3;
            uint8_t g = ((pixel >> 5) & 0x3F) << 2;
            uint8_t b = (pixel & 0x1F) << 3;
            
            gray_buf[oy * OLED_WIDTH + ox] = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
        }
    }
    
    free(decode_buf);
    
    // Apply dithering and display
    oled_draw_grayscale(gray_buf, OLED_WIDTH, OLED_HEIGHT);
    
    free(gray_buf);
    
    return ESP_OK;
}
