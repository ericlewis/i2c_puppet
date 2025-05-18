#pragma once

#include <stdint.h>
#include <stdbool.h>

// UART communication defines
#define UART_PORT_NUM      UART_NUM_0
#define UART_BAUD_RATE     115200
#define UART_BUF_SIZE      1024

// Command protocol defines - must match RP2040 side
#define CMD_PING           0x00
#define CMD_BEGIN_UPDATE   0xA0
#define CMD_DATA_CHUNK     0xA1
#define CMD_END_UPDATE     0xA2
#define CMD_ABORT_UPDATE   0xA3
#define CMD_REBOOT         0xA4

// Response defines - must match RP2040 side
#define RESP_OK            0xB0
#define RESP_ERROR         0xB1
#define RESP_BUSY          0xB2
#define RESP_READY         0xB3

/**
 * Initialize UART bridge for communication with RP2040
 */
void uart_bridge_init(void);

/**
 * Main UART bridge task that processes commands from RP2040
 * This function runs in an infinite loop
 */
void uart_bridge_task(void);

/**
 * Send response byte to RP2040
 */
void uart_bridge_send_response(uint8_t response);