#include "esp32_flash.h"
#include "esp32_comm.h"
#include "../update.h"

#include <pico/stdlib.h>
#include <string.h>
#include <stdio.h>

// ESP32 firmware update state
typedef struct {
    bool updating;
    bool reading_header;
    char line_buffer[1024];
    size_t line_idx;
    uint32_t bytes_sent;
    int status;
} esp32_update_state_t;

static esp32_update_state_t esp32_update;

// Initialize ESP32 firmware update
void esp32_flash_init(void)
{
    // Reset update state
    memset(&esp32_update, 0, sizeof(esp32_update));
    esp32_update.status = UPDATE_OFF;
    
    // Ensure ESP32 communication is initialized
    esp32_comm_init();

#ifndef NDEBUG
    printf("ESP32 flash update initialized\r\n");
#endif
}

// Check if ESP32 is in update mode
bool esp32_flash_is_updating(void)
{
    return esp32_update.updating;
}

// Process incoming update data byte for ESP32
int esp32_flash_recv(uint8_t b)
{
    int rc;

    // Header resets update state
    if (b == '+') {
        esp32_flash_init();
        esp32_update.updating = true;
        esp32_update.reading_header = true;
        esp32_update.status = UPDATE_RECV;

        // Check if ESP32 is connected
        if (!esp32_is_connected()) {
            esp32_update.status = UPDATE_FAILED_ESP32_COMM_ERROR;
            return -esp32_update.status;
        }

        // Send begin update command to ESP32
        rc = esp32_send_command(ESP32_CMD_BEGIN_UPDATE, NULL, 0);
        if (rc < 0) {
            esp32_update.status = UPDATE_FAILED_ESP32_COMM_ERROR;
            return -esp32_update.status;
        }

        return 1; // Continue receiving
    }

    // Ignore header contents up to end of line
    if (esp32_update.reading_header) {
        if (b == '\n' || b == '\r') {
            esp32_update.reading_header = false;
        }
        return 1; // Continue receiving
    }

    // Check for line terminator
    if (b == '\n' || b == '\r') {
        b = '\0';
    }

    // Check for line buffer overflow
    if (esp32_update.line_idx >= sizeof(esp32_update.line_buffer) - 1) {
        esp32_update.status = UPDATE_FAILED_LINE_OVERFLOW;
        return -esp32_update.status;
    }

    // Add byte to line buffer
    esp32_update.line_buffer[esp32_update.line_idx++] = b;

    // If this wasn't the end of line, continue
    if (b != '\0') {
        return 1; // Continue receiving
    }

    // Process complete line (null-terminated)
    if (esp32_update.line_buffer[0] == '\0') {
        // Empty line, reset buffer
        esp32_update.line_idx = 0;
        return 1; // Continue receiving
    }

    // Process Intel HEX record
    // Check if line starts with ':'
    if (esp32_update.line_buffer[0] != ':') {
        // Invalid hex record
        esp32_update.status = UPDATE_FAILED_BAD_LINE;
        return -esp32_update.status;
    }

    // Send data chunk to ESP32
    rc = esp32_send_command(ESP32_CMD_DATA_CHUNK, 
                          (const uint8_t*)esp32_update.line_buffer, 
                          strlen(esp32_update.line_buffer));
    
    // Reset line buffer
    esp32_update.line_idx = 0;
    
    if (rc < 0) {
        esp32_update.status = UPDATE_FAILED_ESP32_COMM_ERROR;
        return -esp32_update.status;
    }

    // Check if this is the end record (":00000001FF")
    if (strncmp(esp32_update.line_buffer, ":00000001FF", 11) == 0) {
        esp32_update.status = UPDATE_ESP32_AWAITING_REBOOT;
        return 0; // Update complete
    }

    return 1; // Continue receiving
}

// Commit ESP32 firmware update and reboot ESP32
void esp32_flash_commit_and_reboot(void)
{
    // Send end update command to ESP32
    esp32_send_command(ESP32_CMD_END_UPDATE, NULL, 0);
    
    // Wait a bit for ESP32 to process
    sleep_ms(100);
    
    // Send reboot command to ESP32
    esp32_send_command(ESP32_CMD_REBOOT, NULL, 0);
    
    // Reset state
    esp32_update.updating = false;
    esp32_update.status = UPDATE_OFF;
    
#ifndef NDEBUG
    printf("ESP32 firmware update complete, ESP32 rebooting\r\n");
#endif
}