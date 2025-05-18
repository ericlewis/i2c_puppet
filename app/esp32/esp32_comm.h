#pragma once

#include <stdbool.h>
#include <stdint.h>

// ESP32 UART communication
#define ESP32_UART_ID           uart1
#define ESP32_UART_TX_PIN       4   // Adjust based on your board configuration
#define ESP32_UART_RX_PIN       5   // Adjust based on your board configuration
#define ESP32_UART_BAUD_RATE    115200

// ESP32 communication protocol commands
#define ESP32_CMD_PING          0x00
#define ESP32_CMD_BEGIN_UPDATE  0xA0
#define ESP32_CMD_DATA_CHUNK    0xA1
#define ESP32_CMD_END_UPDATE    0xA2
#define ESP32_CMD_ABORT_UPDATE  0xA3
#define ESP32_CMD_REBOOT        0xA4

// ESP32 communication protocol responses
#define ESP32_RESP_OK          0xB0
#define ESP32_RESP_ERROR       0xB1
#define ESP32_RESP_BUSY        0xB2
#define ESP32_RESP_READY       0xB3

// Auto-detect and initialize ESP32 communication
// Returns true if ESP32 is detected and connected
bool esp32_auto_detect(void);

// Initialize ESP32 communication
void esp32_comm_init(void);

// Send command to ESP32
int esp32_send_command(uint8_t cmd, const uint8_t* data, size_t len);

// Check if ESP32 is connected
bool esp32_is_connected(void);

// Process UART data from ESP32
void esp32_process_uart_data(void);