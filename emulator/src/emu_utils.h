#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <hdw-btn.h>

#if defined(WINDOWS) || defined(__WINDOWS__) || defined(_WINDOWS) || defined(WIN32) || defined(WIN64)          \
    || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__MINGW32__) \
    || defined(__MINGW64__) || defined(__TOS_WIN__) || defined(_MSC_VER)
    #define EMU_WINDOWS 1
#elif defined(__APPLE__)
    #define EMU_MACOS 1
#elif defined(__linux) || defined(__linux__) || defined(linux) || defined(__LINUX__)
    #define EMU_LINUX 1
#endif

int makeDir(const char* path);
bool makeDirs(const char* path);
void expandPath(char* buffer, size_t length, const char* path);
const char* getTimestampFilename(char* dst, size_t n, const char* prefix, const char* ext);
buttonBit_t parseButtonName(const char* buttonName);
const char* getButtonName(buttonBit_t btn);
