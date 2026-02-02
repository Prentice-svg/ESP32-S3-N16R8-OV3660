# ESP32-S3 Timelapse Camera - Project Context

## Project Overview

**Name:** ESP32-S3 Timelapse Camera
**Target Hardware:** ESP32-S3-N16R8 (16MB Flash, 16MB PSRAM) + OV3660 Camera (3MP)
**Framework:** ESP-IDF (via PlatformIO or native)
**Core Function:** Intelligent timelapse photography system with WiFi control, Web UI, SD card storage, and power management.

## Key Features

*   **Imaging:** 3MP OV3660 Camera, JPEG output, multiple resolutions (UXGA to QVGA).
*   **Storage:** SD Card (SDMMC 4-bit mode recommended) for storing thousands of images.
*   **Connectivity:** WiFi (AP & STA modes) with a real-time Web Interface.
*   **Power:** Battery voltage monitoring, low battery protection, and Deep Sleep (<10μA).
*   **Interface:** Optional ST7789 2.8" LCD support.
*   **Performance:** Uses 16MB PSRAM for image buffering.

## Hardware Configuration

### Pin Definitions

**Camera (OV3660):**
*   XCLK: GPIO8
*   SIOD (SDA): GPIO40
*   SIOC (SCL): GPIO41
*   VSYNC: GPIO6
*   HREF: GPIO7
*   PCLK: GPIO13
*   Data (D0-D7): GPIO11, 9, 4, 5, 15, 16, 17, 18

**SD Card (SDMMC):**
*   CLK: GPIO12
*   CMD: GPIO11
*   D0-D3: GPIO13, 14, 8, 46

**LCD (Optional ST7789):**
*   SCK: GPIO36
*   MOSI: GPIO35
*   CS: GPIO34
*   DC: GPIO38
*   RST: GPIO37

**Power & Control:**
*   Battery ADC: GPIO1 (Voltage divider required)
*   BOOT Button: GPIO0 (Control: Short press=Pause/Resume, Long press=Deep Sleep)

## Architecture & Code Structure

The project follows a modular C structure with ESP-IDF components.

**Directory Tree:**
*   `src/`: Source code
    *   `main.c`: Entry point, initialization sequence, main loop.
    *   `camera/`: OV3660 driver integration.
    *   `sdcard/`: SDMMC filesystem handling.
    *   `wifi/`: WiFi connection and WebServer logic.
    *   `timelapse/`: Core timelapse state machine.
    *   `power/`: Battery monitoring and sleep logic.
*   `include/`: Header files corresponding to source modules.
*   `platformio.ini`: PlatformIO build configuration.
*   `partitions.csv`: Custom partition table (Large FATFS partition).

**Data Flow:**
1.  **Web Request:** User initiates `POST /start` via Web UI.
2.  **Timelapse Engine:** timer triggers `camera_capture()` at set intervals.
3.  **Capture:** Image data captured to PSRAM buffer.
4.  **Storage:** `save_photo()` writes buffer to SD Card with timestamped filename.
5.  **Sleep:** Between shots (if interval is long) or manually triggered, system enters Deep Sleep to save power.

## Build & Run

### PlatformIO (Recommended)
*   **Build:** `pio run`
*   **Upload:** `pio run -t upload`
*   **Monitor:** `pio device monitor`
*   **Config:** Edit `platformio.ini`

### ESP-IDF
*   **Build:** `idf.py build`
*   **Flash:** `idf.py flash`
*   **Monitor:** `idf.py monitor`

## Usage Guide

1.  **Initial Setup:** Flash firmware. Connect to WiFi AP `ESP32-Cam` (Pass: `12345678`).
2.  **Web UI:** Access `http://192.168.4.1`.
3.  **Controls:** Use Web UI to Start/Stop timelapse, view status, or download files.
4.  **Buttons:**
    *   **Short Press (BOOT):** Toggle Start/Pause.
    *   **Long Press (BOOT > 3s):** Enter Deep Sleep.

## Common Issues & Troubleshooting

*   **Camera Init Failed:** Check ribbon cable and pin definitions. Ensure PSRAM is enabled.
*   **SD Mount Failed:** Verify FAT32 formatting and SDMMC pin connections.
*   **WiFi Fail:** Check SSID/Password. If STA fails, it falls back to AP mode.
*   **Brownout:** Ensure stable power supply (LiPo battery or quality USB).

## Development Conventions

*   **Style:** C99 standard. Modular design with header/source separation.
*   **Logging:** Use `ESP_LOGx` macros for debug output.
*   **Memory:** Heavy use of PSRAM for large buffers; avoid large stack allocations.
