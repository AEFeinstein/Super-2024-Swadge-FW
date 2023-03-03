/*! \file hdw-spiffs.h
 *
 * \section spiffs_design Design Philosophy
 *
 * SPIFFS is a file system intended for SPI NOR flash devices on embedded targets. It supports wear levelling, file
 * system consistency checks, and more. The full API reference can be found here: <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.0.1/esp32s2/api-reference/storage/spiffs.html">SPIFFS
 * Filesystem</a>.
 *
 * Ths Swadge treats SPIFFS as a read-only file system.
 *
 * During the build, assets in the \c /assets/ folder, such as PNG images and MIDI songs, are processed into
 * Swadge-friendly formats and written to the \c /spiffs_image/ folder. The contents of the \c /spiffs_image/ are then
 * packaged into a SPIFFS image which is written to the \c storage partition in firmware. These files may be read and
 * used during runtime.
 *
 * \section spiffs_usage Usage
 *
 * You don't need to call initSpiffs() or deinitSpiffs(). The system does that the appropriate time.
 *
 * spiffsReadFile() may be used to read a file straight from SPIFFS, but this probably should not be done directly.
 *
 * Each asset type has it's own SPIFFS loader which handles things like decompresson if the asset type is compressed,
 * and writing values from the read file into a convenient struct. The loader functions are:
 *  - loadFont() & freeFont() - Load font assets from SPIFFS to draw text to the display
 *  - loadWsg() & freeWsg() - Load image asssets from SPIFFS to draw images to the display
 *  - loadSong() & freeSong() - Load song assets from SPIFFS to play songs on the buzzer
 *  - loadJson() & freeJson() - Load JSON assets from SPIFFS to configure games
 *  - loadTxt() & freeTxt() - Load text assets from SPIFFS to use in a Swadge mode
 *
 * Assets may be loaded to either SPI RAM or normal RAM.
 * There is more SPI RAM available, but it is slower to access than normal RAM.
 * Swadge modes should use normal RAM if they can, and use SPI RAM if the mode is asset-heavy.
 *
 * \section spiffs_example Example
 *
 * \code{.c}
 * // Declare and load a font
 * font_t ibm;
 * loadFont("ibm_vga8.font", &ibm, false);
 * // Draw some white text
 * drawText(&ibm, c555, "Hello World", 0, 0);
 * // Free the font
 * freeFont(&ibm);
 *
 * // Declare and load an image
 * wsg_t king_donut;
 * loadWsg("kid0.wsg", &king_donut, true);
 * // Draw the image to the display
 * drawWsg(&king_donut, 100, 100, false, false, 0);
 * // Free the image
 * freeWsg(&king_donut);
 *
 * // Declare and load a song
 * song_t ode_to_joy;
 * loadSong("ode.sng", &ode_to_joy, true);
 * // Play the song
 * bzrPlayBgm(&ode_to_joy);
 * // Free the song
 * freeSong(&ode_to_joy);
 * \endcode
 */

#ifndef _HDW_SPIFFS_H_
#define _HDW_SPIFFS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool initSpiffs(void);
bool deinitSpiffs(void);
uint8_t* spiffsReadFile(const char* fname, size_t* outsize, bool readToSpiRam);

#endif
