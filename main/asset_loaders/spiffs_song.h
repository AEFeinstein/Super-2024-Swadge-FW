/*! \file spiffs_song.h
 *
 * \section spiffs_song_design Design Philosophy
 *
 * These functions load and free song assets which are compressed and compiled into the SPIFFS filesystem into RAM. Once
 * decompressed loaded into RAM, songs may be played on the buzzer.
 *
 * For more information about using songs, see hdw-bzr.h.
 *
 * For information on asset processing, see <a
 * href="https://github.com/AEFeinstein/Swadge-IDF-5.0/tree/main/tools/spiffs_file_preprocessor">spiffs_file_preprocessor</a>.
 *
 * \section spiffs_song_usage Usage
 *
 * Load songs from SPIFFS to RAM using loadSong(). Songs may be loaded to normal RAM, which is smaller and faster, or
 * SPI RAM, which is larger and slower.
 *
 * Free when done using freeSong(). If a song is not freed, the memory will leak.
 *
 * \section spiffs_song_example Example
 *
 * \code{.c}
 * // Load a song
 * song_t ode_to_joy;
 * loadSong("ode.sng", &ode_to_joy, true);
 * // Play the song as background music
 * bzrPlayBgm(&ode_to_joy, BZR_STEREO);
 * // Free the song when done
 * bzrStop(true);
 * freeSong(&ode_to_joy);
 * \endcode
 */

#ifndef _SPIFFS_SONG_H_
#define _SPIFFS_SONG_H_

#include <stdint.h>
#include <stdbool.h>

#include "hdw-bzr.h"

bool loadSong(const char* name, song_t* song, bool spiRam);
void freeSong(song_t* song);

#endif