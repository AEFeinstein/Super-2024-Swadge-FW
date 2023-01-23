idf_component_register(SRCS "swadge2024.c"
                    PRIV_REQUIRES hdw-accel
                    PRIV_REQUIRES hdw-btn
                    PRIV_REQUIRES hdw-bzr
                    PRIV_REQUIRES hdw-led
                    PRIV_REQUIRES hdw-mic                    
                    PRIV_REQUIRES hdw-tft
                    REQUIRES esp_timer spi_flash
                    INCLUDE_DIRS ".")
