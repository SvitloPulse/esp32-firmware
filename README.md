# Svitlo Pulse firmware: ESP32-C3 based sensor for Svitlobot (Work In Progress)

EN | [UA](./README-UA.md)

This is the firmware for the simple mains sensor, built using ESP32-C3 Super Mini board. 
It integrates with https://svitlobot.in.ua/ - a free volunteer project for electricity state monitoring. 
The device sends a ping to Svitlobot Service every minute. When mains goes off, the device stops sending ping, thus Svitlobot detects power outages after some time.

## How to use it?

1. Buy a ESP32-C3 Super Mini board.
1. Flash it with released firmware (see Releases page, coming soon) either with Svitlo Pulse Web Installer (coming soon), [ESPHome Web Installer](https://web.esphome.io/) or other preferred method.
    - Alternatively, you can build and flash it yourself, see below.
1. Configure it before flashing in Svitlo Pulse Web Installer (coming soon) or at runtime using Android app.
1. Connect it to 5V USB adapter, typically used for smartphone charging, and that's it!

## Build instructions

1. Install VSCode IDE
1. Install Platformio VSCode plugin
1. Clone this repo
1. Open the repo folder in VSCode
1. (Optional) Adjust build-time firmware configuration, see below.
1. Use Platformio project tasks to build the firmware

### Build-time configuration

For different reasons you may want to customize the defaults firmware runs with.
For example, it might be hardcoding the WiFI credentials and / or Svitlobot key.

Please see the instructions and config parameters in ``./env.example`` file.
Alternatively, you can customize the same values in ``./src/defconfig.hpp``.
