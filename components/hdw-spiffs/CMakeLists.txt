idf_component_register(SRCS "hdw-spiffs.c"
                    PRIV_REQUIRES hdw-tft hdw-bzr
                    REQUIRES "spiffs" "heap"
                    INCLUDE_DIRS "include")
