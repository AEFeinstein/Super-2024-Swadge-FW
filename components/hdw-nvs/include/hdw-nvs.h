#ifndef _NVS_MANAGER_H_
#define _NVS_MANAGER_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "nvs.h"

//==============================================================================
// Defines
//==============================================================================

#define NVS_PART_NAME_MAX_SIZE 16 /*!< maximum length of partition name (excluding null terminator) */
#define NVS_KEY_NAME_MAX_SIZE  16 /*!< Maximal length of NVS key name (including null terminator) */

//==============================================================================
// Function Prototypes
//==============================================================================

bool initNvs(bool firstTry);
bool eraseNvs(void);
bool readNvs32(const char* key, int32_t* outVal);
bool writeNvs32(const char* key, int32_t val);
bool readNvsBlob(const char* key, void* out_value, size_t* length);
bool writeNvsBlob(const char* key, const void* value, size_t length);
bool eraseNvsKey(const char* key);
bool readNvsStats(nvs_stats_t* outStats);
bool readAllNvsEntryInfos(nvs_stats_t* outStats, nvs_entry_info_t** outEntryInfos, size_t* numEntryInfos);

#endif
