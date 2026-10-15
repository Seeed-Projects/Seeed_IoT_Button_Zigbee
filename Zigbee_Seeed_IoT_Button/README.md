# Zigbee Implementation for Seeed Studio IoT Button

This directory contains the Zigbee implementation for the Seeed Studio IoT Button, which is built with the ESP32-C6 MCU.

## Prerequisites

- ESP32-Arduino board package version 3.1.3
- Seeed Studio IoT Button (based on ESP32-C6)
- Arduino IDE or compatible development environment

## Installation

1. **Install Required Libraries**:
   - Make sure you have ESP32-Arduino board package version 3.1.3 installed
   - In Arduino IDE: Tools → Board → Boards Manager → Search for "esp32" and install version 3.1.3

2. **Set Up Zigbee Library Files**:
   - Copy `ZigbeeMultiButton.h` and `ZigbeeMultiButton.cpp` to the Arduino Zigbee endpoint directory:
     ```
     <Arduino_Installation_Path>/libraries/Zigbee/src/ep/
     ```
   - Edit `<Arduino_Installation_Path>/libraries/Zigbee/src/Zigbee.h` to include the new header file:
     ```cpp
     // Add this line with the other endpoint includes
     #include "ep/ZigbeeMultiButton.h"
     ```

3. **Configure Arduino IDE**:
   - Select the correct board: Tools → Board → ESP32 Arduino → ESP32-C6 Dev Module
   - Enable Zigbee Mode: Tools → Zigbee mode → Zigbee Endpoint
   - Set Partition Scheme: Tools → Partition Scheme → Zigbee 4MB with spiffs
   - For other settings, you can use the defaults

## Using Pre-built Binary Files

For convenience, pre-built binary files are provided in the `build/factory_firmware_zigbee` folder. You can flash these directly to your Seeed IoT Button without compiling the code yourself.

1. **Required Tools**:
   - Install [esptool.py](https://github.com/espressif/esptool): `pip install esptool`

2. **Flash the Binary Files**:
   - Connect your Seeed IoT Button via USB
   - Navigate to the `build/factory_firmware_zigbee` directory
   - Run the following command (also available in `esptool_flash_command.txt`):
   
   ```
   esptool.py --chip esp32c6 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode keep --flash_freq keep --flash_size keep 0x0 "Zigbee_Seeed_IoT_Button.bootloader.bin" 0x8000 "Zigbee_Seeed_IoT_Button.partitions.bin" 0xe000 "boot_app0.bin" 0x10000 "Zigbee_Seeed_IoT_Button.bin"
   ```

   - Alternatively, you can use the merged binary file for a simpler command:
   
   ```
   esptool.py --chip esp32c6 --baud 921600 --before default_reset --after hard_reset write_flash 0x0 "Zigbee_Seeed_IoT_Button.merged.bin"
   ```

3. **After Flashing**:
   - The device will restart automatically
   - Proceed to the pairing steps in the Usage section below

## Usage

1. **Upload the Sketch**:
   - Open `Zigbee_Seeed_IoT_Button.ino` in Arduino IDE
   - Connect your Seeed IoT Button via USB
   - Click Upload

2. **Pairing with Zigbee Coordinator**:
   - After uploading, the device will automatically attempt to join a Zigbee network
   - Put your Zigbee coordinator (Home Assistant, Zigbee2MQTT, etc.) in pairing mode
   - The IoT Button should be discovered as a "Seeed Studio IoT_Button" device

3. **Factory Reset**:
   - If you need to reset the device to pair with a different network, perform a long press (hold for more than 1 second)
   - The device will reset its Zigbee settings and reboot

## Button Actions

The IoT Button supports three different actions that are reported to your Zigbee network:

1. **Single Click**: A quick press and release of the button
   - Reported as a "click_1" event in Zigbee
   - Can be used to toggle lights, activate scenes, etc.

2. **Double Click**: Two presses with at most 600ms between the first release and the second press (default)
   - Reported as a "click_2" event in Zigbee
   - Can be used for different actions than single click

3. **Long Press**: Press and hold for more than 1 second
   - Triggers a factory reset of the Zigbee settings
   - Note: This is currently hardcoded to perform a factory reset

## Integration with Home Assistant

When paired with Home Assistant (via ZHA or Zigbee2MQTT):

1. The button will appear as a device with multiple triggers
2. You can create automations based on the different button actions:
   - Single click trigger
   - Double click trigger

Example automation in Home Assistant:

```yaml
automation:
  - alias: "IoT Button Single Click - Toggle Living Room Light"
    trigger:
      platform: device
      domain: mqtt
      device_id: [YOUR_DEVICE_ID]
      type: action
      subtype: click_1
    action:
      service: light.toggle
      target:
        entity_id: light.living_room
```

## Customization

Button timing lives at the top of `Zigbee_Seeed_IoT_Button.ino`. Change a value, rebuild, and flash:

| Define | Default | Meaning |
| --- | --- | --- |
| `DEBOUNCE_TIME` | 20 ms | The pin must stay at the new level this long before a press or release is accepted |
| `MIN_PRESS_TIME` | 30 ms | Shortest stable press that counts as a click |
| `DOUBLE_CLICK_TIME` | 600 ms | Maximum gap from the first release to the second press |
| `LONG_PRESS_TIME` | 1000 ms | Hold duration that starts a long press / factory reset |
| `BUTTON_POLL_INTERVAL` | 10 ms | How often `loop()` reads the pin |

A single click is reported after `DOUBLE_CLICK_TIME` with no second press, so a larger window gives more time for a second tap and a slightly later single-click report.

You can also:

- Modify the long press behavior (currently set to factory reset)
- Add additional button patterns in `handleButtonEvent`

### Host tests for timing

The gesture detector in `ButtonInput.cpp` can be exercised on a desktop without hardware:

```
g++ -std=c++11 -I Zigbee_Seeed_IoT_Button test/test_button_input.cpp Zigbee_Seeed_IoT_Button/ButtonInput.cpp -o /tmp/test_button_input
/tmp/test_button_input
```

A passing run prints `passed` counts and exits 0.

## Troubleshooting

- **Device Not Joining Network**: Make sure your Zigbee coordinator is in pairing mode and within range
- **Button Not Responding**: Check the wiring of the button to GPIO9
- **Serial Output**: Connect to the serial monitor at 115200 baud to see debug information
- **Compilation Errors**: Ensure you've correctly installed the endpoint files and updated Zigbee.h 