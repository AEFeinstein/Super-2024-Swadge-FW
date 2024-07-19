#!/bin/sh -e
echo 'SUBSYSTEMS=="usb", ATTRS{idVendor}=="1209", ATTRS{idProduct}=="4269", GROUP="plugdev", MODE="0666"' > /etc/udev/rules.d/40-dfuse.rules

pip3 install pyserial esptool
