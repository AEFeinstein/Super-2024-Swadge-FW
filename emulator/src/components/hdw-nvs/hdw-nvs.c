//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <math.h>

#include "hdw-nvs.h"
#include "cJSON.h"
#include "emu_main.h"

//==============================================================================
// Defines
//==============================================================================

#define NVS_JSON_FILE "nvs.json"

// This comes from partitions.csv, and must be changed in both places simultaneously
#define NVS_PARTITION_SIZE   0x6000
#define NVS_ENTRY_BYTES      32
#define NVS_OVERHEAD_ENTRIES 12

//==============================================================================
// Function Prototypes
//==============================================================================

static char* blobToStr(const void* value, size_t length);
static int hexCharToInt(char c);
static void strToBlob(char* str, void* outBlob, size_t blobLen);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the nonvolatile storage
 *
 * @param firstTry true if this is the first time NVS is initialized this boot,
 *                 false otherwise
 * @return true if NVS was initialized and can be used, false if it failed
 */
bool initNvs(bool firstTry)
{
    // Check if the json file exists
    if (access(NVS_JSON_FILE, F_OK) != 0)
    {
        FILE* nvsFile = fopen(NVS_JSON_FILE, "wb");
        if (NULL != nvsFile)
        {
            if (1 == fwrite("{}", sizeof("{}"), 1, nvsFile))
            {
                // Wrote successfully
                fclose(nvsFile);
                return true;
            }
            else
            {
                // Failed to write
                fclose(nvsFile);
                return false;
            }
        }
        else
        {
            // Couldn't open file
            return false;
        }
    }
    else
    {
        // File exists
        return true;
    }
}

/**
 * @brief Deinitialize NVS
 *
 * @return true
 */
bool deinitNvs(void)
{
    return true; // Nothing to do
}

/**
 * @brief Erase and re-initialize the nonvolatile storage
 *
 * @return true if NVS was erased and re-initialized and can be used, false if it failed
 */
bool eraseNvs(void)
{
    // Check if the json file exists
    if (access(NVS_JSON_FILE, F_OK) != 0)
    {
        // File does not exist, ready to initialize
        return initNvs(true);
    }
    else
    {
        if (remove(NVS_JSON_FILE) == 0)
        {
            // File deleted, ready to re-initialize
            return initNvs(true);
        }
        else
        {
            // Couldn't delete file
            return false;
        }
    }
}

/**
 * @brief Read a 32 bit value from NVS with a given string key
 *
 * @param key The key for the value to read
 * @param outVal The value that was read
 * @return true if the value was read, false if it was not
 */
bool readNvs32(const char* key, int32_t* outVal)
{
    return readNamespaceNvs32(NVS_NAMESPACE_NAME, key, outVal);
}

/**
 * @brief Write a 32 bit value to NVS with a given string key
 *
 * @param key The key for the value to write
 * @param val The value to write
 * @return true if the value was written, false if it was not
 */
bool writeNvs32(const char* key, int32_t val)
{
    return writeNamespaceNvs32(NVS_NAMESPACE_NAME, key, val);
}

/**
 * @brief Read a 32 bit value from NVS with a given string key
 *
 * @param namespace The NVS namespace to use
 * @param key The key for the value to read
 * @param outVal The value that was read
 * @return true if the value was read, false if it was not
 */
bool readNamespaceNvs32(const char* namespace, const char* key, int32_t* outVal)
{
    // Open the file
    FILE* nvsFile = fopen(NVS_JSON_FILE, "rb");
    if (NULL != nvsFile)
    {
        // Get the file size
        fseek(nvsFile, 0L, SEEK_END);
        size_t fsize = ftell(nvsFile);
        fseek(nvsFile, 0L, SEEK_SET);

        // Read the file
        char fbuf[fsize + 1];
        fbuf[fsize] = 0;
        if (fsize == fread(fbuf, 1, fsize, nvsFile))
        {
            // Close the file
            fclose(nvsFile);

            // Parse the JSON
            cJSON* json = cJSON_Parse(fbuf);
            cJSON* jsonIter;

            cJSON* jsonNs = cJSON_GetObjectItemCaseSensitive(json, namespace);

            if (cJSON_IsObject(jsonNs))
            {
                // Find the requested key
                char* current_key = NULL;
                cJSON_ArrayForEach(jsonIter, jsonNs)
                {
                    current_key = jsonIter->string;
                    if (current_key != NULL)
                    {
                        // If the key matches
                        if (0 == strcmp(current_key, key))
                        {
                            // Return the value
                            *outVal = (int32_t)cJSON_GetNumberValue(jsonIter);
                            cJSON_Delete(json);
                            return true;
                        }
                    }
                }
            }
            cJSON_Delete(json);
        }
        else
        {
            fclose(nvsFile);
        }
    }
    return false;
}

