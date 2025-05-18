#pragma once

#include "reg.h"
#include "app_config.h"

enum update_mode {
	UPDATE_OFF = 0,
	UPDATE_RECV = 1,
	UPDATE_FAILED = 2,
	UPDATE_FAILED_LINE_OVERFLOW = 3,
	UPDATE_FAILED_FLASH_EMPTY = 4,
	UPDATE_FAILED_FLASH_OVERFLOW = 5,
	UPDATE_FAILED_BAD_LINE = 6,
	UPDATE_FAILED_BAD_CHECKSUM = 7,
	UPDATE_FAILED_ESP32_COMM_ERROR = 8,
	UPDATE_FAILED_UNSUPPORTED_PLATFORM = 9,
	UPDATE_ESP32_AWAITING_REBOOT = 10,
};

// Reset update state
void update_init(void);

// Return 1 when there is more to read
// Return 0 when complete firmware received
int update_recv(uint8_t b);

// Flash received firmware
void update_commit_and_reboot(void);
