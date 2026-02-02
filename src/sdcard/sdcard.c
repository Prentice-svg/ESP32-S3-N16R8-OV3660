/**
 * SD Card Driver Implementation
 * Uses ESP-IDF FatFS/VFS for SD card access
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "diskio.h"
#include "ff.h"
#include "sdcard.h"
#include "camera_pins.h"

static const char *TAG = "sdcard";
static sdmmc_card_t *card = NULL;
static bool is_init = false;
static sdcard_info_t card_info = {0};

// Mount point for FATFS
#define MOUNT_POINT "/sdcard"

esp_err_t sdcard_init(void)
{
    if (is_init) {
        ESP_LOGW(TAG, "SD Card already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing SD Card...");

    // Mount configuration
    esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 5,
        .format_if_mount_failed = false,
        .allocation_unit_size = 16 * 1024
    };

    esp_err_t ret;

    // Try SDMMC first (1-bit mode for Freenove ESP32-S3 CAM)
    sdmmc_host_t sdmmc_host = SDMMC_HOST_DEFAULT();
    sdmmc_host.max_freq_khz = SDMMC_FREQ_DEFAULT;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_cd = SDMMC_SLOT_NO_CD;
    slot_config.gpio_wp = SDMMC_SLOT_NO_WP;
    slot_config.width = 1;  // 1-bit mode only for Freenove board

    slot_config.clk = SD_PIN_CLK;
    slot_config.cmd = SD_PIN_CMD;
    slot_config.d0 = SD_PIN_D0;

    ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &sdmmc_host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SDMMC mount failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check SD card is inserted and pins: CLK=%d, CMD=%d, D0=%d", 
                 SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0);
        return ret;
    }

    // Get card information - card is now available from the mount
    if (card) {
        snprintf(card_info.card_name, sizeof(card_info.card_name), "SD Card");
        card_info.card_size = (uint64_t)card->csd.capacity * card->csd.sector_size;
    } else {
        snprintf(card_info.card_name, sizeof(card_info.card_name), "SD Card");
        card_info.card_size = 0;
    }
    card_info.initialized = true;
    is_init = true;

    // Calculate free space
    FATFS *fs;
    DWORD fre_clust;
    if (f_getfree("0:", &fre_clust, &fs) == FR_OK) {
        uint64_t total = (uint64_t)(fs->n_fatent - 2) * fs->csize;
        uint64_t free = (uint64_t)fre_clust * fs->csize;
        card_info.free_space = free * card->csd.sector_size;
        card_info.used_space = (total - free) * card->csd.sector_size;
    }

    ESP_LOGI(TAG, "SD Card mounted: %s, Size: %llu MB",
             card_info.card_name, card_info.card_size / (1024 * 1024));
    ESP_LOGI(TAG, "Free space: %llu MB", card_info.free_space / (1024 * 1024));

    return ESP_OK;
}

esp_err_t sdcard_deinit(void)
{
    if (!is_init) return ESP_OK;

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    if (ret == ESP_OK) {
        is_init = false;
        card = NULL;
        card_info.initialized = false;
        ESP_LOGI(TAG, "SD Card unmounted");
    }
    return ret;
}

void sdcard_get_info(sdcard_info_t *info)
{
    if (info == NULL) return;

    memcpy(info, &card_info, sizeof(sdcard_info_t));

    // Update free space
    if (is_init) {
        FATFS *fs;
        DWORD fre_clust;
        if (f_getfree("0:", &fre_clust, &fs) == FR_OK) {
            info->free_space = (uint64_t)fre_clust * fs->csize * 512; // Assume 512 byte sectors
        }
    }
}

bool sdcard_is_ready(void)
{
    return is_init;
}

esp_err_t sdcard_write_file(const char *path, const uint8_t *data, size_t len)
{
    if (!is_init) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s/%s", MOUNT_POINT, path);

    ESP_LOGI(TAG, "Writing %d bytes to: %s", len, full_path);

    FILE *f = fopen(full_path, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s (errno=%d)", path, errno);
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, len, f);
    int flush_ret = fflush(f);
    fclose(f);

    if (written != len || flush_ret != 0) {
        ESP_LOGE(TAG, "Failed to write all data (written=%d, expected=%d, flush=%d)", written, len, flush_ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Written %d bytes to %s", len, path);
    return ESP_OK;
}

esp_err_t sdcard_append_file(const char *path, const uint8_t *data, size_t len)
{
    if (!is_init) return ESP_ERR_INVALID_STATE;

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s/%s", MOUNT_POINT, path);

    FILE *f = fopen(full_path, "ab");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for appending: %s", path);
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, len, f);
    fclose(f);

    if (written != len) {
        ESP_LOGE(TAG, "Failed to append all data");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t sdcard_read_file(const char *path, uint8_t *data, size_t *len)
{
    if (!is_init) return ESP_ERR_INVALID_STATE;

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s/%s", MOUNT_POINT, path);

    FILE *f = fopen(full_path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", path);
        return ESP_FAIL;
    }

    size_t read = fread(data, 1, *len, f);
    *len = read;
    fclose(f);

    return ESP_OK;
}

esp_err_t sdcard_read_file_offset(const char *path, size_t offset, uint8_t *data, size_t *len)
{
    if (!is_init) return ESP_ERR_INVALID_STATE;

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s/%s", MOUNT_POINT, path);

    FILE *f = fopen(full_path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", path);
        return ESP_FAIL;
    }

    // Seek to offset
    if (fseek(f, (long)offset, SEEK_SET) != 0) {
        ESP_LOGE(TAG, "Failed to seek to offset %zu in file: %s", offset, path);
        fclose(f);
        return ESP_FAIL;
    }

    size_t read = fread(data, 1, *len, f);
    *len = read;
    fclose(f);

    return ESP_OK;
}

esp_err_t sdcard_delete_file(const char *path)
{
    if (!is_init) return ESP_ERR_INVALID_STATE;

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s/%s", MOUNT_POINT, path);

    if (remove(full_path) != 0) {
        ESP_LOGE(TAG, "Failed to delete file: %s", path);
        return ESP_FAIL;
    }

    return ESP_OK;
}

bool sdcard_exists(const char *path)
{
    if (!is_init) return false;

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s/%s", MOUNT_POINT, path);

    struct stat st;
    return stat(full_path, &st) == 0;
}

int sdcard_list_files(const char *path, char *buffer, size_t buffer_size)
{
    if (!is_init) return 0;

    char dir_path[128];
    if (path == NULL || strlen(path) == 0) {
        strcpy(dir_path, MOUNT_POINT);
    } else {
        snprintf(dir_path, sizeof(dir_path), "%s/%s", MOUNT_POINT, path);
    }

    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory: %s", dir_path);
        return 0;
    }

    int count = 0;
    struct dirent *entry;
    size_t offset = 0;

    while ((entry = readdir(dir)) != NULL && offset < buffer_size - 64) {
        size_t written = snprintf(buffer + offset, buffer_size - offset,
                                  "%s%s\n", entry->d_name,
                                  (entry->d_type & DT_DIR) ? "/" : "");
        if (written > 0) {
            offset += written;
            count++;
        }
    }

    closedir(dir);
    return count;
}

int64_t sdcard_get_file_size(const char *path)
{
    if (!is_init) return -1;

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s/%s", MOUNT_POINT, path);

    struct stat st;
    if (stat(full_path, &st) != 0) {
        return -1;
    }
    return st.st_size;
}

esp_err_t sdcard_mkdir(const char *path)
{
    if (!is_init) return ESP_ERR_INVALID_STATE;

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "%s/%s", MOUNT_POINT, path);

    // Check if directory already exists
    struct stat st;
    if (stat(full_path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            ESP_LOGI(TAG, "Directory already exists: %s", path);
            return ESP_OK;
        }
    }

    if (mkdir(full_path, 0755) != 0) {
        ESP_LOGE(TAG, "Failed to create directory: %s (errno=%d)", path, errno);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Directory created: %s", path);
    return ESP_OK;
}

static uint32_t file_index = 0;

const char *sdcard_get_next_filename(const char *prefix, const char *extension,
                                      char *buffer, size_t buffer_size, uint32_t *index)
{
    if (!is_init) return NULL;

    if (index == NULL) {
        index = &file_index;
    }

    // Generate timestamp-based filename
    time_t now;
    time(&now);
    struct tm *tm_info = localtime(&now);

    // Format: PREFIX_YYYYMMDD_HHMMSS_XXXX.extension
    snprintf(buffer, buffer_size, "%s_%04d%02d%02d_%02d%02d%02d_%04lu%s",
             prefix,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
             (unsigned long)*index,
             extension);

    (*index)++;

    return buffer;
}
