idf_component_register(SRCS "asset_loaders/common/heatshrink_encoder.c"
                            "asset_loaders/heatshrink_decoder.c"
                            "asset_loaders/heatshrink_helper.c"
                            "asset_loaders/fs_font.c"
                            "asset_loaders/fs_json.c"
                            "asset_loaders/fs_txt.c"
                            "asset_loaders/fs_wsg.c"
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
                            "midi/drums.c"
                            "midi/midiData.c"
                            "midi/midiFileParser.c"
                            "midi/midiPlayer.c"
                            "midi/midiUsb.c"
                            "midi/midiUtil.c"
                            "midi/waveTables.c"
                            "modes/games/bigbug/aabb_utils_bigbug.c"
                            "modes/games/bigbug/entity_bigbug.c"
                            "modes/games/bigbug/entityManager_bigbug.c"
                            "modes/games/bigbug/gameData_bigbug.c"
                            "modes/games/bigbug/mode_bigbug.c"
                            "modes/games/bigbug/soundManager_bigbug.c"
                            "modes/games/bigbug/tilemap_bigbug.c"
                            "modes/games/bigbug/pathfinding_bigbug.c"
                            "modes/games/pinball/mode_pinball.c"
                            "modes/games/pinball/pinball_draw.c"
                            "modes/games/pinball/pinball_physics.c"
                            "modes/games/pinball/pinball_test.c"
                            "modes/games/pinball/pinball_zones.c"
                            "modes/games/pango/pango.c"
                            "modes/games/pango/paEntity.c"
                            "modes/games/pango/paEntityManager.c"
                            "modes/games/pango/paGameData.c"
                            "modes/games/pango/paSoundManager.c"
                            "modes/games/pango/paTilemap.c"
                            "modes/games/ultimateTTT/ultimateTTT.c"
                            "modes/games/ultimateTTT/ultimateTTTgame.c"
                            "modes/games/ultimateTTT/ultimateTTThowTo.c"
                            "modes/games/ultimateTTT/ultimateTTTp2p.c"
                            "modes/games/ultimateTTT/ultimateTTTmarkerSelect.c"
                            "modes/games/ultimateTTT/ultimateTTTresult.c"
                            "modes/music/colorchord/colorchord.c"
                            "modes/music/jukebox/jukebox.c"
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
                            "modes/test/factoryTest/factoryTest.c"
                            "modes/test/touchTest/touchTest.c"
                            "modes/test/keebTest/keebTest.c"
                            "modes/utilities/dance/dance.c"
                            "modes/utilities/dance/portableDance.c"
                            "modes/utilities/gamepad/gamepad.c"
                            "modes/utilities/timer/modeTimer.c"
                            "swadge2024.c"
                            "utils/color_utils.c"
                            "utils/cnfs.c"
                            "utils/cnfs_image.c"
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
                    INCLUDE_DIRS "."
                                "./asset_loaders"
                                "./asset_loaders/common"
                                "./colorchord"
                                "./display"
                                "./menu"
                                "./midi"
                                "./modes"
                                "./modes/games"
                                "./modes/games/pinball"
                                "./modes/games/bigbug"
                                "./modes/games/ultimateTTT"
                                "./modes/music"
                                "./modes/music/colorchord"
                                "./modes/music/jukebox"
                                "./modes/music/usbsynth"
                                "./modes/music/tunernome"
                                "./modes/system"
                                "./modes/system/credits"
                                "./modes/system/intro"
                                "./modes/system/mainMenu"
                                "./modes/system/quickSettings"
                                "./modes/test"
                                "./modes/test/accelTest"
                                "./modes/test/factoryTest"
                                "./modes/test/touchTest"
                                "./modes/test/keebTest"
                                "./modes/utilities"
                                "./modes/utilities/dance"
                                "./modes/utilities/gamepad"
                                "./modes/utilities/timer"
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
