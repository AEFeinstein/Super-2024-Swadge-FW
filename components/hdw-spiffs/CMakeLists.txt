idf_component_register(SRCS "hdw-spiffs.c"
                            "heatshrink_decoder.c"
                            "spiffs_font.c"
                            "spiffs_json.c"
                            "spiffs_txt.c"
                            "spiffs_wsg.c"
                    PRIV_REQUIRES hdw-tft
                    REQUIRES "spiffs" "heap"
                    INCLUDE_DIRS "include")
