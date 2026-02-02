# Copilot Instructions
## Project Snapshot
- ESP-IDF C firmware targeting ESP32-S3-N16R8 with OV3660; core entry is [src/main.c](src/main.c) orchestrating power, storage, camera, WiFi, and timelapse boot order.
- Module layout mirrors headers under [include](include) with implementation folders in [src](src); keep new code aligned with this split.
## Build and Flash
- Preferred workflow uses PlatformIO environment esp32s3cam (platformio.ini); build with pio run from project root.
- Upload via pio run -e esp32s3cam -t upload -p /dev/ttyUSB0 when the board enumerates on USB; Serial monitor expects 115200 baud.
- Flash layout is fixed by partitions.csv with large fatfs region; adjust partition sizes there and rerun a clean build if storage scheme changes.
## Runtime Architecture
- app_main in [src/main.c](src/main.c) initializes NVS, power, buttons, SD, camera, optional LCD, config, WiFi/web, then timelapse; keep new features after prerequisites to avoid boot failures.
- Button handling uses a FreeRTOS task plus event group for the BOOT pin; reuse the BTN_BOOT_PRESSED pattern for additional wake sources.
- The main loop logs every 5 seconds and enforces low battery stops via power_is_low_battery; long operations should live in their own tasks.
## Configuration and State
- System defaults live in reset_config within [src/common/config.c](src/common/config.c); NVS namespaces are system for general settings and timelapse for capture parameters.
- When modifying config structs update both the header in [include/config.h](include/config.h) and save/load code; remember to bump initialization defaults.
- Avoid blocking inside NVS calls; handle ESP_ERR_NVS_NO_FREE_PAGES similarly to existing recovery code in app_main.
## Timelapse Engine
- [src/timelapse/timelapse.c](src/timelapse/timelapse.c) runs a dedicated FreeRTOS task driven by an auto-reload software timer; altering intervals must update xTimerChangePeriod to keep the timer aligned.
- Filenames are timestamped (prefix_YYYYMMDD_HHMMSS_seq.jpg) and persistence relies on sdcard_write_file; maintain sequence_number continuity when adding new save paths.
- Timelapse status populates timelapse_status_t including SD free space; extend responses by editing both status struct and serializers in webserver.c.
## Storage and SD
- [src/sdcard/sdcard.c](src/sdcard/sdcard.c) attempts SDMMC 4-bit first then falls back to SPI using SPI2_HOST; if you change pin assignments adjust both slot_config and spi_bus_config.
- Large writes use FatFS via /sdcard mount; ensure new file ops respect buffer limits and close files promptly to avoid exhausting PSRAM.
## Camera and Preview
- Camera configuration pulls pins from [include/camera_pins.h](include/camera_pins.h); reconcile with board revisions before changing defaults.
- camera_get_preview temporarily forces QVGA before restoring the previous framesize; avoid parallel capture calls that would fight over sensor state.
## Web API and Networking
- Web server routes are defined in the uris array inside [src/wifi/webserver.c](src/wifi/webserver.c); register new handlers there and keep responses lightweight.
- Exposed endpoints are GET /, /status, /config, /preview, /files, /download plus POST /start, /stop, /capture, /config; docs mentioning /reboot or /format are aspirational and currently unimplemented.
- JSON payloads use cJSON; guard allocations and free(json) as shown to prevent leaks.
- WiFi setup in [src/wifi/wifi.c](src/wifi/wifi.c) recreates esp_netif instances each init; call wifi_module_deinit before reconfiguring modes.
## Power and Sleep
- Battery sampling in [src/power/power.c](src/power/power.c) averages ADC1 readings on GPIO1 with a fixed 2:1 divider; tweak VOLTAGE_DIVIDER when hardware changes.
- power_deep_sleep configures GPIO0 wake and optional timer; ensure new wake sources use esp_sleep_enable_* before esp_deep_sleep_start.
## Development Tips
- Logging uses ESP_LOG across modules; follow existing TAG naming and log levels for consistency.
- There are no automated tests; validate changes with PlatformIO build, upload, and by curling /status and exercising button flows.
- Shared buffers (camera fb, SD scratch) reside in PSRAM; prefer stack allocations under 4 KB inside tasks to avoid fragmentation.
