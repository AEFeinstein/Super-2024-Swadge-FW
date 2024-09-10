REM Make sure the tools are built first
make -C .\tools\reboot_into_bootloader\ reboot_swadge_into_bootloader.exe
make -C .\tools\bootload_reboot_stub\ bootload_reboot_stub.bin
make -C .\tools\swadgeterm\

REM Reboot the swadge into bootloader mode
.\tools\reboot_into_bootloader\reboot_swadge_into_bootloader.exe || true
REM Flash the swadge
idf.py flash
sleep 1
REM Reboot the swadge again
python -m esptool --before no_reset --after no_reset load_ram tools\bootload_reboot_stub\bootload_reboot_stub.bin || true
REM Open a serial monitor
make -C tools\swadgeterm monitor
