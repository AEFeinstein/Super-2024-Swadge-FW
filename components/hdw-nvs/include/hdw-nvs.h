/*! \file hdw-nvs.h
 *
 * \section nvs_design Design Philosophy
 *
 * The hdw-nvs component is a convenience wrapper for the IDF's <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.1.1/esp32s2/api-reference/storage/nvs_flash.html">Non-volatile
 * Storage Library</a>. Non-volatile storage (NVS) library is designed to store key-value pairs in flash.
 *
 * The goal is to make reading and writing integer and blob values easy and simple for Swadge modes.
 * The component takes care of opening and closing handles and error checking.
 *
 * There are no checks for key collisions between Swadge modes, so it's wise to use a mode-specific prefix for that
 * mode's keys.
 *
 * \section nvs_usage Usage
 *
 * You don't need to call initNvs() or deinitNvs(). The system does this the appropriate time.
 *
 * readNvs32() and writeNvs32() can be used to read and write 32 bit integer values.
 *
 * readNvsBlob() and writeNvsBlob() can be used to read and write binary blobs.
 *
 * eraseNvsKey() can be used to erase a key, both 32 bit integer or blob.
 * eraseNvs() can be used to erase all keys. It's dangerous to use, so be careful.
 *
 * readNvsStats() and readAllNvsEntryInfos() can be used to read metadata about NVS.
 *
 * \section nvs_example Example
 *
 * \code{.c}
 * const char demoKey[] = "demo_high_score";
 * int32_t highScoreToWrite = 99999;
 * if(true == writeNvs32(demoKey, highScoreToWrite))
 * {
 *     int32_t highScoreToRead;
 *     if(true == readNvs32(demoKey, &highScoreToRead))
 *     {
 *         printf("High score in NVS is %ld\n", highScoreToRead);
 *     }
 * }
 * \endcode
 */

#ifndef _NVS_MANAGER_H_
#define _NVS_MANAGER_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "nvs.h"

//==============================================================================
// Defines
//==============================================================================

#define NVS_PART_NAME_MAX_SIZE 16        /*!< maximum length of partition name (excluding null terminator) */
#define NVS_KEY_NAME_MAX_SIZE  16        /*!< Maximal length of NVS key name (including null terminator) */
#define NVS_NAMESPACE_NAME     "storage" /*!< The namespace used for NVS */

//==============================================================================
// Function Prototypes
//==============================================================================

bool initNvs(bool firstTry);
bool deinitNvs(void);
bool eraseNvs(void);
bool readNvs32(const char* key, int32_t* outVal);
bool writeNvs32(const char* key, int32_t val);
bool readNvsBlob(const char* key, void* out_value, size_t* length);
bool writeNvsBlob(const char* key, const void* value, size_t length);
bool readNamespaceNvsBlob(const char* namespace, const char* key, void* out_value, size_t* length);
bool writeNamespaceNvsBlob(const char* namespace, const char* key, const void* value, size_t length);
bool eraseNvsKey(const char* key);
bool readNvsStats(nvs_stats_t* outStats);
bool readAllNvsEntryInfos(nvs_stats_t* outStats, nvs_entry_info_t** outEntryInfos, size_t* numEntryInfos);
bool readNamespaceNvsEntryInfos(const char* namespace, nvs_stats_t* outStats, nvs_entry_info_t** outEntryInfos, size_t* numEntryInfos);

#endif
