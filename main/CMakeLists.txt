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
                            "display/wsgCanvas.c"
                            "display/wsgPalette.c"
                            "menu/menu.c"
                            "menu/menuManiaRenderer.c"
                            "menu/menuMegaRenderer.c"
                            "menu/menuSimpleRenderer.c"
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
                            "modes/games/artillery/artillery.c"
                            "modes/games/artillery/artillery_game.c"
                            "modes/games/artillery/artillery_game_over.c"
                            "modes/games/artillery/artillery_help.c"
                            "modes/games/artillery/artillery_p2p.c"
                            "modes/games/artillery/artillery_paint.c"
                            "modes/games/artillery/artillery_phys.c"
                            "modes/games/artillery/artillery_phys_bsp.c"
                            "modes/games/artillery/artillery_phys_camera.c"
                            "modes/games/artillery/artillery_phys_collisions.c"
                            "modes/games/artillery/artillery_phys_objs.c"
                            "modes/games/artillery/artillery_phys_terrain.c"
                            "modes/games/cosCrunch/ccmgBreakTime.c"
                            "modes/games/cosCrunch/ccmgDelivery.c"
                            "modes/games/cosCrunch/ccmgSew.c"
                            "modes/games/cosCrunch/ccmgSlice.c"
                            "modes/games/cosCrunch/ccmgSpray.c"
                            "modes/games/cosCrunch/ccmgThread.c"
                            "modes/games/cosCrunch/cosCrunch.c"
                            "modes/games/cosCrunch/cosCrunchUtil.c"
                            "modes/games/cosCrunch/menuCosCrunchRenderer.c"
                            "modes/games/danceNetwork/danceNetwork.c"
                            "modes/games/danceNetwork/dn_entity.c"
                            "modes/games/danceNetwork/dn_entityManager.c"
                            "modes/games/danceNetwork/dn_howTo.c"
                            "modes/games/danceNetwork/dn_p2p.c"
                            "modes/games/danceNetwork/dn_random.c"
                            "modes/games/danceNetwork/dn_result.c"
                            "modes/games/danceNetwork/dn_utility.c"
                            "modes/games/picross/mode_picross.c"
                            "modes/games/picross/picross_menu.c"
                            "modes/games/picross/picross_select.c"
                            "modes/games/picross/picross_tutorial.c"
                            "modes/games/platformer/aabb_utils.c"
                            "modes/games/platformer/megaPulseEx.c"
                            "modes/games/platformer/mgEntity.c"
                            "modes/games/platformer/mgEntityManager.c"
                            "modes/games/platformer/mgGameData.c"
                            "modes/games/platformer/mgSoundManager.c"
                            "modes/games/platformer/mgTilemap.c"
                            "modes/games/platformer/mgWsgManager.c"
                            "modes/games/RoboRunner/roboRunner.c"
                            "modes/games/swadgedoku/sudoku_data.c"
                            "modes/games/swadgedoku/sudoku_game.c"
                            "modes/games/swadgedoku/sudoku_ui.c"
                            "modes/games/swadgedoku/swadgedoku.c"
                            "modes/games/swadgeIt/swadgeIt.c"
                            "modes/music/colorchord/colorchord.c"
                            "modes/music/jukebox/jukebox.c"
                            "modes/music/sequencer/sequencerGrid.c"
                            "modes/music/sequencer/sequencerHelp.c"
                            "modes/music/sequencer/sequencerMode.c"
                            "modes/music/swadgetamatone/swadgetamatone.c"
                            "modes/music/tunernome/tunernome.c"
                            "modes/music/usbsynth/mode_synth.c"
                            "modes/system/credits/credits_utils.c"
                            "modes/system/credits/mode_credits.c"
                            "modes/system/intro/introMode.c"
                            "modes/system/intro/tutorial.c"
                            "modes/system/mainMenu/mainMenu.c"
                            "modes/system/quickSettings/menuQuickSettingsRenderer.c"
                            "modes/system/quickSettings/quickSettings.c"
                            "modes/system/trophyCase/trophyCase.c"
                            "modes/test/accelTest/accelTest.c"
                            "modes/test/accelTest/bunny.c"
                            "modes/test/canvasTest/canvas.c"
                            "modes/test/factoryTest/factoryTest.c"
                            "modes/test/keebTest/keebTest.c"
                            "modes/test/nameTest/nameTest.c"
                            "modes/test/swadgePassTest/swadgePassTest.c"
                            "modes/test/touchTest/touchTest.c"
                            "modes/test/trophyTest/trophyTest.c"
                            "modes/test/sonaTest/sonaTest.c"
                            "modes/utilities/bouncy/bouncy.c"
                            "modes/utilities/ch32v003test/mode_ch32v003test.c"
                            "modes/utilities/dance/dance.c"
                            "modes/utilities/dance/portableDance.c"
                            "modes/utilities/dice/mode_diceroller.c"
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
                            "utils/highScores.c"
                            "utils/imu_utils.c"
                            "utils/linked_list.c"
                            "utils/nameList.c"
                            "utils/p2pConnection.c"
                            "utils/settingsManager.c"
                            "utils/swadgePass.c"
                            "utils/swadgesona.c"
                            "utils/swSynth.c"
                            "utils/textEntry.c"
                            "utils/touchTextEntry.c"
                            "utils/touchUtils.c"
                            "utils/trigonometry.c"
                            "utils/trophy.c"
                            "utils/vector2d.c"
                            "utils/wheel_menu.c"
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
                                "./modes/games"
                                "./modes/games/artillery"
                                "./modes/games/cosCrunch"
                                "./modes/games/danceNetwork"
                                "./modes/games/picross"
                                "./modes/games/platformer"
                                "./modes/games/RoboRunner"
                                "./modes/games/swadgedoku"
                                "./modes/games/swadgeIt"
                                "./modes/music"
                                "./modes/music/colorchord"
                                "./modes/music/jukebox"
                                "./modes/music/sequencer"
                                "./modes/music/swadgetamatone"
                                "./modes/music/tunernome"
                                "./modes/music/usbsynth"
                                "./modes/system"
                                "./modes/system/credits"
                                "./modes/system/intro"
                                "./modes/system/mainMenu"
                                "./modes/system/quickSettings"
                                "./modes/system/trophyCase"
                                "./modes/test"
                                "./modes/test/accelTest"
                                "./modes/test/canvasTest"
                                "./modes/test/factoryTest"
                                "./modes/test/keebTest"
                                "./modes/test/nameTest"
                                "./modes/test/swadgePassTest"
                                "./modes/test/touchTest"
                                "./modes/test/trophyTest"
                                "./modes/test/sonaTest"
                                "./modes/utilities"
                                "./modes/utilities/bouncy"
                                "./modes/utilities/ch32v003test"
                                "./modes/utilities/dance"
                                "./modes/utilities/dice"
                                "./modes/utilities/gamepad"
                                "./utils"
                                "./utils/fl_math")

# Custom target will always cause its dependencies to be evaluated and is
# run by default
add_custom_target(dummy_target ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/utils/cnfs_image.c
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../.assets_ts
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/../tools/assets_preprocessor/
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tools/assets_preprocessor/assets_preprocessor -c ${CMAKE_CURRENT_SOURCE_DIR}/../assets.conf -i ${CMAKE_CURRENT_SOURCE_DIR}/../assets/ -o ${CMAKE_CURRENT_SOURCE_DIR}/../assets_image/ -t ${CMAKE_CURRENT_SOURCE_DIR}/../.assets_ts
    DEPENDS always_rebuild
)

# custom_output will always be rebuilt because it depends on always_rebuild
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/utils/cnfs_image.c
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/utils/cnfs_image.h
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/../tools/cnfs
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tools/cnfs/cnfs_gen ${CMAKE_CURRENT_SOURCE_DIR}/../assets_image/ ${CMAKE_CURRENT_SOURCE_DIR}/utils/cnfs_image.c ${CMAKE_CURRENT_SOURCE_DIR}/utils/cnfs_image.h
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
