idf_component_register(SRCS "hdw-usb.c" "advanced_usb_control.c"
                    INCLUDE_DIRS "include" "."
                    REQUIRES driver esp_tinyusb tinyusb spi_flash)