/**
 * @brief Write a 32 bit value to NVS with a given string key
 *
 * @param namespace The NVS namespace to use
 * @param key The key for the value to write
 * @param val The value to write
 * @return true if the value was written, false if it was not
 */
bool writeNamespaceNvs32(const char* namespace, const char* key, int32_t val)
{
    // Open the file
    FILE* nvsFile = fopen(NVS_JSON_FILE, "rb");
    if (NULL != nvsFile)
    {
        // Get the file size
        fseek(nvsFile, 0L, SEEK_END);
        size_t fsize = ftell(nvsFile);
        fseek(nvsFile, 0L, SEEK_SET);

        // Read the file
        char fbuf[fsize + 1];
        fbuf[fsize] = 0;
        if (fsize == fread(fbuf, 1, fsize, nvsFile))
        {
            // Close the file
            fclose(nvsFile);

            // Parse the JSON
            cJSON* json = cJSON_Parse(fbuf);

            cJSON* jsonNs = cJSON_GetObjectItemCaseSensitive(json, namespace);

            if (NULL == jsonNs)
            {
                jsonNs = cJSON_CreateObject();
                cJSON_AddItemToObject(json, namespace, jsonNs);
            }

            // Check if the key alredy exists
            cJSON* jsonIter;
            bool keyExists = false;
            cJSON_ArrayForEach(jsonIter, jsonNs)
            {
                if (0 == strcmp(jsonIter->string, key))
                {
                    keyExists = true;
                }
            }

            // Add or replace the item
            cJSON* jsonVal = cJSON_CreateNumber(val);
            if (keyExists)
            {
                cJSON_ReplaceItemInObject(jsonNs, key, jsonVal);
            }
            else
            {
                cJSON_AddItemToObject(jsonNs, key, jsonVal);
            }

            // Write the new JSON back to the file
            FILE* nvsFileW = fopen(NVS_JSON_FILE, "wb");
            if (NULL != nvsFileW)
            {
                char* jsonStr = cJSON_Print(json);
                fprintf(nvsFileW, "%s", jsonStr);
                fclose(nvsFileW);

                free(jsonStr);
                cJSON_Delete(json);

                return true;
            }
            else
            {
                // Couldn't open file to write
            }
            cJSON_Delete(json);
        }
        else
        {
            // Couldn't read file
            fclose(nvsFile);
        }
    }
    else
    {
        // couldn't open file to read
    }
    return false;
}

/**
 * @brief Read a blob from NVS with a given string key. Typically, this should be called once with NULL passed for
 * out_value, to get the value for length, then memory for out_value should be allocated, then this should be called
 * again.
 *
 * @param namespace The NVS namespace to use
 * @param key The key for the value to read
 * @param out_value The value will be written to this memory. It must be allocated before calling readNvsBlob()
 * @param length If out_value is `NULL`, this will be set to the length of the given key. Otherwise, it is the length of
 * the blob to read.
 * @return true if the value was read, false if it was not
 */
