idf_component_register(SRCS "hdw-imu.c" "quaternions.c"
                    INCLUDE_DIRS "include" "."
                    REQUIRES driver
					PRIV_REQUIRES hdw-nvs)
