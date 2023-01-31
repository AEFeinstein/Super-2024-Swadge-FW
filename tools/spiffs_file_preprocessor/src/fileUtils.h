#ifndef _FILE_UTILS_H_
#define _FILE_UTILS_H_

#include <stdbool.h>

#define HI_WORD(x) ((x >> 16) & 0xFFFF)
#define LO_WORD(x) ((x)&0xFFFF)
#define HI_BYTE(x) ((x >> 8) & 0xFF)
#define LO_BYTE(x) ((x)&0xFF)

long getFileSize(const char* fname);
bool doesFileExist(const char* fname);
const char* get_filename(const char* filename);

#endif