bool readNamespaceNvsBlob(const char* namespace, const char* key, void* out_value, size_t* length)
{
    // Open the file
    FILE* nvsFile = fopen(NVS_JSON_FILE, "rb");
    if (NULL != nvsFile)
    {
        // Get the file size
        fseek(nvsFile, 0L, SEEK_END);
        size_t fsize = ftell(nvsFile);
        fseek(nvsFile, 0L, SEEK_SET);

        // Read the file
        char fbuf[fsize + 1];
        fbuf[fsize] = 0;
        if (fsize == fread(fbuf, 1, fsize, nvsFile))
        {
            // Close the file
            fclose(nvsFile);

            // Parse the JSON
            cJSON* json = cJSON_Parse(fbuf);
            cJSON* jsonIter;

            cJSON* jsonNs = cJSON_GetObjectItemCaseSensitive(json, namespace);

            if (NULL != jsonNs && cJSON_IsObject(jsonNs))
            {
                // Find the requested key
                char* current_key = NULL;
                cJSON_ArrayForEach(jsonIter, jsonNs)
                {
                    current_key = jsonIter->string;
                    if (current_key != NULL)
                    {
                        // If the key matches
                        if (0 == strcmp(current_key, key))
                        {
                            // Return the value
                            char* strBlob = cJSON_GetStringValue(jsonIter);

                            if (out_value != NULL)
                            {
                                // The call to read, using returned length
                                strToBlob(strBlob, out_value, *length);
                            }
                            else
                            {
                                // The call to get length of blob
                                *length = strlen(strBlob) / 2;
                            }
                            cJSON_Delete(json);
                            return true;
                        }
                    }
                }
            }
            cJSON_Delete(json);
        }
        else
        {
            fclose(nvsFile);
        }
    }
    return false;
}

/**
 * @brief Write a blob to NVS with a given string key
 *
 * @param namespace The NVS namespace to use
 * @param key The key for the value to write
 * @param value The blob value to write
 * @param length The length of the blob
 * @return true if the value was written, false if it was not
 */
bool writeNamespaceNvsBlob(const char* namespace, const char* key, const void* value, size_t length)
{
    // Open the file
    FILE* nvsFile = fopen(NVS_JSON_FILE, "rb");
    if (NULL != nvsFile)
    {
        // Get the file size
        fseek(nvsFile, 0L, SEEK_END);
        size_t fsize = ftell(nvsFile);
        fseek(nvsFile, 0L, SEEK_SET);

        // Read the file
        char fbuf[fsize + 1];
        fbuf[fsize] = 0;
        if (fsize == fread(fbuf, 1, fsize, nvsFile))
        {
            // Close the file
            fclose(nvsFile);

            // Parse the JSON
            cJSON* json = cJSON_Parse(fbuf);

            cJSON* jsonNs = cJSON_GetObjectItemCaseSensitive(json, namespace);

            if (NULL == jsonNs)
            {
                jsonNs = cJSON_CreateObject();
                cJSON_AddItemToObject(json, namespace, jsonNs);
            }

            // Check if the key alredy exists
            cJSON* jsonIter;
            bool keyExists = false;
            cJSON_ArrayForEach(jsonIter, jsonNs)
            {
                if (0 == strcmp(jsonIter->string, key))
                {
                    keyExists = true;
                }
            }

            // Add or replace the item
            char* blobStr  = blobToStr(value, length);
            cJSON* jsonVal = cJSON_CreateString(blobStr);
            free(blobStr);
            if (keyExists)
            {
                cJSON_ReplaceItemInObject(jsonNs, key, jsonVal);
            }
            else
            {
                cJSON_AddItemToObject(jsonNs, key, jsonVal);
            }

            // Write the new JSON back to the file
            FILE* nvsFileW = fopen(NVS_JSON_FILE, "wb");
            if (NULL != nvsFileW)
            {
                char* jsonStr = cJSON_Print(json);
                fprintf(nvsFileW, "%s", jsonStr);
                fclose(nvsFileW);

                free(jsonStr);
                cJSON_Delete(json);

                return true;
            }
            else
            {
                // Couldn't open file to write
            }
            cJSON_Delete(json);
        }
        else
        {
            // Couldn't read file
            fclose(nvsFile);
        }
    }
    else
    {
        // couldn't open file to read
    }
    return false;
}

/**
 * @brief Read a blob from NVS with a given string key. Typically, this should be called once with NULL passed for
 * out_value, to get the value for length, then memory for out_value should be allocated, then this should be called
 * again.
 *
 * @param key The key for the value to read
 * @param out_value The value will be written to this memory. It must be allocated before calling readNvsBlob()
 * @param length If out_value is `NULL`, this will be set to the length of the given key. Otherwise, it is the length of
 * the blob to read.
 * @return true if the value was read, false if it was not
 */
