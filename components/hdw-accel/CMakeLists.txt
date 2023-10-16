idf_component_register(SRCS "hdw-accel.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver
					PRIV_REQUIRES hdw-nvs)
