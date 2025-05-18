#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"

#include "uart_bridge.h"
#include "update.h"

static const char *TAG = "i2c_puppet_esp32";

void app_main(void)
{
    // Initialize UART bridge for communication with RP2040
    uart_bridge_init();

    // Initialize firmware update system
    update_init();

    ESP_LOGI(TAG, "I2C Puppet ESP32 firmware v1.0.0 started");
    ESP_LOGI(TAG, "Free heap: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "Ready for auto-detection and commands from RP2040");

    // Main ESP32 loop - process UART commands from RP2040
    uart_bridge_task();
}