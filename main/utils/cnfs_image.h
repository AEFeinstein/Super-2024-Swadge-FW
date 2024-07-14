#pragma once

#include <stdint.h>

typedef struct
{
    const char* name;
    uint32_t len;
    uint32_t offset;
} cnfsFileEntry;

const uint8_t* getCnfsImage(void);
int32_t getCnfsSize(void);
const cnfsFileEntry* getCnfsFiles(void);
int32_t getCnfsNumFiles(void);
