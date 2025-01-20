#pragma once

#include <stddef.h>
#include <stdint.h>

char* blobToStr(const void* value, size_t length);
void strToBlob(char* str, void* outBlob, size_t blobLen);

void emuInjectNvsBlob(const char* namespace, const char* key, size_t length, const void* blob);
void emuInjectNvs32(const char* namespace, const char* key, int32_t value);
void emuInjectNvsDelete(const char* namespace, const char* key);
void emuInjectNvsClearAll(void);