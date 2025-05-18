#include "uart_bridge.h"
#include "update.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "uart_bridge";

// UART buffer
static uint8_t uart_rx_buffer[UART_BUF_SIZE];

// Initialize UART for RP2040 communication
void uart_bridge_init(void)
{
    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_APB,
    };

    // Install UART driver
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));

    ESP_LOGI(TAG, "UART bridge initialized");
}

// Send response byte to RP2040
void uart_bridge_send_response(uint8_t response)
{
    uart_write_bytes(UART_PORT_NUM, &response, 1);
    ESP_LOGD(TAG, "Sent response: 0x%02X", response);
}

// Process command from RP2040
static void process_command(uint8_t cmd, uint8_t *data, size_t data_len)
{
    ESP_LOGD(TAG, "Processing command: 0x%02X, data_len: %d", cmd, data_len);

    switch (cmd) {
        case CMD_PING:
            ESP_LOGI(TAG, "Received PING command");
            uart_bridge_send_response(RESP_OK);
            break;

        case CMD_BEGIN_UPDATE:
            ESP_LOGI(TAG, "Received BEGIN_UPDATE command");
            if (update_begin()) {
                uart_bridge_send_response(RESP_OK);
            } else {
                uart_bridge_send_response(RESP_ERROR);
            }
            break;

        case CMD_DATA_CHUNK:
            ESP_LOGD(TAG, "Received DATA_CHUNK command");
            if (update_write_chunk(data, data_len)) {
                uart_bridge_send_response(RESP_OK);
            } else {
                uart_bridge_send_response(RESP_ERROR);
            }
            break;

        case CMD_END_UPDATE:
            ESP_LOGI(TAG, "Received END_UPDATE command");
            if (update_end()) {
                uart_bridge_send_response(RESP_OK);
            } else {
                uart_bridge_send_response(RESP_ERROR);
            }
            break;

        case CMD_ABORT_UPDATE:
            ESP_LOGI(TAG, "Received ABORT_UPDATE command");
            update_abort();
            uart_bridge_send_response(RESP_OK);
            break;

        case CMD_REBOOT:
            ESP_LOGI(TAG, "Received REBOOT command - rebooting...");
            uart_bridge_send_response(RESP_OK);
            vTaskDelay(pdMS_TO_TICKS(100)); // Short delay to ensure response is sent
            esp_restart();
            break;

        default:
            ESP_LOGW(TAG, "Unknown command: 0x%02X", cmd);
            uart_bridge_send_response(RESP_ERROR);
            break;
    }
}

// Main UART bridge task
void uart_bridge_task(void)
{
    size_t rx_bytes = 0;
    uint8_t cmd = 0;
    uint8_t len_bytes[3] = {0};
    uint32_t data_len = 0;

    while (1) {
        // Wait for command byte
        rx_bytes = uart_read_bytes(UART_PORT_NUM, &cmd, 1, pdMS_TO_TICKS(100));
        if (rx_bytes != 1) {
            continue;  // No data received
        }

        // Read length bytes (3 bytes, little-endian)
        rx_bytes = uart_read_bytes(UART_PORT_NUM, len_bytes, 3, pdMS_TO_TICKS(1000));
        if (rx_bytes != 3) {
            ESP_LOGW(TAG, "Failed to receive length bytes");
            uart_bridge_send_response(RESP_ERROR);
            continue;
        }

        // Calculate data length
        data_len = len_bytes[0] | (len_bytes[1] << 8) | (len_bytes[2] << 16);

        // If there's data to read
        if (data_len > 0) {
            // Check if data length is within bounds
            if (data_len > UART_BUF_SIZE) {
                ESP_LOGW(TAG, "Data length too large: %u", data_len);
                uart_bridge_send_response(RESP_ERROR);
                continue;
            }

            // Read data
            rx_bytes = uart_read_bytes(UART_PORT_NUM, uart_rx_buffer, data_len, pdMS_TO_TICKS(5000));
            if (rx_bytes != data_len) {
                ESP_LOGW(TAG, "Failed to receive all data: %d/%d", rx_bytes, data_len);
                uart_bridge_send_response(RESP_ERROR);
                continue;
            }

            // Process command with data
            process_command(cmd, uart_rx_buffer, data_len);
        } else {
            // Process command without data
            process_command(cmd, NULL, 0);
        }
    }
}