idf_component_register(SRCS "swadge2024.c"
                            "asset_loaders/heatshrink_decoder.c"
                            "asset_loaders/heatshrink_helper.c"
                            "asset_loaders/spiffs_font.c"
                            "asset_loaders/spiffs_json.c"
                            "asset_loaders/spiffs_song.c"
                            "asset_loaders/spiffs_txt.c"
                            "asset_loaders/spiffs_wsg.c"
                            "colorchord/DFT32.c"
                            "colorchord/embeddedNf.c"
                            "colorchord/embeddedOut.c"
                            "display/fill.c"
                            "display/font.c"
                            "display/shapes.c"
                            "display/wsg.c"
                            "menu/menu.c"
                            "menu/menuLogbookRenderer.c"
                            "modes/colorchord/mode_colorchord.c"
                            "modes/demo/demoMode.c"
                            "modes/mainMenu/mainMenu.c"
                            "modes/pong/pong.c"
                            "modes/lumberjack/lumberjack.c"
                            "modes/lumberjack/lumberjackEntity.c"
                            "modes/lumberjack/lumberjackPlayer.c"
                            "utils/color_utils.c"
                            "utils/geometry.c"
                            "utils/linked_list.c"
                            "utils/p2pConnection.c"
                            "utils/settingsManager.c"
                            "utils/trigonometry.c"
                            "utils/vector2d.c"
                    PRIV_REQUIRES hdw-accel
                                  hdw-battmon
                                  hdw-btn
                                  hdw-bzr
                                  hdw-esp-now
                                  hdw-led
                                  hdw-mic
                                  hdw-nvs
                                  hdw-spiffs
                                  hdw-tft
                                  hdw-temperature
                                  hdw-usb
                                  crashwrap
                    REQUIRES esp_timer
                             spi_flash
                    INCLUDE_DIRS "."
                                "./asset_loaders"
                                "./colorchord"
                                "./utils"
                                "./display"
                                "./menu"
                                "./modes/colorchord"
                                "./modes/pong"
                                "./modes/mainMenu"
                                "./modes/demo"
                                "./modes/lumberjack")

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

# Save the git hash in a variable
execute_process(
    COMMAND git rev-parse --short=7 HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE VERSION_SHA1 )
string(STRIP ${VERSION_SHA1} VERSION_SHA1)
add_definitions( -DGIT_SHA1="${VERSION_SHA1}" )
