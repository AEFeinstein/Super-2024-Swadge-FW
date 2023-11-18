idf_component_register(SRCS "swadge2024.c"
                            "asset_loaders/heatshrink_decoder.c"
                            "asset_loaders/heatshrink_helper.c"
                            "asset_loaders/spiffs_font.c"
                            "asset_loaders/spiffs_json.c"
                            "asset_loaders/spiffs_song.c"
                            "asset_loaders/spiffs_txt.c"
                            "asset_loaders/spiffs_wsg.c"
                            "asset_loaders/common/heatshrink_encoder.c"
                            "colorchord/DFT32.c"
                            "colorchord/embeddedNf.c"
                            "colorchord/embeddedOut.c"
                            "display/fill.c"
                            "display/font.c"
                            "display/shapes.c"
                            "display/wsg.c"
                            "menu/menu.c"
                            "menu/menu_utils.c"
                            "menu/menuLogbookRenderer.c"
                            "modes/accelTest/accelTest.c"
                            "modes/breakout/aabb_utils.c"
                            "modes/breakout/breakout.c"
                            "modes/breakout/entity.c"
                            "modes/breakout/entityManager.c"
                            "modes/breakout/gameData.c"
                            "modes/breakout/soundManager.c"
                            "modes/breakout/starfield.c"
                            "modes/breakout/tilemap.c"
                            "modes/colorchord/colorchord.c"
                            "modes/credits/credits_utils.c"
                            "modes/credits/mode_credits.c"
                            "modes/dance/dance.c"
                            "modes/dance/portableDance.c"
                            "modes/demo/demoMode.c"
                            "modes/factoryTest/factoryTest.c"
                            "modes/gamepad/gamepad.c"
                            "modes/jukebox/jukebox.c"
                            "modes/lumberjack/lumberjack.c"
                            "modes/lumberjack/lumberjackGame.c"
                            "modes/lumberjack/lumberjackEntity.c"
                            "modes/lumberjack/lumberjackPlayer.c"
                            "modes/mainMenu/mainMenu.c"
                            "modes/mfpaint/mode_paint.c"
                            "modes/mfpaint/paint_browser.c"
                            "modes/mfpaint/paint_brush.c"
                            "modes/mfpaint/paint_canvas.c"
                            "modes/mfpaint/paint_common.c"
                            "modes/mfpaint/paint_draw.c"
                            "modes/mfpaint/paint_gallery.c"
                            "modes/mfpaint/paint_help.c"
                            "modes/mfpaint/paint_nvs.c"
                            "modes/mfpaint/paint_share.c"
                            "modes/mfpaint/paint_song.c"
                            "modes/mfpaint/paint_ui.c"
                            "modes/mfpaint/paint_util.c"
                            "modes/mfpaint/px_stack.c"
                            "modes/platformer/plEntity.c"
                            "modes/platformer/plEntityManager.c"
                            "modes/platformer/plGameData.c"
                            "modes/platformer/mode_platformer.c"
                            "modes/platformer/plTilemap.c"
                            "modes/platformer/plSoundManager.c"
                            "modes/pushy/pushy.c"
                            "modes/quickSettings/menuQuickSettingsRenderer.c"
                            "modes/quickSettings/quickSettings.c"
                            "modes/ray/fp_math.c"
                            "modes/ray/mode_ray.c"
                            "modes/ray/ray_credits.c"
                            "modes/ray/ray_dialog.c"
                            "modes/ray/ray_map.c"
                            "modes/ray/ray_object.c"
                            "modes/ray/ray_pause.c"
                            "modes/ray/ray_renderer.c"
                            "modes/ray/ray_script.c"
                            "modes/ray/ray_tex_manager.c"
                            "modes/ray/ray_player.c"
                            "modes/ray/ray_warp_screen.c"
                            "modes/ray/ray_death_screen.c"
                            "modes/ray/enemies/ray_enemy.c"
                            "modes/ray/enemies/ray_enemy.h"
                            "modes/ray/enemies/ray_enemy_armored.c"
                            "modes/ray/enemies/ray_enemy_armored.h"
                            "modes/ray/enemies/ray_enemy_boss.c"
                            "modes/ray/enemies/ray_enemy_boss.h"
                            "modes/ray/enemies/ray_enemy_flaming.c"
                            "modes/ray/enemies/ray_enemy_flaming.h"
                            "modes/ray/enemies/ray_enemy_hidden.c"
                            "modes/ray/enemies/ray_enemy_hidden.h"
                            "modes/ray/enemies/ray_enemy_normal.c"
                            "modes/ray/enemies/ray_enemy_normal.h"
                            "modes/ray/enemies/ray_enemy_strong.c"
                            "modes/ray/enemies/ray_enemy_strong.h"
                            "modes/soko/soko_game.c"
                            "modes/soko/soko_gamerules.c"
                            "modes/soko/soko_input.c"
                            "modes/soko/soko.c"
                            "modes/touchTest/touchTest.c"
                            "modes/tunernome/tunernome.c"
                            "utils/color_utils.c"
                            "utils/dialogBox.c"
                            "utils/geometry.c"
                            "utils/linked_list.c"
                            "utils/p2pConnection.c"
                            "utils/settingsManager.c"
                            "utils/textEntry.c"
                            "utils/touchUtils.c"
                            "utils/trigonometry.c"
                            "utils/vector2d.c"
                            "utils/wheel_menu.c"
                    PRIV_REQUIRES hdw-imu
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
                                "./asset_loaders/common"
                                "./colorchord"
                                "./utils"
                                "./display"
                                "./menu"
                                "./modes/accelTest"
                                "./modes/breakout"
                                "./modes/colorchord"
                                "./modes/credits"
                                "./modes/dance"
                                "./modes/demo"
                                "./modes/factoryTest"
                                "./modes/gamepad"
                                "./modes/jukebox"
                                "./modes/lumberjack"
                                "./modes/mainMenu"
                                "./modes/mfpaint"
                                "./modes/platformer"
                                "./modes/pushy"
                                "./modes/quickSettings"
                                "./modes/ray"
                                "./modes/ray/enemies"
                                "./modes/soko"
                                "./modes/touchTest"
                                "./modes/tunernome"
                                "./modes/demo"
                                "./modes/quickSettings")

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
