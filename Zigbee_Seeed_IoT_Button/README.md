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

The IoT Button supports three actions. The first two are sent over Zigbee as **IAS Zone** status on endpoint 10. Home Assistant ZHA shows that as a two-state entity (**Off** / **On**).

1. **Single Click**: A quick press and release
   - Zigbee IAS Zone status `0` (alarms clear)
   - Home Assistant: **Off** / **Turned off**

2. **Double Click**: Two quick presses within 400ms
   - Zigbee IAS Zone status with alarm bits set
   - Home Assistant: **On** / **Turned on**

3. **Long Press**: Press and hold for more than 1 second
   - Factory-resets Zigbee settings and reboots
   - This is a local reset, not a third Home Assistant trigger

Step-by-step pairing and automation examples: [docs/home-assistant.md](../docs/home-assistant.md).

## Integration with Home Assistant

After pairing with **ZHA** or **Zigbee2MQTT**, open the **Seeed Studio IoT_Button** device. You should see one two-state entity. Build automations from state changes:

- Single click → entity becomes `off`
- Double click → entity becomes `on`

Example (ZHA, YAML). Replace the entity ids with the ones on your device page:

```yaml
automation:
  - alias: "IoT Button single click - toggle living room light"
    trigger:
      - platform: state
        entity_id: binary_sensor.seeed_studio_iot_button
        to: "off"
    action:
      - service: light.toggle
        target:
          entity_id: light.living_room
```

UI path: **Settings → Automations & Scenes → Create Automation → Device → Seeed Studio IoT_Button**, then **Turned off** (single click) or **Turned on** (double click).

The [Seeed wiki automation section](https://wiki.seeedstudio.com/seeed_iot_button_with_zigbee/#step-8-create-automations-in-home-assistant) describes a later firmware with virtual switches and “Single press” / “Double press” device actions. This repository’s `Zigbee_Seeed_IoT_Button.merged.bin` uses the two-state IAS Zone mapping above. Full walkthrough: [docs/home-assistant.md](../docs/home-assistant.md).

## Customization

You can modify the button behavior by editing the `Zigbee_Seeed_IoT_Button.ino` file:

- Change timing parameters for button actions
- Modify the long press behavior (currently set to factory reset)
- Add additional button patterns

## Troubleshooting

- **Device Not Joining Network**: Make sure your Zigbee coordinator is in pairing mode and within range
- **Button Not Responding**: Check the wiring of the button to GPIO9
- **Home Assistant only lists On/Off (or Turned on/Turned off)**: That is how this firmware appears. Map single click to Off and double click to On; see [docs/home-assistant.md](../docs/home-assistant.md)
- **Serial Output**: Connect to the serial monitor at 115200 baud to see debug information
- **Compilation Errors**: Ensure you've correctly installed the endpoint files and updated Zigbee.h 