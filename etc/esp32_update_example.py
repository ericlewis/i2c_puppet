#!/usr/bin/env python3
"""
Example script to update ESP32 firmware using the I2C Puppet's ESP32 update mechanism.

This script demonstrates how to:
1. Select the ESP32 as the target platform
2. Check if ESP32 is connected
3. Send firmware update data
4. Monitor update progress

Prerequisites:
- I2C Puppet firmware v3.1 or higher
- ESP32 connected to the I2C Puppet via UART
- ESP32 firmware image in Intel HEX format
"""

import time
import sys
from i2c_puppet import I2C_Puppet

# Registers
REG_UPDATE_TARGET = 0x31
REG_ESP32_STATUS = 0x32
REG_UPDATE_DATA = 0x30

# Update status codes
UPDATE_OFF = 0
UPDATE_RECV = 1
UPDATE_FAILED = 2
UPDATE_FAILED_LINE_OVERFLOW = 3
UPDATE_FAILED_FLASH_EMPTY = 4
UPDATE_FAILED_FLASH_OVERFLOW = 5
UPDATE_FAILED_BAD_LINE = 6
UPDATE_FAILED_BAD_CHECKSUM = 7
UPDATE_FAILED_ESP32_COMM_ERROR = 8
UPDATE_FAILED_UNSUPPORTED_PLATFORM = 9
UPDATE_ESP32_AWAITING_REBOOT = 10

# Target platforms
UPDATE_TARGET_RP2040 = 0x01
UPDATE_TARGET_ESP32 = 0x02

def update_esp32_firmware(puppet, hex_file_path):
    """
    Update ESP32 firmware using a HEX file
    
    Args:
        puppet: I2C_Puppet instance
        hex_file_path: Path to Intel HEX format firmware file
    
    Returns:
        bool: True if update successful, False otherwise
    """
    # Select ESP32 as target platform
    puppet.write_register(REG_UPDATE_TARGET, UPDATE_TARGET_ESP32)
    print(f"Selected target platform: ESP32")
    
    # Check if ESP32 is connected
    esp32_status = puppet.read_register(REG_ESP32_STATUS)
    if esp32_status & 0x03 != 0x03:
        print(f"ESP32 not connected (status: 0x{esp32_status:02X})")
        return False
    
    print(f"ESP32 connected (status: 0x{esp32_status:02X})")
    
    try:
        # Read HEX file
        with open(hex_file_path, 'r') as f:
            hex_data = f.readlines()
    except Exception as e:
        print(f"Error reading HEX file: {e}")
        return False
    
    # Start update with header
    puppet.write_register(REG_UPDATE_DATA, ord('+'))
    for c in "ESP32":
        puppet.write_register(REG_UPDATE_DATA, ord(c))
    puppet.write_register(REG_UPDATE_DATA, ord('\n'))
    
    # Check status
    status = puppet.read_register(REG_UPDATE_DATA)
    if status != UPDATE_RECV:
        print(f"Failed to start update, status: {status}")
        return False
    
    print("Starting firmware update...")
    
    # Send firmware data line by line
    line_count = 0
    for line in hex_data:
        line = line.strip()
        if not line:
            continue
        
        # Send line character by character
        for c in line:
            puppet.write_register(REG_UPDATE_DATA, ord(c))
        
        # Send line terminator
        puppet.write_register(REG_UPDATE_DATA, ord('\n'))
        
        # Check status periodically
        line_count += 1
        if line_count % 10 == 0:
            status = puppet.read_register(REG_UPDATE_DATA)
            print(f"Progress: {line_count} lines, status: {status}")
            
            if status != UPDATE_RECV:
                if status == UPDATE_OFF or status == UPDATE_ESP32_AWAITING_REBOOT:
                    # Update complete
                    break
                else:
                    # Error occurred
                    print(f"Update failed with status: {status}")
                    return False
    
    # Final status check
    status = puppet.read_register(REG_UPDATE_DATA)
    print(f"Final status: {status}")
    
    if status == UPDATE_ESP32_AWAITING_REBOOT:
        print("ESP32 firmware update completed successfully, ESP32 is rebooting")
        return True
    elif status == UPDATE_OFF:
        print("ESP32 firmware update completed successfully")
        return True
    else:
        print(f"Update failed with status: {status}")
        return False


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <hex_file>")
        sys.exit(1)
    
    hex_file = sys.argv[1]
    
    try:
        # Initialize I2C Puppet
        puppet = I2C_Puppet()
        puppet.start()
        
        # Update ESP32 firmware
        success = update_esp32_firmware(puppet, hex_file)
        
        if success:
            print("ESP32 firmware update completed successfully")
            sys.exit(0)
        else:
            print("ESP32 firmware update failed")
            sys.exit(1)
            
    except KeyboardInterrupt:
        print("Update cancelled")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()