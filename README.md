# Seeed Studio IoT Button

This repository contains the Zigbee implementation for the Seeed Studio IoT Button. For the ESPHome implementation, please visit [xiao-esphome-projects](https://github.com/Seeed-Studio/xiao-esphome-projects/tree/main/projects/seeedstudio-iot-button).

## Project Overview

The Seeed Studio IoT Button is a versatile IoT device that can be integrated with smart home systems. This repository provides the Zigbee implementation for integration with any Zigbee-compatible hub.

## Features

- Multiple button actions: single click, double click, and long press
- Configurable debounce and click timing in `Zigbee_Seeed_IoT_Button.ino`
- Easy integration with Zigbee networks
- Factory reset capability via long press

## Zigbee Implementation

The Zigbee implementation (`Zigbee_Seeed_IoT_Button/`) includes:

- Arduino sketch for ESP32-C6 with Zigbee capability
- Button event detection (single click, double click, long press)
- Zigbee endpoint configuration for smart home integration
- Factory reset capability via long press

## Installation

You can install the firmware directly to your device using the web installer available at the project's GitHub Pages site.

## Development

To modify the firmware:

1. For Zigbee: Modify the Arduino sketch in the `Zigbee_Seeed_IoT_Button` directory

## CI/CD

This project uses GitHub Actions to:
- Build the firmware automatically
- Deploy the web installer to GitHub Pages
- Run continuous integration tests

## License

MIT
