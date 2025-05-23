#pragma once

#include <stddef.h>
#include <stdint.h>

bool emuNvsInjectBlobFile(const char* namespace, const char* key, const char* filename);
void emuInjectNvsBlob(const char* namespace, const char* key, size_t length, const void* blob);
void emuInjectNvs32(const char* namespace, const char* key, int32_t value);