bool readNvsBlob(const char* key, void* out_value, size_t* length)
{
    return readNamespaceNvsBlob(NVS_NAMESPACE_NAME, key, out_value, length);
}

/**
 * @brief Write a blob to NVS with a given string key
 *
 * @param key The key for the value to write
 * @param value The blob value to write
 * @param length The length of the blob
 * @return true if the value was written, false if it was not
 */
bool writeNvsBlob(const char* key, const void* value, size_t length)
{
    return writeNamespaceNvsBlob(NVS_NAMESPACE_NAME, key, value, length);
}

/**
 * @brief Delete the value with the given key from NVS
 *
 * @param key The NVS key to be deleted
 * @return true if the value was deleted, false if it was not
 */
bool eraseNvsKey(const char* key)
{
    return eraseNamespaceNvsKey(NVS_NAMESPACE_NAME, key);
}

/**
 * @brief Delete the value with the given key from NVS
 *
 * @param namespace The NVS namespace to use
 * @param key The NVS key to be deleted
 * @return true if the value was deleted, false if it was not
 */
bool eraseNamespaceNvsKey(const char* namespace, const char* key)
{
    // Open the file
    FILE* nvsFile = fopen(NVS_JSON_FILE, "rb");
    if (NULL != nvsFile)
    {
        // Get the file size
        fseek(nvsFile, 0L, SEEK_END);
        size_t fsize = ftell(nvsFile);
        fseek(nvsFile, 0L, SEEK_SET);

        // Read the file
        char fbuf[fsize + 1];
        fbuf[fsize] = 0;
        if (fsize == fread(fbuf, 1, fsize, nvsFile))
        {
            // Close the file
            fclose(nvsFile);

            // Parse the JSON
            cJSON* json = cJSON_Parse(fbuf);

            // Check if the key exists
            cJSON* jsonIter;
            bool keyExists = false;

            cJSON* jsonNs = cJSON_GetObjectItemCaseSensitive(json, namespace);

            if (NULL != jsonNs)
            {
                cJSON_ArrayForEach(jsonIter, jsonNs)
                {
                    if (0 == strcmp(jsonIter->string, key))
                    {
                        keyExists = true;
                    }
                }
            }

            // Remove the key if it exists
            if (keyExists)
            {
                cJSON_DeleteItemFromObject(jsonNs, key);
            }

            // Write the new JSON back to the file
            FILE* nvsFileW = fopen(NVS_JSON_FILE, "wb");
            if (NULL != nvsFileW)
            {
                char* jsonStr = cJSON_Print(json);
                fprintf(nvsFileW, "%s", jsonStr);
                fclose(nvsFileW);

                free(jsonStr);
                cJSON_Delete(json);

                return keyExists;
            }
            else
            {
                // Couldn't open file to write
            }
            cJSON_Delete(json);
        }
        else
        {
            // Couldn't read file
            fclose(nvsFile);
        }
    }
    else
    {
        // couldn't open file to read
    }
    return false;
}

/**
 * @brief Read info about used memory in NVS
 *
 * @param outStats The NVS stats struct will be written to this memory. It must be allocated before calling
 * readNvsStats()
 * @return true if the stats were read, false if it was not
 */
