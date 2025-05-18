#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize the firmware update system
 */
void update_init(void);

/**
 * Begin a firmware update process
 * 
 * @return true if update started successfully, false on failure
 */
bool update_begin(void);

/**
 * Write a chunk of firmware data
 * 
 * @param data Pointer to the data to write
 * @param len Length of the data to write
 * @return true if chunk was written successfully, false on failure
 */
bool update_write_chunk(const uint8_t *data, size_t len);

/**
 * Complete the firmware update and prepare for reboot
 * 
 * @return true if update was finalized successfully, false on failure
 */
bool update_end(void);

/**
 * Abort the current firmware update
 */
void update_abort(void);