idf_component_register(SRCS "modes/games/atrium/atrium.c"
                            "modes/games/RoboRunner/roboRunner.c"
                            "modes/games/campIdle/campIdle.c"
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
                            "modes/system/swsnCreator/swsnCreator.c"
                            "modes/system/trophyCase/trophyCase.c"
                            "modes/test/accelTest/accelTest.c"
                            "modes/test/accelTest/bunny.c"
                            "modes/test/canvasTest/canvas.c"
                            "modes/test/ch32v003test/mode_ch32v003test.c"
                            "modes/test/factoryTest/factoryTest.c"
                            "modes/test/keebTest/keebTest.c"
                            "modes/test/nameTest/nameTest.c"
                            "modes/test/sonaTest/sonaTest.c"
                            "modes/test/swadgePassTest/swadgePassTest.c"
                            "modes/test/touchTest/touchTest.c"
                            "modes/test/trophyTest/trophyTest.c"
                            "modes/utilities/dance/dance.c"
                            "modes/utilities/dance/portableDance.c"
                            "modes/utilities/gamepad/gamepad.c"
                            "swadge.c"
                            "utils/colorchord/DFT32.c"
                            "utils/colorchord/embeddedNf.c"
                            "utils/colorchord/embeddedOut.c"
                            "utils/data_structures/hashMap.c"
                            "utils/data_structures/linked_list.c"
                            "utils/draw/color_utils.c"
                            "utils/draw/fill.c"
                            "utils/draw/font.c"
                            "utils/draw/shapes.c"
                            "utils/draw/wsg.c"
                            "utils/draw/wsgCanvas.c"
                            "utils/draw/wsgPalette.c"
                            "utils/filesystem/cnfs.c"
                            "utils/filesystem/cnfs_image.c"
                            "utils/filesystem/fs_font.c"
                            "utils/filesystem/fs_json.c"
                            "utils/filesystem/fs_txt.c"
                            "utils/filesystem/fs_wsg.c"
                            "utils/filesystem/heatshrink/common/heatshrink_encoder.c"
                            "utils/filesystem/heatshrink/heatshrink_decoder.c"
                            "utils/filesystem/heatshrink/heatshrink_helper.c"
                            "utils/game/cutscene.c"
                            "utils/game/dialogBox.c"
                            "utils/game/helpPages.c"
                            "utils/game/highScores.c"
                            "utils/game/nameList.c"
                            "utils/game/swadgesona.c"
                            "utils/game/trophy.c"
                            "utils/input/textEntry.c"
                            "utils/input/touchTextEntry.c"
                            "utils/input/wheel_menu.c"
                            "utils/math/fl/geometryFl.c"
                            "utils/math/fl/vectorFl2d.c"
                            "utils/math/fp_math.c"
                            "utils/math/geometry.c"
                            "utils/math/quaternions.c"
                            "utils/math/trigonometry.c"
                            "utils/math/vector2d.c"
                            "utils/menu/menu.c"
                            "utils/menu/menuManiaRenderer.c"
                            "utils/menu/menuMegaRenderer.c"
                            "utils/menu/menuSimpleRenderer.c"
                            "utils/menu/menu_utils.c"
                            "utils/menu/modeIncludeList.c"
                            "utils/midi/bakedDrums.c"
                            "utils/midi/drums.c"
                            "utils/midi/midiData.c"
                            "utils/midi/midiFileParser.c"
                            "utils/midi/midiPlayer.c"
                            "utils/midi/midiUsb.c"
                            "utils/midi/midiUtil.c"
                            "utils/midi/swSynth.c"
                            "utils/midi/waveTables.c"
                            "utils/network/p2pConnection.c"
                            "utils/network/swadgePass.c"
                            "utils/peripherals/imu_utils.c"
                            "utils/peripherals/touchUtils.c"
                            "utils/settings/settingsManager.c"
                    PRIV_REQUIRES hdw-imu
                                  hdw-battmon
                                  hdw-btn
                                  hdw-ch32v003
                                  hdw-dac
                                  hdw-esp-now
                                  hdw-led
                                  hdw-mic
                                  hdw-nvs
                                  hdw-tft
                                  hdw-temperature
                                  hdw-touch
                                  hdw-usb
                                  crashwrap
                    REQUIRES esp_timer
                             spi_flash
                    INCLUDE_DIRS "./"
                                "./modes"
                                "./modes/games"
                                "./modes/games/atrium"
                                "./modes/games/campIdle"
                                "./modes/games/RoboRunner"
                                "./modes/music"
                                "./modes/music/colorchord"
                                "./modes/music/jukebox"
                                "./modes/music/tunernome"
                                "./modes/music/usbsynth"
                                "./modes/system"
                                "./modes/system/credits"
                                "./modes/system/intro"
                                "./modes/system/mainMenu"
                                "./modes/system/quickSettings"
                                "./modes/system/swsnCreator"
                                "./modes/system/trophyCase"
                                "./modes/test"
                                "./modes/test/accelTest"
                                "./modes/test/canvasTest"
                                "./modes/test/ch32v003test"
                                "./modes/test/factoryTest"
                                "./modes/test/keebTest"
                                "./modes/test/nameTest"
                                "./modes/test/sonaTest"
                                "./modes/test/swadgePassTest"
                                "./modes/test/touchTest"
                                "./modes/test/trophyTest"
                                "./modes/utilities"
                                "./modes/utilities/dance"
                                "./modes/utilities/gamepad"
                                "./utils"
                                "./utils/colorchord"
                                "./utils/data_structures"
                                "./utils/draw"
                                "./utils/filesystem"
                                "./utils/filesystem/heatshrink"
                                "./utils/filesystem/heatshrink/common"
                                "./utils/game"
                                "./utils/input"
                                "./utils/math"
                                "./utils/math/fl"
                                "./utils/menu"
                                "./utils/midi"
                                "./utils/network"
                                "./utils/peripherals"
                                "./utils/settings")

# Custom target will always cause its dependencies to be evaluated and is
# run by default
add_custom_target(dummy_target ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/utils/filesystem/cnfs_image.c
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../.assets_ts
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/../tools/assets_preprocessor/
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tools/assets_preprocessor/assets_preprocessor -c ${CMAKE_CURRENT_SOURCE_DIR}/../assets.conf -i ${CMAKE_CURRENT_SOURCE_DIR}/../assets/ -o ${CMAKE_CURRENT_SOURCE_DIR}/../assets_image/ -t ${CMAKE_CURRENT_SOURCE_DIR}/../.assets_ts
    DEPENDS always_rebuild
)

# custom_output will always be rebuilt because it depends on always_rebuild
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/utils/filesystem/cnfs_image.c
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/utils/filesystem/cnfs_image.h
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/../tools/cnfs
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tools/cnfs/cnfs_gen ${CMAKE_CURRENT_SOURCE_DIR}/../assets_image/ ${CMAKE_CURRENT_SOURCE_DIR}/utils/filesystem/cnfs_image.c ${CMAKE_CURRENT_SOURCE_DIR}/utils/filesystem/cnfs_image.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../.assets_ts
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
