idf_component_register(SRCS "hdw-nvs.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_flash
                    PRIV_REQUIRES hdw-bzr)
