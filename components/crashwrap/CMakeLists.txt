idf_component_register(SRCS "crashwrap.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES main
                    REQUIRES nvs_flash esp_system)
