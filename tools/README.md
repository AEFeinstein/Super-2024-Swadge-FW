# Tools

This folder has tools used for development. Some tools have their own `README.md` if you want to read more.

## Level Editing

- [`breakout_editor`](./breakout_editor) is a tool used to create levels for Galactic Brickdown. It uses the [Tiled](https://www.mapeditor.org/) editor.
- [`lumber_jacks`](./lumber_jacks) is a tool used to create levels for Lumber Jack Panic & Attack. It uses the [Tiled](https://www.mapeditor.org/) editor.
- [`platformer_editor`](./platformer_editor) is a tool used to create levels for Swadge Land. It uses the [Tiled](https://www.mapeditor.org/) editor.
- [`rayMapEditor`](./rayMapEditor) is a Python tool used to create levels for Magtroid Pocket.

## Asset Processing

- [`assets_preprocessor`](./assets_preprocessor) is a C program which takes assets like text, PNG images, or font files and processes them into compressed, embedded friendly formats, like WSG. It is used by the build system to process files in the `assets` folder into the `assets_image` folder, which is built into the firmware by `cnfs_gen`.
- [`font_maker`](./font_maker) is a C program which takes a TrueType font and renders it into a `.font.png` file. This file can be given to `assets_preprocessor` to flash to the Swadge and then be used to draw text to the display.
- [`3dmodelheadermaker`](./3dmodelheadermaker) is used to process 3D models for usage in the Flight Sim game.
- [`sprite-tinter`](./sprite-tinter) is used to tint sprites (specifically the Boss) for Magtroid Pocket.

## Flashing

- [`pyFlashGui`](./pyFlashGui) is a Python GUI program which is used to program Swadges during manufacturing. It spins around and programs Swadges as they are connected to the host computer over USB.
- [`reboot_into_bootloader`](./reboot_into_bootloader) is a tool used to reboot a Swadge into bootloader mode over USB so that it can be flashed. It is used by `reflash_and_monitor.bat`
- [`bootload_reboot_stub`](./bootload_reboot_stub) is a tool used to reboot a Swadge over USB after flashing. It is used by `reflash_and_monitor.bat`
- [`reflash_and_monitor.bat`](./reflash_and_monitor.bat) is a Windows batch file to automatically reboot a USB-connected Swadge into bootloader mode, flash it, reboot it back to normal mode, and open a serial monitor.

## Monitoring

- [`swadgeterm`](./swadgeterm) is a tool to monitor serial output from a Swadge over USB. It is used by `reflash_and_monitor.bat`.
- [`monitor_emu_wifi.py`](./monitor_emu_wifi.py) is a Python command-line program which listens for emulated ESPNOW packets and prints them for debugging purposes.

## Experimenting

- [`hidapi.c`](./hidapi.c) & [`hidapi.h`](./hidapi.h) is a Multi-Platform library for communication with HID devices. This is used by other tools, like `hidapi_test`, `reboot_into_bootloader`, `sandbox_test`, and `swadgeterm`.
- [`hidapi_test`](./hidapi_test) tests something with the Swadge as a USB HID device (gamepad mode).
- [`sandbox_test`](./sandbox_test) is [cnlohr's](https://github.com/cnlohr) sandbox for all sorts of tests and experiments. It can load executable code over USB while the Swadge is running rather than reflash the ESP32-S2.
