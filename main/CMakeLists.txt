idf_component_register(SRCS "swadge2024.c"
                            "display/fill.c"
                            "display/font.c"
                            "display/bresenham.c"
                            "display/cndraw.c"
                    PRIV_REQUIRES hdw-accel
                    PRIV_REQUIRES hdw-btn
                    PRIV_REQUIRES hdw-bzr
                    PRIV_REQUIRES hdw-led
                    PRIV_REQUIRES hdw-mic
                    PRIV_REQUIRES hdw-spiffs
                    PRIV_REQUIRES hdw-tft
                    PRIV_REQUIRES hdw-temperature
                    REQUIRES esp_timer spi_flash
                    INCLUDE_DIRS "."
                                 "./display")

function(spiffs_file_preprocessor)
    add_custom_target(spiffs_preprocessor ALL
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/../tools/spiffs_file_preprocessor/
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tools/spiffs_file_preprocessor/spiffs_file_preprocessor -i ${CMAKE_CURRENT_SOURCE_DIR}/../assets/ -o ${CMAKE_CURRENT_SOURCE_DIR}/../spiffs_image/
    )
endfunction()

# Create a SPIFFS image from the contents of the 'spiffs_image' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
spiffs_file_preprocessor()
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)

execute_process(
    COMMAND git rev-parse --short=7 HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE VERSION_SHA1 )

string(STRIP ${VERSION_SHA1} VERSION_SHA1)

add_definitions( -DGIT_SHA1="${VERSION_SHA1}" )
