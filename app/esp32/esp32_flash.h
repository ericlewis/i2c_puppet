#pragma once

#include <stdbool.h>
#include <stdint.h>

// ESP32 update status codes (in addition to ones in update.h)
#define UPDATE_ESP32_FAILED_COMM        8
#define UPDATE_ESP32_UNSUPPORTED_PLATFORM 9
#define UPDATE_ESP32_AWAITING_REBOOT    10

// Initialize ESP32 firmware update mechanism
void esp32_flash_init(void);

// Process incoming update data for ESP32
// Return 1 when more to receive, 0 when complete, negative on error
int esp32_flash_recv(uint8_t b);

// Commit ESP32 firmware and reboot
void esp32_flash_commit_and_reboot(void);

// Check if ESP32 is in update mode
bool esp32_flash_is_updating(void);