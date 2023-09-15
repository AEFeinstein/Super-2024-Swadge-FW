# tools

This folder has tools used for development. Each has it's own `README.md` if you want to read more

## [`spiffs_file_preprocessor`](./spiffs_file_preprocessor)

`spiffs_file_preprocessor` is a C program which takes assets like text, PNG images, or font files and processes them into compressed, embedded friendly formats, like WSG. It is used by the build system to process files in the `assets` folder into the `spiffs_image` folder, which is built into the firmware as a SPIFFS partition.

## [`font_maker`](./font_maker)

`font_maker` is a C program which takes a TrueType font and renders it into a `.font.png` file.
This file can be given to `spiffs_file_preprocessor` to flash to the Swadge and then be used to draw text to the display.

## [`pyFlashGui`](./pyFlashGui)

`pyFlashGui` is a Python GUI program which is used to program Swadges during manufacturing. It spins around and programs Swadges as they are connected to the host computer over USB.

## `monitor_emu_wifi.py`

`monitor_emu_wifi.py` is a Python command-line program which listens for emulated ESPNOW packets and prints them for debugging purposes.
