# Swadge-IDF-5.0
Migrating the Swadge to ESP IDF 5.0, one component at a time

## Notes

In the IDF, need to wrap the contents of `esp_netif_lwip_ppp.h` with `#if defined(CONFIG_ESP_NETIF_TCPIP_LWIP) ... #endif`