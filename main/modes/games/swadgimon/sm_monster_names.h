#ifndef _SM_MONSTER_NAMES_H
#define _SM_MONSTER_NAMES_H

#include <stdbool.h>
#include <stdint.h>

#include "sm_names.h"

names_header_t* monsterNamesInit(void);
uint8_t monsterNamesNameLength(const names_header_t* monsterNamesWithHeader);
uint16_t monsterNamesCapacity(const names_header_t* monsterNamesWithHeader);
char* monsterNamesGet(names_header_t* monsterNamesWithHeader, uint16_t idx);
bool monsterNamesGetFirstUnused(names_header_t* monsterNamesWithHeader, char** ptrOut, uint16_t* idxOut);
bool monsterNamesGetLastUsed(names_header_t* monsterNamesWithHeader, char** ptrOut, uint16_t* idxOut);
bool monsterNamesPut(names_header_t* monsterNamesWithHeader, char** ptrOut, uint16_t* idxOut, const char* data);
bool monsterNamesStrncpy(names_header_t* monsterNamesWithHeader, char* dataOut, uint16_t idx);
bool monsterNamesSearch(names_header_t* monsterNamesWithHeader, char** ptrOut, uint16_t* idxOut, const char* data);
bool monsterNamesRemoveIdx(names_header_t* monsterNamesWithHeader, uint16_t idx);
bool monsterNamesRemove(names_header_t* monsterNamesWithHeader, const char* data);
bool monsterNamesCopy(names_header_t* dest, names_header_t* src);
bool monsterNamesGrow(names_header_t** monsterNamesWithHeader, uint16_t additionalNames);
uint16_t monsterNamesCountUsed(names_header_t* monsterNamesWithHeader);
uint8_t monsterNamesCountLongestName(names_header_t* monsterNamesWithHeader);
bool monsterNamesEmpty(names_header_t* monsterNamesWithHeader);
bool monsterNamesFull(names_header_t* monsterNamesWithHeader);

#endif