bool readNvsStats(nvs_stats_t* outStats)
{
    // Open the file
    FILE* nvsFile = fopen(NVS_JSON_FILE, "rb");
    if (NULL != nvsFile)
    {
        // Get the file size
        fseek(nvsFile, 0L, SEEK_END);
        size_t fsize = ftell(nvsFile);
        fseek(nvsFile, 0L, SEEK_SET);

        // Read the file
        char fbuf[fsize + 1];
        fbuf[fsize] = 0;
        if (fsize == fread(fbuf, 1, fsize, nvsFile))
        {
            // Close the file
            fclose(nvsFile);

            // Parse the JSON
            cJSON* json = cJSON_Parse(fbuf);
            cJSON* jsonIter;
            cJSON* namespace;

            cJSON_ArrayForEach(namespace, json)
            {
                // 1 entry is always used by each namespace, and there should only ever be 1 namespace
                outStats->used_entries++;
                // TODO: I just checked a Swadge and it said it was using 5 namespaces. Why?
                outStats->namespace_count++;
                /**
                 * When running readNvsStats() on an actual Swadge, the total NVS
                 * size is displayed as 12 entries less than the partition size.
                 *
                 * It's unknown if this is a percentage of total size,
                 * or a fixed number of overhead/control entries.
                 * I'm assuming it's a fixed number here.
                 */
                outStats->total_entries = NVS_PARTITION_SIZE / NVS_ENTRY_BYTES - NVS_OVERHEAD_ENTRIES;

                cJSON_ArrayForEach(jsonIter, namespace)
                {
                    if (jsonIter->string != NULL)
                    {
                        switch (jsonIter->type)
                        {
                            case cJSON_Number:
                            {
                                outStats->used_entries += 1;
                                break;
                            }
                            case cJSON_String:
                            {
                                char* strBlob = cJSON_GetStringValue(jsonIter);

                                /**
                                 * Get length of blob
                                 *
                                 * When the ESP32 is storing blobs, it uses 1 entry to index chunks,
                                 * 1 entry per chunk, then 1 entry for every 32 bytes of data, rounding up.
                                 *
                                 * I don't know how to find out how many chunks the ESP32 would split
                                 * certain length blobs into, so for now I'm assuming 1 chunk per blob.
                                 *
                                 * Blobs in the JSON are encoded as hexadecimal, so every 2 characters are
                                 * 1 byte of data. Then, every 32 bytes of data is an entry.
                                 */
                                outStats->used_entries += 2 + ceil(strlen(strBlob) / 2.0f / NVS_ENTRY_BYTES);
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                    }
                }
            }

            outStats->free_entries = outStats->total_entries - outStats->used_entries;
            cJSON_Delete(json);
            return true;
        }
        else
        {
            fclose(nvsFile);
        }
    }
    return false;
}

/**
 * @brief Read info about each used entry in a specific NVS namespace. Typically, this should be called once with NULL
 * passed for outEntryInfos, to get the value for numEntryInfos, then memory for outEntryInfos should be allocated, then
 * this should be called again
 *
 * @param namespace The name of the NVS namespace to use
 * @param outStats If not `NULL`, the NVS stats struct will be written to this memory. It must be allocated before
 * calling readAllNvsEntryInfos()
 * @param outEntryInfos A pointer to an array of NVS entry info structs will be written to this memory
 * @param numEntryInfos If outEntryInfos is `NULL`, this will be set to the length of the given key. Otherwise, it is
 * the number of entry infos to read
 * @return true if the entry infos were read, false if they were not
 */
bool readNamespaceNvsEntryInfos(const char* namespace, nvs_stats_t* outStats, nvs_entry_info_t** outEntryInfos,
                                size_t* numEntryInfos)
{
    // Open the file
    FILE* nvsFile = fopen(NVS_JSON_FILE, "rb");
    if (NULL != nvsFile)
    {
        // Get the file size
        fseek(nvsFile, 0L, SEEK_END);
        size_t fsize = ftell(nvsFile);
        fseek(nvsFile, 0L, SEEK_SET);

        // Read the file
        char fbuf[fsize + 1];
        fbuf[fsize] = 0;
        if (fsize == fread(fbuf, 1, fsize, nvsFile))
        {
            // Close the file
            fclose(nvsFile);

            // Parse the JSON
            cJSON* json = cJSON_Parse(fbuf);
            cJSON* jsonIter;

            // If the user doesn't want to receive the stats, only use them internally
            bool freeOutStats = false;
            if (outStats == NULL)
            {
                outStats     = calloc(1, sizeof(nvs_stats_t));
                freeOutStats = true;
            }

            if (!readNvsStats(outStats))
            {
                if (freeOutStats)
                {
                    free(outStats);
                }
                return false;
            }

            cJSON* jsonNs = cJSON_GetObjectItemCaseSensitive(json, namespace);

            if (NULL != jsonNs && cJSON_IsObject(jsonNs))
            {
                int i = 0;
                char* current_key;
                cJSON_ArrayForEach(jsonIter, jsonNs)
                {
                    if (outEntryInfos != NULL && i >= *numEntryInfos)
                    {
                        break;
                    }

                    current_key = jsonIter->string;
                    if (current_key != NULL)
                    {
                        if (outEntryInfos != NULL)
                        {
                            switch (jsonIter->type)
                            {
                                case cJSON_Number:
                                {
#ifdef USING_U32
                                    // cJSON cannot store any integer larger than 2^53 or smaller than -(2^53), since
                                    // those are the limits of a double
                                    int64_t val = (int64_t)cJSON_GetNumberValue(jsonIter);
                                    if (val > INT32_MAX)
                                    {
                                        (&((*outEntryInfos)[i]))->type = NVS_TYPE_U32;
                                    }
                                    else
#endif
                                    {
                                        (&((*outEntryInfos)[i]))->type = NVS_TYPE_I32;
                                    }
                                    break;
                                }
                                case cJSON_String:
                                {
                                    (&((*outEntryInfos)[i]))->type = NVS_TYPE_BLOB;
                                    break;
                                }
                                default:
                                {
                                    break;
                                }
                            }
                            snprintf((&((*outEntryInfos)[i]))->namespace_name, NVS_KEY_NAME_MAX_SIZE, "%s",
                                    namespace);
                            snprintf((&((*outEntryInfos)[i]))->key, NVS_KEY_NAME_MAX_SIZE, "%s", current_key);
                        }
                        i++;
                    }
                }

                if (outEntryInfos == NULL)
                {
                    *numEntryInfos = i;
                }
            }

            if (freeOutStats)
            {
                free(outStats);
            }

            cJSON_Delete(json);
            return true;
        }
        else
        {
            fclose(nvsFile);
        }
    }
    return false;
}

/**
 * @brief Read info about each used entry in NVS. Typically, this should be called once with NULL passed for
 * outEntryInfos, to get the value for numEntryInfos, then memory for outEntryInfos should be allocated, then this
 * should be called again
 *
 * @param outStats If not `NULL`, the NVS stats struct will be written to this memory. It must be allocated before
 * calling readAllNvsEntryInfos()
 * @param outEntryInfos A pointer to an array of NVS entry info structs will be written to this memory
 * @param numEntryInfos If outEntryInfos is `NULL`, this will be set to the length of the given key. Otherwise, it is
 * the number of entry infos to read
 * @return true if the entry infos were read, false if they were not
 */
bool readAllNvsEntryInfos(nvs_stats_t* outStats, nvs_entry_info_t** outEntryInfos, size_t* numEntryInfos)
{
    return readNamespaceNvsEntryInfos(NVS_NAMESPACE_NAME, outStats, outEntryInfos, numEntryInfos);
}

/**
 * @brief Convert a blob to a hex string
 *
 * @param value The blob
 * @param length The length of the blob
 * @return char* An allocated hex string, must be free()'d when done
 */
static char* blobToStr(const void* value, size_t length)
{
    const uint8_t* value8 = (const uint8_t*)value;
    char* blobStr         = malloc((length * 2) + 1);
    for (size_t i = 0; i < length; i++)
    {
        sprintf(&blobStr[i * 2], "%02X", value8[i]);
    }
    return blobStr;
}

/**
 * @brief Convert a hex char to an int
 *
 * @param c A hex char, [0-9a-fA-F]
 * @return int The integer value for this char
 */
static int hexCharToInt(char c)
{
    if ('0' <= c && c <= '9')
    {
        return c - '0';
    }
    else if ('a' <= c && c <= 'f')
    {
        return 10 + (c - 'a');
    }
    else if ('A' <= c && c <= 'F')
    {
        return 10 + (c - 'A');
    }
    return 0;
}

/**
 * @brief Convert a string to a blob
 *
 * @param str A null terminated string
 * @param outBlob The blob will be written here, must already be allocated
 * @param blobLen The length of the blob to write
 */
static void strToBlob(char* str, void* outBlob, size_t blobLen)
{
    uint8_t* outBlob8 = (uint8_t*)outBlob;
    for (size_t i = 0; i < blobLen; i++)
    {
        if (((2 * i) + 1) < strlen(str))
        {
            uint8_t upperNib = hexCharToInt(str[2 * i]);
            uint8_t lowerNib = hexCharToInt(str[(2 * i) + 1]);
            outBlob8[i]      = (upperNib << 4) | (lowerNib);
        }
        else
        {
            outBlob8[i] = 0;
        }
    }
}
