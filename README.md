# Swadge-IDF-5.0
Migrating the Swadge to ESP IDF 5.0.1, one component at a time

## Dependencies

* [ESP-IDF](https://github.com/espressif/esp-idf) to build the firmware
* [Doxygen](https://www.doxygen.nl/download.html) to build the documentation
* [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to format the code
* [find](https://www.gnu.org/software/findutils/manual/html_mono/find.html) to help the makefile

## Notes

In the IDF, need to wrap the contents of `esp_netif_lwip_ppp.h` with `#if defined(CONFIG_ESP_NETIF_TCPIP_LWIP) ... #endif`