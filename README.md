# I2C Puppet for Beepy

This is a port of the old [BB Q10 Keyboard-to-I2C Software](https://github.com/solderparty/bbq10kbd_i2c_sw) to the RP2040, expanded with new features, while still staying backwards compatible.

The target product/keyboard for this software is the BB Q20 keyboard, which adds a trackpad to the mix.

On the features side, this software adds USB support, the keyboard acts as a USB keyboard, and the trackpad acts as a USB mouse.

On the I2C side, you can access the key presses, the trackpad state, you can control some of the board GPIOs, as well as the backlight.

## New Feature: ESP32 Support

Starting with version 3.1, I2C Puppet now supports updating ESP32 firmware through the same I2C interface. This allows ESP32 boards with compatible pin headers to be updated using the same protocol.

Key features:
- Automatic detection of connected ESP32 boards on startup
- Select target platform (RP2040 or ESP32) for firmware updates
- Same Intel HEX format for firmware updates
- Check ESP32 connectivity status
- Send commands to the ESP32

See the [ESP32 firmware README](esp32/README.md) for details on the ESP32 side implementation.

See [Protocol](#protocol) for details of the I2C puppet.

## Modifications

Firmware has been updated to use BB10-style sticky modifier keys. It has a corresponding kernel module that has been updated to read modifier fields over I2C.

Holding a modifier key (shift, physical alt, Symbol) while typing an alpha keys will apply the modifier to all alpha keys until the modifier is released.

One press and release of the modifier will enter sticky mode, applying the modifier to
the next alpha key only. If the same modifier key is pressed and released again in sticky mode, it will be canceled.

Call is mapped to Control. The Berry button is mapped to `KEY_PROPS`. Clicking the touchpad button is mapped to `KEY_COMPOSE`. Back is mapped to Escape. End Call is not sent as a key, but holding it will still trigger the power-off routine. Symbol is mapped to AltGr (Right Alt).

This firmware targets the Beepy hardware. It can still act as a USB keyboard, but physical alt keys will not work unless you remap their values.

Physical alt does not send an actual Alt key, but remaps the output scancodes to the range 135 to 161 in QWERTY order. This should be combined with a keymap for proper symbol output. This allows symbols to be customized without rebuilding the firmware, as well as proper use of the actual Alt key.

### The rest of the Readme

I have not yet updated any other part of the Readme file.

## Checkout

The code depends on the Raspberry Pi Pico SDK, which is added as a submodule. Because the Pico SDK includes TinyUSB as a module, it is not recommended to do a recursive submodule init, and rather follow these steps:

    git clone https://github.com/solderparty/i2c_puppet
    cd i2c_puppet
    git submodule update --init
    cd 3rdparty/pico-sdk
    git submodule update --init

## Build

See the `boards` directory for a list of available boards.

    mkdir build
    cd build
    cmake -DPICO_BOARD=beepy -DCMAKE_BUILD_TYPE=Debug ..
    make

## Vendor USB Class

You can configure the software over USB in a similar way you would do it over I2C. You can access the same registers (like the backlight register) using the USB Vendor Class.
If you don't want to prefix all config interactions with `sudo`, you can copy the included udev rule:

    sudo cp etc/99-i2c_puppet.rules /lib/udev/rules.d/
    sudo udevadm control --reload
    sudo udevadm trigger

To interact with the internal registers of the keyboard over USB, use the `i2c_puppet.py` script included in the `etc` folder.
just import it, create a `I2C_Puppet` object, and you can interact with the keyboard in the same you would do using the I2C interface and the CircuitPython class linked below.

## Implementations

Here are libraries that allow I2C interaction with the boards running this software. Not all libraries might support all the features.

- [Arduino](https://github.com/solderparty/arduino_bbq10kbd)
- [CircuitPython](https://github.com/solderparty/arturo182_CircuitPython_BBQ10Keyboard)
- [Rust (Embedded-HAL)](https://crates.io/crates/bbq10kbd)
- [Feature-rich Linux Driver](https://github.com/wallComputer/bbqX0kbd_driver/)
- [Linux Kernel Module](https://github.com/billylindeman/bbq10kbd-kernel-driver)

## Protocol

The device uses I2C slave interface to communicate, the address can be configured in `app/config/conf_app.h`, the default is `0x1F`.

You can read the values of all the registers, the number of returned bytes depends on the register.
It's also possible to write to the registers, to do that, apply the write mask `0x80` to the register ID (for example, the backlight register `0x05` becomes `0x85`).

### The FW Version register (REG_VER = 0x01)

Data written to this register is discarded. Reading this register returns 1 byte, the first nibble contains the major version and the second nibble contains the minor version of the firmware.

### The configuration register (REG_CFG = 0x02)

This register can be read and written to, it's 1 byte in size.

This register is a bit map of various settings that can be changed to customize the behaviour of the firmware.

See `REG_CF2` for additional settings.

| Bit    | Name             | Description                                                        |
| ------ |:----------------:| ------------------------------------------------------------------:|
| 7      | CFG_USE_MODS     | Should Alt, Sym and the Shift keys modify the keys being reported. |
| 6      | CFG_REPORT_MODS  | Should Alt, Sym and the Shift keys be reported as well.            |
| 5      | CFG_PANIC_INT    | Currently not implemented.                                         |
| 4      | CFG_KEY_INT      | Should an interrupt be generated when a key is pressed.            |
| 3      | CFG_NUMLOCK_INT  | Should an interrupt be generated when Num Lock is toggled.         |
| 2      | CFG_CAPSLOCK_INT | Should an interrupt be generated when Caps Lock is toggled.        |
| 1      | CFG_OVERFLOW_INT | Should an interrupt be generated when a FIFO overflow happens.     |
| 0      | CFG_OVERFLOW_ON  | When a FIFO overflow happens, should the new entry still be pushed, overwriting the oldest one. If 0 then new entry is lost. |

Defaut value:
`CFG_OVERFLOW_INT | CFG_KEY_INT | CFG_USE_MODS`

### Interrupt status register (REG_INT = 0x03)

When an interrupt happens, the register can be read to check what caused the interrupt. It's 1 byte in size.

| Bit    | Name             | Description                                                 |
| ------ |:----------------:| -----------------------------------------------------------:|
| 7      | N/A              | Currently not implemented.                                  |
| 6      | INT_TOUCH        | The interrupt was generated by a trackpad motion.           |
| 5      | INT_GPIO         | The interrupt was generated by a input GPIO changing level. |
| 4      | INT_PANIC        | Currently not implemented.                                  |
| 3      | INT_KEY          | The interrupt was generated by a key press.                 |
| 2      | INT_NUMLOCK      | The interrupt was generated by Num Lock.                    |
| 1      | INT_CAPSLOCK     | The interrupt was generated by Caps Lock.                   |
| 0      | INT_OVERFLOW     | The interrupt was generated by FIFO overflow.               |

After reading the register, it has to manually be reset to `0x00`.

For `INT_GPIO` check the bits in `REG_GIN` to see which GPIO triggered the interrupt. The GPIO interrupt must first be enabled in `REG_GIC`.

### Key status register (REG_KEY = 0x04)

This register contains information about the state of the fifo as well as modified keys. It is 1 byte in size.

| Bit    | Name             | Description                                     |
| ------ |:----------------:| -----------------------------------------------:|
| 7      | N/A              | Currently not implemented.                      |
| 6      | KEY_NUMLOCK      | Is Num Lock on at the moment.                   |
| 5      | KEY_CAPSLOCK     | Is Caps Lock on at the moment.                  |
| 0-4    | KEY_COUNT        | Number of items in the FIFO waiting to be read. |

### Backlight control register (REG_BKL = 0x05)

Internally a PWM signal is generated to control the keyboard backlight, this register allows changing the brightness of the backlight. It is 1 byte in size, `0x00` being off and `0xFF` being the brightest.

Default value: `0xFF`.

### Debounce configuration register (REG_DEB = 0x06)

Currently not implemented.

Default value: 10

### Poll frequency configuration register (REG_FRQ = 0x07)

Currently not implemented.

Default value: 5

### Chip reset register (REG_RST = 0x08)

Reading or writing to this register will cause a SW reset of the chip.

### FIFO access register (REG_FIF = 0x09)

This register can be used to read the top of the key FIFO. It returns two bytes, a key state and a key code.

Possible key states:

| Value  | State                   |
| ------ |:-----------------------:|
| 1      | Pressed                 |
| 2      | Pressed and Held        |
| 3      | Released                |

### Secondary backlight control register (REG_BK2 = 0x0A)

Internally a PWM signal is generated to control a secondary backlight (for example, a screen), this register allows changing the brightness of the backlight. It is 1 byte in size, `0x00` being off and `0xFF` being the brightest.

Default value: `0xFF`.

### GPIO direction register (REG_DIR = 0x0B)

This register controls the direction of the GPIO expander pins, each bit corresponding to one pin. It is 1 byte in size.

The actual pin[7..0] to MCU pin assignment depends on the board, see `<board>.h` of the board for the assignments.

Any bit set to `1` means the GPIO is configured as input, any bit set to `0` means the GPIO is configured as output.

Default value: `0xFF` (all GPIOs configured as input)

### GPIO input pull enable register (REG_PUE = 0x0C)

If a GPIO is configured as an input (using `REG_DIR`), a optional pull-up/pull-down can be enabled.

This register controls the pull enable status, each bit corresponding to one pin. It is 1 byte in size.

The actual pin[7..0] to MCU pin assignment depends on the board, see `<board>.h` of the board for the assignments.

Any bit set to `1` means the input pull for that pin is enabled, any bit set to `0` means the input pull for that pin is disabled.

The direction of the pull is done in `REG_PUD`.

When a pin is configured as output, its bit in this register has no effect.

Default value: 0 (all pulls disabled)

### GPIO input pull direction register (REG_PUD = 0x0D)

If a GPIO is configured as an input (using `REG_DIR`), a optional pull-up/pull-down can be configured.

The pull functionality is enabled using `REG_PUE` and the direction of the pull is configured using this register, each bit corresponding to one pin. This register is 1 byte in size.

The actual pin[7..0] to MCU pin assignment depends on the board, see `<board>.h` of the board for the assignments.

Any bit set to `1` means the input pull is set to pull-up, any bit set to `0` means the input pul is set to pull-down.

When a pin is configured as output, its bit in this register has no effect.

Default value: `0xFF` (all pulls set to pull-up, if enabled in `REG_PUE` and set to input in `REG_DIR`)

### GPIO value register (REG_GIO = 0x0E)

This register contains the values of the GPIO Expander pins, each bit corresponding to one pin. It is 1 byte in size.

The actual pin[7..0] to MCU pin assignment depends on the board, see `<board>.h` of the board for the assignments.

If a pin is configured as an output (via `REG_DIR`), writing to this register will change the value of that pin.

Reading from this register will return the values for both input and output pins.

Default value: Depends on pin values

### GPIO interrupt config register (REG_GIC = 0x0F)

If a GPIO is configured as an input (using `REG_DIR`), an interrupt can be configured to trigger when the pin's value changes.

This register controls the interrupt, each bit corresponding to one pin.

The actual pin[7..0] to MCU pin assignment depends on the board, see `<board>.h` of the board for the assignments.

Any bit set to `1` means the input pin will trigger and interrupt when changing value, any bit set to `0` means no interrupt for that pin is triggered.

When an interrupt happens, the GPIO that triggered the interrupt can be determined by reading `REG_GIN`. Additionally, the `INT_GPIO` bit will be set in `REG_INT`.

Default value: `0x00`

### GPIO interrupt status register (REG_GIN = 0x10)

When an interrupt happens, the register can be read to check which GPIO caused the interrupt, each bit corresponding to one pin. This register is 1 byte in size.

The actual pin[7..0] to MCU pin assignment depends on the board, see `<board>.h` of the board for the assignments.

After reading the register, it has to manually be reset to `0x00`.

Default value: `0x00`

### Key hold threshold configuration (REG_HLD = 0x11)

This register can be read and written to, it is 1 byte in size.

The value of this register (expressed in units of 10ms) is used to determine if a "press and hold" state should be entered.

If a key is held down longer than the value, it enters the "press and hold" state.

Default value: 30 (300ms)

### Device I2C address (REG_ADR = 0x12)

The address that the device shows up on the I2C bus under. This register can be read and written to, it is 1 byte in size.

The change is applied as soon as a new value is written to the register. The next communication must be performed on the new address.

The address is not saved after a reset.

Default value: `0x1F`

### Interrupt duration (REG_IND = 0x13)

The value of this register determines how long the INT/IRQ pin is held LOW after an interrupt event happens.This register can be read and written to, it is 1 byte in size.

The value of this register is expressed in ms.

Default value: 1 (1ms)

### The configuration register 2 (REG_CF2 = 0x14)

This register can be read and written to, it's 1 byte in size.

This register is a bit map of various settings that can be changed to customize the behaviour of the firmware.

See `REG_CFG` for additional settings.

| Bit    | Name             | Description                                                        |
| ------ |:----------------:| ------------------------------------------------------------------:|
| 7      | N/A              | Currently not implemented.                                         |
| 6      | N/A              | Currently not implemented.                                         |
| 5      | N/A              | Currently not implemented.                                         |
| 4      | N/A              | Currently not implemented.                                         |
| 3      | N/A              | Currently not implemented.                                         |
| 2      | CF2_USB_MOUSE_ON | Should trackpad events be sent over USB HID.                       |
| 1      | CF2_USB_KEYB_ON  | Should key events be sent over USB HID.                            |
| 0      | CF2_TOUCH_INT    | Should trackpad events generate interrupts.                        |

Default value: `CF2_TOUCH_INT | CF2_USB_KEYB_ON | CF2_USB_MOUSE_ON`

### Trackpad X Position(REG_TOX = 0x15)

This is a read-only register, it is 1 byte in size.

Trackpad X-axis position delta since the last time this register was read.

The value reported is signed and can be in the range of (-128 to 127).

When the value of this register is read, it is afterwards reset back to 0.

It is recommended to read the value of this register often, or data loss might occur.

Default value: 0

### Trackpad Y position (REG_TOY = 0x16)

This is a read-only register, it is 1 byte in size.

Trackpad Y-axis position delta since the last time this register was read.

The value reported is signed and can be in the range of (-128 to 127).

When the value of this register is read, it is afterwards reset back to 0.

It is recommended to read the value of this register often, or data loss might occur.

Default value: 0

### LED RGB values (REG_LED_R = 0x21, REG_LED_G = 0x22, REG_LED_B = 0x23)

These registers can be read and written to, each are 1 byte in size.

Set the red/green/blue values between 0-255 `0x00 - 0xFF`

### LED On/Off (REG_LED = 0x20)

This register can be read and written to, it is 1 byte in size.

`0x00` is off, any other value is on

Default value: 0

### Update Target Platform (REG_UPDATE_TARGET = 0x31)

Starting with Beepy firmware 3.1, you can select the target platform for firmware updates. This register can be read and written to, it's 1 byte in size.

Possible values:
- `UPDATE_TARGET_RP2040 = 0x01` - Target RP2040 (default)
- `UPDATE_TARGET_ESP32 = 0x02` - Target ESP32

The selected target platform determines which device will be updated when writing to `REG_UPDATE_DATA`.

### ESP32 Status Register (REG_ESP32_STATUS = 0x32)

This register provides status information about the connected ESP32. It is read-only and returns 1 byte.

| Bit | Name            | Description                                           |
| --- |:---------------:| -----------------------------------------------------:|
| 0   | ESP32_INIT      | ESP32 communication is initialized                    |
| 1   | ESP32_CONNECTED | ESP32 is connected and responding to commands         |
| 2-7 | Reserved        | Reserved for future use                               |

### ESP32 Command Register (REG_ESP32_COMMAND = 0x33)

This register allows sending commands directly to the ESP32. It is write-only, 1 byte in size.

Commands will only be processed if the ESP32 is connected (check `REG_ESP32_STATUS`).

### Firmware update (REG_UPDATE_DATA = 0x30)

Starting with Beepy firmware 3.0, firmware is loaded in two stages.

For RP2040 updates:
- The first stage is a modified version of [pico-flashloader](https://github.com/rhulme/pico-flashloader).
- It allows updates to be flashed to the second stage firmware while booted.
- The second stage is the actual Beepy firmware.

For ESP32 updates:
- The ESP32 uses its built-in OTA update mechanism.
- The RP2040 acts as a bridge, forwarding update data to the ESP32 over UART.

Reading `REG_UPDATE_DATA` will return an update status code:

- `UPDATE_OFF = 0`
- `UPDATE_RECV = 1`
- `UPDATE_FAILED = 2`
- `UPDATE_FAILED_LINE_OVERFLOW = 3`
- `UPDATE_FAILED_FLASH_EMPTY = 4`
- `UPDATE_FAILED_FLASH_OVERFLOW = 5`
- `UPDATE_FAILED_BAD_LINE = 6`
- `UPDATE_FAILED_BAD_CHECKSUM = 7`
- `UPDATE_FAILED_ESP32_COMM_ERROR = 8`
- `UPDATE_FAILED_UNSUPPORTED_PLATFORM = 9`
- `UPDATE_ESP32_AWAITING_REBOOT = 10`

Firmware updates are flashed by writing byte-by-byte to `REG_UPDATE_DATA`:

- Header line beginning with `+` e.g. `+Beepy` for RP2040 or `+ESP32` for ESP32
- Followed by the contents of an image in Intel HEX format

By default, `REG_UPDATE_DATA` will be set to `UPDATE_OFF`.
After writing, `REG_UPDATE_DATA` will be set to `UPDATE_RECV` if more data is expected.

If the update completes successfully:

For RP2040 updates:
- `REG_UPDATE_DATA` will be set to `UPDATE_OFF`
- Shutdown signal will be sent to the Pi
- Delay to allow the Pi to cleanly shut down before poweroff (configurable with `REG_SHUTDOWN_GRACE`)
- Firmware is flashed and the system is reset

For ESP32 updates:
- `REG_UPDATE_DATA` will be set to `UPDATE_ESP32_AWAITING_REBOOT`
- ESP32 will reboot with the new firmware

Please wait until the system reboots on its own before removing power.

If the update failed, `REG_UPDATE_DATA` will contain an error code and the firmware will not be modified.

The header line `+...` will reset the update process, so an interrupted or failed update can be retried by restarting the firmware write.
