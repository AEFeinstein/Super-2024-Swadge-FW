/*! \file spiffs_txt.h
 *
 * \section spiffs_txt_design Design Philosophy
 *
 * These functions load and free plaintext assets which are compiled into the SPIFFS filesystem into RAM.
 * Once loaded into RAM, the text data is a string that may be used for whatever purpose.
 *
 * For information on asset processing, see <a
 * href="https://github.com/AEFeinstein/Swadge-IDF-5.0/tree/main/tools/spiffs_file_preprocessor">spiffs_file_preprocessor</a>.
 *
 * \section spiffs_txt_usage Usage
 *
 * Load text from SPIFFS to RAM using loadTxt(). Text may be loaded to normal RAM, which is smaller and
 * faster, or SPI RAM, which is larger and slower.
 *
 * Free when done using freeTxt(). If text is not freed, the memory will leak.
 *
 * \section spiffs_txt_example Example
 *
 * \code{.c}
 * char* txtStr = loadTxt("story.txt", true);
 * // Free the txt
 * freeTxt(&txtStr);
 * \endcode
 */

#ifndef _SPIFFS_TXT_H_
#define _SPIFFS_TXT_H_

char* loadTxt(const char* name, bool spiRam);
void freeTxt(char* txtStr);

#endif