/*! \file spiffs_txt.h
 *
 * \section spiffs_txt_design Design Philosophy
 *
 * TODO doxygen
 *
 * \section spiffs_txt_usage Usage
 *
 * TODO doxygen
 *
 * \section spiffs_txt_example Example
 *
 * \code{.c}
 * TODO doxygen
 * \endcode
 */

#ifndef _SPIFFS_TXT_H_
#define _SPIFFS_TXT_H_

char* loadTxt(const char* name, bool spiRam);
void freeTxt(char* txtStr);

#endif