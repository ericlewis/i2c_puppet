# I2C Puppet ESP32 Firmware

This is the ESP32-side firmware component of the I2C Puppet project, allowing firmware updates over I2C to ESP32 boards.

## Overview

The ESP32 firmware provides a bridge that allows firmware updates to be sent from the RP2040 (which receives updates over I2C) to the ESP32 over a UART connection. It implements an OTA (Over-The-Air) update mechanism for ESP32 using the ESP-IDF's built-in OTA functionality.

## Features

- Auto-detection by the RP2040 I2C Puppet controller on startup
- UART communication with the RP2040 I2C Puppet controller
- Firmware update protocol supporting Intel HEX format
- OTA update system for updating ESP32 firmware
- Status reporting back to RP2040

## Building

This project uses the ESP-IDF build system. To build:

1. Set up the ESP-IDF environment variables
2. Navigate to the `esp32` directory
3. Run the build command:
   ```
   idf.py build
   ```

## Flashing

To flash directly to the ESP32:

```
idf.py -p <PORT> flash
```

## Usage

Once flashed, the ESP32 will automatically establish communication with the RP2040 I2C Puppet controller via UART. It accepts the following commands:

- `CMD_PING`: Ping the ESP32 to check connectivity
- `CMD_BEGIN_UPDATE`: Begin a firmware update
- `CMD_DATA_CHUNK`: Send firmware data chunk
- `CMD_END_UPDATE`: Complete firmware update and prepare for reboot
- `CMD_ABORT_UPDATE`: Abort current firmware update
- `CMD_REBOOT`: Reboot the ESP32

## Protocol

The ESP32 and RP2040 communicate using a simple packet-based protocol:

1. Command byte (1 byte)
2. Data length (3 bytes, little-endian)
3. Data (optional, based on data length)

The ESP32 responds with a single byte:
- `RESP_OK`: Command executed successfully
- `RESP_ERROR`: Error occurred
- `RESP_BUSY`: ESP32 is busy processing
- `RESP_READY`: ESP32 is ready to receive more data

## Integration with I2C Puppet

This firmware is designed to work with the I2C Puppet project. On the RP2040 side, the I2C Puppet provides I2C registers that clients can use to:

1. Select the update target platform (RP2040 or ESP32)
2. Check ESP32 connectivity
3. Send firmware update data
4. Monitor update status

This allows ESP32 firmware updates to be delivered through the same I2C interface that the I2C Puppet exposes to clients.