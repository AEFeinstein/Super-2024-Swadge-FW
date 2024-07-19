# Compatibility

This Python script should be compatible with Python 2 & 3. I have tested it on Windows with Python 3.

# Summary

This Python script polls for USB serial ports which match the VID & PID of an ESP32-S2. When one is found, it will attempt to flash it using `esptool`.

The UI will be green when a Swadge is successfully flashed. It will be red if the flash failed.

# Driver Dependencies

## Windows

Flashing an ESP32-S2 over USB on Windows requires a specific driver to be installed. This driver only needs to be installed once. Instructions can be found in the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/v4.4.3/esp32s2/api-guides/dfu.html#usb-drivers-windows-only) and are copied below:

> `dfu-util` uses _libusb_ to access the device. You have to register on Windows the device with the WinUSB driver. Please see the [libusb wiki](https://github.com/libusb/libusb/wiki/Windows#How_to_use_libusb_on_Windows) for more details.
>
> The drivers can be installed by the [Zadig tool](https://zadig.akeo.ie/). Please make sure that the device is in download mode before running the tool and that it detects the ESP32-S2 device before installing the drivers. The Zadig tool might detect several USB interfaces of ESP32-S2. Please install the WinUSB driver for only that interface for which there is no driver installed (probably it is Interface 2) and don’t re-install the driver for the other interface.
>
> **Warning**
> The manual installation of the driver in Device Manager of Windows is not recommended because the flashing might not work properly.

## Linux

The ESP-IDF Programming Guide also [specifies a udev rule for Linux](https://docs.espressif.com/projects/esp-idf/en/v4.4.3/esp32s2/api-guides/dfu.html#udev-rule-linux-only):

> udev is a device manager for the Linux kernel. It allows us to run `dfu-util` (and `idf.py dfu-flash`) without `sudo` for gaining access to the chip.
> 
> Create file `/etc/udev/rules.d/40-dfuse.rules` with the following content:
> 
>
> ```
> SUBSYSTEMS=="usb", ATTRS{idVendor}=="1209", ATTRS{idProduct}=="4269", GROUP="plugdev", MODE="0666"
> ```
> **Note** Please check the output of command `groups`. The user has to be a member of the _GROUP_ specified above. You may use some other existing group for this purpose (e.g. _uucp_ on some systems instead of _plugdev_) or create a new group for this purpose.
> 
> Restart your computer so the previous setting could take into affect or run `sudo udevadm trigger` to force manually udev to trigger your new rule.

# Script Dependencies

This script requires ``python3`` which can be installed from https://www.python.org/downloads/.

The following Python modules must be installed:
| Module | URL | PIP |
| ------------- | ------------- | ------------- |
| ``tkinter`` | https://tkdocs.com/tutorial/install.html | Tkinter (and, since Python 3.1, ttk, the interface to the newer themed widgets) is included in the Python standard library. | 
| ``pyserial`` | https://pyserial.readthedocs.io/en/latest/pyserial.html | ```pip install pyserial```| 
| ``esptool`` | https://docs.espressif.com/projects/esptool/en/latest/esp32s2/installation.html | ```pip install esptool``` |

For the script to actually flash firmware, the following files must be in the same directory as ``pyFlashGui.py``:

| File | Relative Path |
| ------------- | ------------- |
| ``bootloader.bin`` | ``../build/bootloader/bootloader.bin`` |
| ``swadge2024.bin`` | ``../build/swadge2024.bin`` |
| ``partition-table.bin`` | ``../build/partition_table/partition-table.bin`` |
| ``storage.bin`` | ``../build/storage.bin`` |

Assuming you've checked out the whole repository, set up the IDF, and are in the `Super-2023-Swadge-FW` folder, then this should build and place the files:

```bash
idf.py build
cp ./build/bootloader/bootloader.bin ./build/swadge2024.bin ./build/partition_table/partition-table.bin ./build/storage.bin ./pyFlashGui/
```

# Instructions

1. Download and install the dependencies
1. Download the final Swadge firmware from the Releases tab: https://github.com/AEFeinstein/Super-2023-Swadge-FW/releases/
1. Run the programmer script (``python3 pyFlashGui.py`` from a terminal)
1. Switch the Swadge's power to USB (right position)
1. Hold the PGM button down (Up on the directional pad)
1. Plug the Swadge into the computer with a USB-C cable
1. Wait for the GUI to flash green, then you're done.

An instructional video can be found here: https://youtu.be/AdtCOf92yiQ

# Kiosk Instructions

1. Switch the Swadge's power switch to USB (right position)
1. Press and hold the directional-pad up button (Should say "PGM" directly above it)
1. Plug the Swadge into the computer with the USB-C cable
1. Release the up button
1. Wait for the window on the computer screen to turn green
1. Unplug the Swadge
1. You're done! Switch the Swadge back to BAT (left position) to turn it on and check out the cool new features!
