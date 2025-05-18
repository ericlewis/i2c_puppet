#include "esp32_comm.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"

#include <stdio.h>
#include <string.h>

static struct {
    bool initialized;
    bool connected;
    uint8_t rx_buffer[256];
    uint8_t rx_buffer_idx;
} esp32_comm;

// Auto-detect and initialize ESP32 communication
bool esp32_auto_detect(void)
{
    // Only try to auto-detect if not already initialized
    if (!esp32_comm.initialized) {
        esp32_comm_init();
    }
    
    // Try to ping the ESP32 several times
    for (int i = 0; i < 3; i++) {
        if (esp32_is_connected()) {
            esp32_comm.connected = true;
            
#ifndef NDEBUG
            printf("ESP32 detected and connected\r\n");
#endif
            return true;
        }
        sleep_ms(100); // Wait a bit between retries
    }
    
#ifndef NDEBUG
    printf("ESP32 not detected\r\n");
#endif
    return false;
}

// Initialize UART for ESP32 communication
void esp32_comm_init(void)
{
    uart_init(ESP32_UART_ID, ESP32_UART_BAUD_RATE);

    gpio_set_function(ESP32_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(ESP32_UART_RX_PIN, GPIO_FUNC_UART);

    // Set FIFO levels
    uart_set_fifo_enabled(ESP32_UART_ID, true);

    // Clear state
    memset(&esp32_comm, 0, sizeof(esp32_comm));
    esp32_comm.initialized = true;

#ifndef NDEBUG
    printf("ESP32 communication initialized\r\n");
#endif
}

// Send command to ESP32 and wait for response
int esp32_send_command(uint8_t cmd, const uint8_t* data, size_t len)
{
    if (!esp32_comm.initialized) {
        return -1;
    }

    // Prepare command header (1 byte command + 3 bytes length)
    uint8_t header[4] = {
        cmd,
        (uint8_t)(len & 0xFF),
        (uint8_t)((len >> 8) & 0xFF),
        (uint8_t)((len >> 16) & 0xFF)
    };

    // Send command header
    uart_write_blocking(ESP32_UART_ID, header, sizeof(header));
    
    // Send command data if any
    if (len > 0 && data != NULL) {
        uart_write_blocking(ESP32_UART_ID, data, len);
    }
    
    // Wait for response
    uint8_t response = 0;
    absolute_time_t timeout = make_timeout_time_ms(1000); // 1 second timeout
    
    while (!time_reached(timeout)) {
        if (uart_is_readable(ESP32_UART_ID)) {
            uart_read_blocking(ESP32_UART_ID, &response, 1);
            
            if (response == ESP32_RESP_OK) {
                return 1; // Success
            } else if (response == ESP32_RESP_BUSY) {
                // ESP32 is busy, wait a bit and try again
                sleep_ms(100);
                continue;
            } else {
                return -1; // Error
            }
        }
        sleep_ms(10);
    }
    
    return -1; // Timeout
}

// Check if ESP32 is connected by sending a ping command
bool esp32_is_connected(void)
{
    if (!esp32_comm.initialized) {
        return false;
    }

    // Clear any pending UART data
    while (uart_is_readable(ESP32_UART_ID)) {
        uint8_t dummy;
        uart_read_blocking(ESP32_UART_ID, &dummy, 1);
    }
    
    // Send ping command
    int result = esp32_send_command(ESP32_CMD_PING, NULL, 0);
    
    esp32_comm.connected = (result == 1);
    return esp32_comm.connected;
}

// Process any available UART data from ESP32
void esp32_process_uart_data(void)
{
    if (!esp32_comm.initialized) {
        return;
    }
    
    while (uart_is_readable(ESP32_UART_ID)) {
        uint8_t data;
        uart_read_blocking(ESP32_UART_ID, &data, 1);
        
        // Process received data here
        // For now, just store in buffer
        if (esp32_comm.rx_buffer_idx < sizeof(esp32_comm.rx_buffer)) {
            esp32_comm.rx_buffer[esp32_comm.rx_buffer_idx++] = data;
        }
    }
}