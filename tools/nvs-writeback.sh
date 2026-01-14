#!/bin/bash

PORT=/dev/ttyACM0
OPTS="--chip esp32s2 --port $PORT -b 2000000 --before no_reset --after no_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 4MB"

# esptool.py $OPTS 0x1000 fb_bootloader.bin
# esptool.py $OPTS 0x8000 fb_partition-table.bin
esptool.py $OPTS 0x9000 fb_nvs.bin
# esptool.py $OPTS 0xf000 fb_phy.bin
# esptool.py $OPTS 0x10000 fb_app.bin

# Flashing param addresses
# "--flash-size", "4MB",
# "0x1000",  "bootloader.bin",
# "0x8000",  "partition-table.bin",
# "0x10000", "swadge2024.bin"

# Partition table addresses
# nvs,      data, nvs,     0x9000,  0x6000,
# phy_init, data, phy,     0xf000,  0x1000,
# factory,  app,  factory, 0x10000, 0x3F0000,
