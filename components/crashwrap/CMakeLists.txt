idf_component_register(SRCS "crashwrap.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES hdw-nvs
                    REQUIRES nvs_flash esp_system)
