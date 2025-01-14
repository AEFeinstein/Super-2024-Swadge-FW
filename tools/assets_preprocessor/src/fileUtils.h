#ifndef _FILE_UTILS_H_
#define _FILE_UTILS_H_

#include <stdbool.h>

#if defined(WINDOWS) || defined(__WINDOWS__) || defined(_WINDOWS) || defined(WIN32) || defined(WIN64)          \
    || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__MINGW32__) \
    || defined(__MINGW64__) || defined(__TOS_WIN__) || defined(_MSC_VER)
    #define SAPP_WINDOWS 1
#elif defined(__APPLE__)
    #define SAPP_MACOS 1
#elif defined(__linux) || defined(__linux__) || defined(linux) || defined(__LINUX__)
    #define SAPP_LINUX 1
#endif

#define HI_WORD(x) ((x >> 16) & 0xFFFF)
#define LO_WORD(x) ((x) & 0xFFFF)
#define HI_BYTE(x) ((x >> 8) & 0xFF)
#define LO_BYTE(x) ((x) & 0xFF)

long getFileSize(const char* fname);
bool doesFileExist(const char* fname);
const char* get_filename(const char* filename);
bool isSourceFileNewer(const char* sourceFile, const char* destFile);

#endif