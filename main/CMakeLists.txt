idf_component_register(SRCS "asset_loaders/common/heatshrink_encoder.c"
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
                            "menu/menuManiaRenderer.c"
                            "menu/menu_utils.c"
                            "modes/games/pinball/mode_pinball.c"
                            "modes/games/pinball/pinball_draw.c"
                            "modes/games/pinball/pinball_physics.c"
                            "modes/games/pinball/pinball_test.c"
                            "modes/games/pinball/pinball_zones.c"
                            "modes/music/colorchord/colorchord.c"
                            "modes/music/jukebox/jukebox.c"
                            "modes/music/tunernome/tunernome.c"
                            "modes/music/usbsynth/mode_synth.c"
                            "modes/system/credits/credits_utils.c"
                            "modes/system/credits/mode_credits.c"
                            "modes/system/mainMenu/mainMenu.c"
                            "modes/system/quickSettings/menuQuickSettingsRenderer.c"
                            "modes/system/quickSettings/quickSettings.c"
                            "modes/test/accelTest/accelTest.c"
                            "modes/test/factoryTest/factoryTest.c"
                            "modes/test/touchTest/touchTest.c"
                            "modes/utilities/dance/dance.c"
                            "modes/utilities/dance/portableDance.c"
                            "modes/utilities/gamepad/gamepad.c"
                            "modes/utilities/timer/modeTimer.c"
                            "swadge2024.c"
                            "utils/color_utils.c"
                            "utils/dialogBox.c"
                            "utils/drums.c"
                            "utils/fp_math.c"
                            "utils/geometry.c"
                            "utils/linked_list.c"
                            "utils/midiPlayer.c"
                            "utils/p2pConnection.c"
                            "utils/settingsManager.c"
                            "utils/sngPlayer.c"
                            "utils/swSynth.c"
                            "utils/textEntry.c"
                            "utils/touchTextEntry.c"
                            "utils/touchUtils.c"
                            "utils/trigonometry.c"
                            "utils/vector2d.c"
                            "utils/waveTables.c"
                            "utils/wheel_menu.c"
                    PRIV_REQUIRES hdw-imu
                                  hdw-battmon
                                  hdw-btn
                                  hdw-bzr
                                  hdw-dac
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
                                "./asset_loaders/common"
                                "./colorchord"
                                "./display"
                                "./menu"
                                "./modes"
                                "./modes/games"
                                "./modes/games/pinball"
                                "./modes/music"
                                "./modes/music/colorchord"
                                "./modes/music/jukebox"
                                "./modes/music/usbsynth"
                                "./modes/music/tunernome"
                                "./modes/system"
                                "./modes/system/credits"
                                "./modes/system/mainMenu"
                                "./modes/system/quickSettings"
                                "./modes/test"
                                "./modes/test/accelTest"
                                "./modes/test/factoryTest"
                                "./modes/test/touchTest"
                                "./modes/utilities"
                                "./modes/utilities/dance"
                                "./modes/utilities/gamepad"
                                "./modes/utilities/timer"
                                "./utils")

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
