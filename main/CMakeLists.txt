idf_component_register(SRCS "asset_loaders/common/heatshrink_encoder.c"
                            "asset_loaders/fs_font.c"
                            "asset_loaders/fs_json.c"
                            "asset_loaders/fs_txt.c"
                            "asset_loaders/fs_wsg.c"
                            "asset_loaders/heatshrink_decoder.c"
                            "asset_loaders/heatshrink_helper.c"
                            "colorchord/DFT32.c"
                            "colorchord/embeddedNf.c"
                            "colorchord/embeddedOut.c"
                            "display/fill.c"
                            "display/font.c"
                            "display/shapes.c"
                            "display/wsg.c"
                            "display/wsgPalette.c"
                            "menu/menu.c"
                            "menu/menuManiaRenderer.c"
                            "menu/menu_utils.c"
                            "midi/bakedDrums.c"
                            "midi/drums.c"
                            "midi/midiData.c"
                            "midi/midiFileParser.c"
                            "midi/midiPlayer.c"
                            "midi/midiUsb.c"
                            "midi/midiUtil.c"
                            "midi/waveTables.c"
                            "modeIncludeList.c"
                            "modes/music/colorchord/colorchord.c"
                            "modes/music/jukebox/jukebox.c"
                            "modes/music/sequencer/sequencerGrid.c"
                            "modes/music/sequencer/sequencerHelp.c"
                            "modes/music/sequencer/sequencerMode.c"
                            "modes/music/tunernome/tunernome.c"
                            "modes/music/usbsynth/mode_synth.c"
                            "modes/system/credits/credits_utils.c"
                            "modes/system/credits/mode_credits.c"
                            "modes/system/intro/introMode.c"
                            "modes/system/intro/tutorial.c"
                            "modes/system/mainMenu/mainMenu.c"
                            "modes/system/quickSettings/menuQuickSettingsRenderer.c"
                            "modes/system/quickSettings/quickSettings.c"
                            "modes/test/accelTest/accelTest.c"
                            "modes/test/accelTest/bunny.c"
                            "modes/test/factoryTest/factoryTest.c"
                            "modes/test/keebTest/keebTest.c"
                            "modes/test/touchTest/touchTest.c"
                            "modes/utilities/dance/dance.c"
                            "modes/utilities/dance/portableDance.c"
                            "modes/utilities/gamepad/gamepad.c"
                            "swadge2024.c"
                            "utils/cnfs.c"
                            "utils/cnfs_image.c"
                            "utils/color_utils.c"
                            "utils/dialogBox.c"
                            "utils/fl_math/geometryFl.c"
                            "utils/fl_math/vectorFl2d.c"
                            "utils/fp_math.c"
                            "utils/geometry.c"
                            "utils/hashMap.c"
                            "utils/linked_list.c"
                            "utils/p2pConnection.c"
                            "utils/settingsManager.c"
                            "utils/swSynth.c"
                            "utils/textEntry.c"
                            "utils/touchTextEntry.c"
                            "utils/touchUtils.c"
                            "utils/trigonometry.c"
                            "utils/vector2d.c"
                            "utils/wheel_menu.c"
                    PRIV_REQUIRES hdw-imu
                                  hdw-battmon
                                  hdw-btn
                                  hdw-dac
                                  hdw-esp-now
                                  hdw-led
                                  hdw-mic
                                  hdw-nvs
                                  hdw-tft
                                  hdw-temperature
                                  hdw-usb
                                  crashwrap
                    REQUIRES esp_timer
                             spi_flash
                    INCLUDE_DIRS "./"
                                "./asset_loaders"
                                "./asset_loaders/common"
                                "./colorchord"
                                "./display"
                                "./menu"
                                "./midi"
                                "./modes"
                                "./modes/music"
                                "./modes/music/colorchord"
                                "./modes/music/jukebox"
                                "./modes/music/sequencer"
                                "./modes/music/tunernome"
                                "./modes/music/usbsynth"
                                "./modes/system"
                                "./modes/system/credits"
                                "./modes/system/intro"
                                "./modes/system/mainMenu"
                                "./modes/system/quickSettings"
                                "./modes/test"
                                "./modes/test/accelTest"
                                "./modes/test/factoryTest"
                                "./modes/test/keebTest"
                                "./modes/test/touchTest"
                                "./modes/utilities"
                                "./modes/utilities/dance"
                                "./modes/utilities/gamepad"
                                "./utils"
                                "./utils/fl_math")

# Custom target will always cause its dependencies to be evaluated and is
# run by default
add_custom_target(dummy_target ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/utils/cnfs_image.c
)

# custom_output will always be rebuilt because it depends on always_rebuild
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/utils/cnfs_image.c
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/utils/cnfs_image.h
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/../tools/assets_preprocessor/
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tools/assets_preprocessor/assets_preprocessor -i ${CMAKE_CURRENT_SOURCE_DIR}/../assets/ -o ${CMAKE_CURRENT_SOURCE_DIR}/../assets_image/
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/../tools/cnfs
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tools/cnfs/cnfs_gen ${CMAKE_CURRENT_SOURCE_DIR}/../assets_image/ ${CMAKE_CURRENT_SOURCE_DIR}/utils/cnfs_image.c ${CMAKE_CURRENT_SOURCE_DIR}/utils/cnfs_image.h
    DEPENDS always_rebuild
)

# Dummy output which is never actually produced. Anything that depends on
# this will always be rebuilt.
add_custom_command(
    OUTPUT always_rebuild
    COMMAND cmake -E echo
)

# Save the git hash in a variable
execute_process(
    COMMAND git rev-parse --short=7 HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE VERSION_SHA1 )
string(STRIP ${VERSION_SHA1} VERSION_SHA1)
add_definitions( -DGIT_SHA1="${VERSION_SHA1}" )
