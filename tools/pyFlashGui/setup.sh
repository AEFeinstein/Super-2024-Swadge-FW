#!/bin/sh -e
echo 'SUBSYSTEMS=="usb", ATTRS{idVendor}=="303a", ATTRS{idProduct}=="00??", GROUP="plugdev", MODE="0666"' > /etc/udev/rules.d/40-dfuse.rules

pip3 install pyserial esptool
