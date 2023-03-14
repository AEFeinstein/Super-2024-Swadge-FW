Meaningless change

# Swadge-IDF-5.0
Migrating the Swadge to ESP IDF 5.0.1, one component at a time

## Documentation

Full Doxygen documentation can be found at https://adam.feinste.in/Swadge-IDF-5.0/

## Continuous Integration

This project uses Github Actions to automatically build the firmware any time a change is pushed to main.

![Build Firmware and Emulator](https://github.com/AEFeinstein/Swadge-IDF-5.0/actions/workflows/build-firmware-and-emulator.yml/badge.svg)

## Dependencies

* [ESP-IDF](https://github.com/espressif/esp-idf) to build the firmware
* [Doxygen](https://www.doxygen.nl/download.html) to build the documentation
* [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to format the code
* [find](https://www.gnu.org/software/findutils/manual/html_mono/find.html) to help the makefile

## Notes

In the IDF, need to wrap the contents of `esp_netif_lwip_ppp.h` with `#if defined(CONFIG_ESP_NETIF_TCPIP_LWIP) ... #endif`
