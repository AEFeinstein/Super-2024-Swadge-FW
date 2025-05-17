#ifndef _FILE_UTILS_H_
#define _FILE_UTILS_H_

#include <stdbool.h>

#include "assets_preprocessor.h"

#define HI_WORD(x) ((x >> 16) & 0xFFFF)
#define LO_WORD(x) ((x) & 0xFFFF)
#define HI_BYTE(x) ((x >> 8) & 0xFF)
#define LO_BYTE(x) ((x) & 0xFF)

long getFileSize(const char* fname);
bool doesFileExist(const char* fname);
const char* get_filename(const char* filename);
bool isSourceFileNewer(const char* sourceFile, const char* destFile);
bool deleteFile(const char* path);

bool getOptionsFromIniFile(processorOptions_t* options, const char* file);
void deleteOptions(processorOptions_t* options);
const char* getFullOptionKey(char* tmp, size_t n, const optPair_t* option);
const char* getStrOption(const processorOptions_t* options, const char* name);
int getIntOption(const processorOptions_t* options, const char* name, int defaultVal);
bool getBoolOption(const processorOptions_t* options, const char* name, bool defaultVal);
bool hasOption(const processorOptions_t* options, const char* name);

#endif