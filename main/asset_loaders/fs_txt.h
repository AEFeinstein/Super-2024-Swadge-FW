/*! \file fs_txt.h
 *
 * \section fs_txt_design Design Philosophy
 *
 * These functions load and free plaintext assets which are compiled into the filesystem into RAM.
 * Once loaded into RAM, the text data is a string that may be used for whatever purpose.
 *
 * For information on asset processing, see <a
 * href="https://github.com/AEFeinstein/Super-2024-Swadge-FW/tree/main/tools/assets_preprocessor">assets_preprocessor</a>.
 *
 * \section fs_txt_usage Usage
 *
 * Load text from the filesystem to RAM using loadTxt(). Text may be loaded to normal RAM, which is smaller and
 * faster, or SPI RAM, which is larger and slower.
 *
 * Free when done using freeTxt(). If text is not freed, the memory will leak.
 *
 * \section fs_txt_example Example
 *
 * \code{.c}
 * char* txtStr = loadTxt("story.txt", true);
 * // Free the txt
 * freeTxt(&txtStr);
 * \endcode
 */

#ifndef _FS_TXT_H_
#define _FS_TXT_H_

char* loadTxt(const char* name, bool spiRam);
void freeTxt(char* txtStr);

#endif