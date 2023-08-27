make -C tools/reboot_into_bootloader || true
sleep 1.2
idf.py flash
sleep 1.2
make -C tools/bootload_reboot_stub reboot
sleep 2.5
make -C tools/swadgeterm monitor
