idf_component_register(SRCS "hdw-imu.c" "quaternions.c"
                    INCLUDE_DIRS "include" "."
                    PRIV_REQUIRES main
                    REQUIRES driver)
