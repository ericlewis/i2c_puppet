#include "update.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"

static const char *TAG = "update";

// OTA update state
static esp_ota_handle_t ota_handle = 0;
static const esp_partition_t *update_partition = NULL;
static bool update_in_progress = false;

// Initialize update system
void update_init(void)
{
    ESP_LOGI(TAG, "Update system initialized");
    update_partition = NULL;
    update_in_progress = false;
}

// Begin firmware update
bool update_begin(void)
{
    esp_err_t err;

    // Check if update is already in progress
    if (update_in_progress) {
        ESP_LOGW(TAG, "Update already in progress");
        return false;
    }

    // Get the next OTA partition
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "No OTA partition found");
        return false;
    }

    ESP_LOGI(TAG, "Starting OTA update to partition type %d subtype %d at offset 0x%x",
             update_partition->type, update_partition->subtype, update_partition->address);

    // Begin OTA update
    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed, error=%d", err);
        return false;
    }

    update_in_progress = true;
    return true;
}

// Write chunk of data to update partition
bool update_write_chunk(const uint8_t *data, size_t len)
{
    esp_err_t err;

    // Check if update is in progress
    if (!update_in_progress) {
        ESP_LOGW(TAG, "No update in progress");
        return false;
    }

    // Write data to partition
    err = esp_ota_write(ota_handle, data, len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_write failed, error=%d", err);
        update_abort();
        return false;
    }

    return true;
}

// Finish update and prepare for boot
bool update_end(void)
{
    esp_err_t err;

    // Check if update is in progress
    if (!update_in_progress) {
        ESP_LOGW(TAG, "No update in progress");
        return false;
    }

    // End OTA update
    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed, error=%d", err);
        update_in_progress = false;
        return false;
    }

    // Set boot partition
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed, error=%d", err);
        update_in_progress = false;
        return false;
    }

    ESP_LOGI(TAG, "OTA update completed successfully. Restart required to boot into new firmware.");
    update_in_progress = false;
    return true;
}

// Abort update process
void update_abort(void)
{
    if (update_in_progress) {
        ESP_LOGW(TAG, "Aborting OTA update");
        esp_ota_abort(ota_handle);
        update_in_progress = false;
    }
}