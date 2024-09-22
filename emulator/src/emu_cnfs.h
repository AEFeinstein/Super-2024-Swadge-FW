#pragma once

#include <stdbool.h>
#include <stddef.h>

bool emuCnfsInjectFile(const char* name, const char* filePath);
void emuCnfsInjectFileData(const char* name, size_t length, void* data);
