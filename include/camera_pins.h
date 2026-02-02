/**
 * Camera Configuration for Freenove ESP32-S3 CAM Board
 *
 * Pin definitions matching CAMERA_MODEL_ESP32S3_EYE
 * Based on Freenove development board documentation
 */

#ifndef __CAMERA_PINS_H
#define __CAMERA_PINS_H

// Camera Power/Reset Pins
#define CAM_PIN_PWDN    -1      // Power down (not used, directly connected)
#define CAM_PIN_RESET   -1      // Reset (not used, directly connected)

// DVP (Digital Video Port) Pins - ESP32S3_EYE configuration
#define CAM_PIN_XCLK    15      // External clock input
#define CAM_PIN_PCLK    13      // Pixel clock output
#define CAM_PIN_VSYNC   6       // Vertical sync
#define CAM_PIN_HREF    7       // Horizontal reference

// D0-D7 Data Lines (Y2-Y9 in camera config)
#define CAM_PIN_D0      11      // Y2
#define CAM_PIN_D1      9       // Y3
#define CAM_PIN_D2      8       // Y4
#define CAM_PIN_D3      10      // Y5
#define CAM_PIN_D4      12      // Y6
#define CAM_PIN_D5      18      // Y7
#define CAM_PIN_D6      17      // Y8
#define CAM_PIN_D7      16      // Y9

// SCCB (I2C-like) Pins for sensor control
#define CAM_PIN_SIOD    4       // SCCB data (SDA)
#define CAM_PIN_SIOC    5       // SCCB clock (SCL)

// SD Card Pins (1-bit SDMMC mode) - Freenove ESP32-S3 CAM
#define SD_PIN_CLK      39      // SD CLK
#define SD_PIN_CMD      38      // SD CMD
#define SD_PIN_D0       40      // SD D0

// Button Pins
#define BOOT_PIN        0

// ADC for Battery Monitoring
#define BATTERY_ADC_PIN 1       // GPIO1 with ADC1 Channel 0

// OLED Display I2C Pins (0.96" SSD1306/SSD1315)
// Choose unused GPIO pins - adjust based on your wiring
#define OLED_SDA_PIN    2       // I2C SDA
#define OLED_SCL_PIN    3       // I2C SCL

// External Button Pins (Active LOW - connect to GND when pressed)
// K1-K4 buttons from external module
#define KEY1_PIN        14      // K1 - Menu/Select
#define KEY2_PIN        21      // K2 - Start/OK
#define KEY3_PIN        47      // K3 - Stop/Back  
#define KEY4_PIN        48      // K4 - Settings

#endif // __CAMERA_PINS_H
