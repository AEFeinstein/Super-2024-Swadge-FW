//==============================================================================
// Includes
//==============================================================================

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>

#include "hdw-nvs.h"

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
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    // Check error
    switch (err)
    {
        case ESP_OK:
        {
            // NVS opened
            return true;
        }
        case ESP_ERR_NVS_NO_FREE_PAGES:
        case ESP_ERR_NVS_NEW_VERSION_FOUND:
        {
            // If this is the first try
            if (true == firstTry)
            {
                // NVS partition was truncated and needs to be erased
                // Retry nvs_flash_init
                switch (nvs_flash_erase())
                {
                    case ESP_OK:
                    {
                        // NVS erased, try initializing one more time
                        return initNvs(false);
                    }
                    default:
                    case ESP_ERR_NOT_FOUND:
                    {
                        // Couldn't erase flash
                        return false;
                    }
                }
            }
            else
            {
                // Failed twice, return false
                return false;
            }
            break;
        }
        default:
        case ESP_ERR_NOT_FOUND:
        case ESP_ERR_NO_MEM:
        {
            // NVS not opened
            return false;
        }
    }
}

/**
 * @brief Deinitialize NVS
 *
 * @return true if NVS was deinitialized, false if it failed
 */
bool deinitNvs(void)
{
    return (ESP_OK == nvs_flash_deinit());
}

/**
 * @brief Erase and re-initialize the nonvolatile storage
 *
 * @return true if NVS was erased and re-initialized and can be used, false if it failed
 */
bool eraseNvs(void)
{
    switch (nvs_flash_erase())
    {
        case ESP_OK:
        {
            // NVS erased successfully, need to re-initialize
            return initNvs(true);
        }
        default:
        case ESP_ERR_NOT_FOUND:
        {
            // Couldn't erase flash
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
    nvs_handle_t handle;
    esp_err_t openErr = nvs_open(NVS_NAMESPACE_NAME, NVS_READONLY, &handle);
    switch (openErr)
    {
        case ESP_OK:
        {
            // Assume the commit is bad
            bool readOk = false;
            // Write the NVS
            esp_err_t readErr = nvs_get_i32(handle, key, outVal);
            // Check the write error
            switch (readErr)
            {
                case ESP_OK:
                {
                    readOk = true;
                    break;
                }
                default:
                case ESP_ERR_NVS_NOT_FOUND: // This is the error when a nonexistent key is read
                case ESP_ERR_NVS_INVALID_HANDLE:
                case ESP_ERR_NVS_INVALID_NAME:
                case ESP_ERR_NVS_INVALID_LENGTH:
                {
                    ESP_LOGE("NVS", "%s readErr %s", __func__, esp_err_to_name(readErr));
                    break;
                }
            }
            // Close the handle
            nvs_close(handle);
            return readOk;
        }
        default:
        case ESP_ERR_NVS_NOT_INITIALIZED:
        case ESP_ERR_NVS_PART_NOT_FOUND:
        case ESP_ERR_NVS_NOT_FOUND:
        case ESP_ERR_NVS_INVALID_NAME:
        case ESP_ERR_NO_MEM:
        {
            ESP_LOGE("NVS", "%s openErr %s", __func__, esp_err_to_name(openErr));
            return false;
        }
    }
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
    nvs_handle_t handle;
    esp_err_t openErr = nvs_open(NVS_NAMESPACE_NAME, NVS_READWRITE, &handle);
    switch (openErr)
    {
        case ESP_OK:
        {
            // Assume the commit is bad
            bool commitOk = false;
            // Write the NVS
            esp_err_t writeErr = nvs_set_i32(handle, key, val);
            // Check the write error
            switch (writeErr)
            {
                case ESP_OK:
                {
                    // Commit NVS
                    commitOk = (ESP_OK == nvs_commit(handle));
                    break;
                }
                default:
                case ESP_ERR_NVS_INVALID_HANDLE:
                case ESP_ERR_NVS_READ_ONLY:
                case ESP_ERR_NVS_INVALID_NAME:
                case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
                case ESP_ERR_NVS_REMOVE_FAILED:
                {
                    ESP_LOGE("NVS", "%s err %s", __func__, esp_err_to_name(writeErr));
                    break;
                }
            }

            // Close the handle
            nvs_close(handle);
            return commitOk;
        }
        default:
        case ESP_ERR_NVS_NOT_INITIALIZED:
        case ESP_ERR_NVS_PART_NOT_FOUND:
        case ESP_ERR_NVS_NOT_FOUND:
        case ESP_ERR_NVS_INVALID_NAME:
        case ESP_ERR_NO_MEM:
        {
            ESP_LOGE("NVS", "%s openErr %s", __func__, esp_err_to_name(openErr));
            return false;
        }
    }
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
    nvs_handle_t handle;
    esp_err_t openErr = nvs_open(namespace, NVS_READONLY, &handle);
    switch (openErr)
    {
        case ESP_OK:
        {
            // Assume the commit is bad
            bool readOk = false;
            // Write the NVS
            esp_err_t readErr = nvs_get_blob(handle, key, out_value, length);
            // Check the write error
            switch (readErr)
            {
                case ESP_OK:
                {
                    readOk = true;
                    break;
                }
                default:
                case ESP_ERR_NVS_NOT_FOUND: // This is the error when a nonexistent key is read
                case ESP_ERR_NVS_INVALID_HANDLE:
                case ESP_ERR_NVS_INVALID_NAME:
                case ESP_ERR_NVS_INVALID_LENGTH:
                {
                    ESP_LOGE("NVS", "%s readErr %s", __func__, esp_err_to_name(readErr));
                    break;
                }
            }
            // Close the handle
            nvs_close(handle);
            return readOk;
        }
        default:
        case ESP_ERR_NVS_NOT_INITIALIZED:
        case ESP_ERR_NVS_PART_NOT_FOUND:
        case ESP_ERR_NVS_NOT_FOUND:
        case ESP_ERR_NVS_INVALID_NAME:
        case ESP_ERR_NO_MEM:
        {
            ESP_LOGE("NVS", "%s openErr %s", __func__, esp_err_to_name(openErr));
            return false;
        }
    }
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
    nvs_handle_t handle;
    esp_err_t openErr = nvs_open(namespace, NVS_READWRITE, &handle);
    switch (openErr)
    {
        case ESP_OK:
        {
            // Assume the commit is bad
            bool commitOk = false;
            // Write the NVS
            esp_err_t writeErr = nvs_set_blob(handle, key, value, length);
            // Check the write error
            switch (writeErr)
            {
                case ESP_OK:
                {
                    // Commit NVS
                    commitOk = (ESP_OK == nvs_commit(handle));
                    break;
                }
                default:
                case ESP_ERR_NVS_INVALID_HANDLE:
                case ESP_ERR_NVS_READ_ONLY:
                case ESP_ERR_NVS_INVALID_NAME:
                case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
                case ESP_ERR_NVS_REMOVE_FAILED:
                {
                    ESP_LOGE("NVS", "%s err %s", __func__, esp_err_to_name(writeErr));
                    break;
                }
            }

            // Close the handle
            nvs_close(handle);
            return commitOk;
        }
        default:
        case ESP_ERR_NVS_NOT_INITIALIZED:
        case ESP_ERR_NVS_PART_NOT_FOUND:
        case ESP_ERR_NVS_NOT_FOUND:
        case ESP_ERR_NVS_INVALID_NAME:
        case ESP_ERR_NO_MEM:
        {
            ESP_LOGE("NVS", "%s openErr %s", __func__, esp_err_to_name(openErr));
            return false;
        }
    }
}

/**
 * @brief Delete the value with the given key from NVS
 *
 * @param key The NVS key to be deleted
 * @return true if the value was deleted, false if it was not
 */
bool eraseNvsKey(const char* key)
{
    nvs_handle_t handle;
    esp_err_t openErr = nvs_open(NVS_NAMESPACE_NAME, NVS_READWRITE, &handle);
    switch (openErr)
    {
        case ESP_OK:
        {
            // Assume the commit is bad
            bool commitOk = false;
            // Write the NVS
            esp_err_t writeErr = nvs_erase_key(handle, key);
            // Check the write error
            switch (writeErr)
            {
                case ESP_OK:
                {
                    // Commit NVS
                    commitOk = (ESP_OK == nvs_commit(handle));
                    break;
                }
                default:
                case ESP_ERR_NVS_INVALID_HANDLE:
                case ESP_ERR_NVS_READ_ONLY:
                case ESP_ERR_NVS_INVALID_NAME:
                case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
                case ESP_ERR_NVS_REMOVE_FAILED:
                {
                    ESP_LOGE("NVS", "%s err %s", __func__, esp_err_to_name(writeErr));
                    break;
                }
            }

            // Close the handle
            nvs_close(handle);
            return commitOk;
        }
        default:
        case ESP_ERR_NVS_NOT_INITIALIZED:
        case ESP_ERR_NVS_PART_NOT_FOUND:
        case ESP_ERR_NVS_NOT_FOUND:
        case ESP_ERR_NVS_INVALID_NAME:
        case ESP_ERR_NO_MEM:
        {
            ESP_LOGE("NVS", "%s openErr %s", __func__, esp_err_to_name(openErr));
            return false;
        }
    }
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
    esp_err_t readErr = nvs_get_stats(NULL, outStats);

    switch (readErr)
    {
        case ESP_OK:
        {
            return true;
        }
        default:
        case ESP_ERR_INVALID_ARG:
        {
            ESP_LOGE("NVS", "%s err %s", __func__, esp_err_to_name(readErr));
            return false;
        }
    }
}

/**
 * @brief Read info about each used entry in a specific NVS namespace. Typically, this should be called once with NULL passed for
 * outEntryInfos, to get the value for numEntryInfos, then memory for outEntryInfos should be allocated, then this
 * should be called again
 *
 * @param namespace The name of the NVS namespace to use
 * @param outStats If not `NULL`, the NVS stats struct will be written to this memory. It must be allocated before
 * calling readAllNvsEntryInfos()
 * @param outEntryInfos A pointer to an array of NVS entry info structs will be written to this memory
 * @param numEntryInfos If outEntryInfos is `NULL`, this will be set to the length of the given key. Otherwise, it is
 * the number of entry infos to read
 * @return true if the entry infos were read, false if they were not
 */
bool readNamespaceNvsEntryInfos(const char* namespace, nvs_stats_t* outStats, nvs_entry_info_t** outEntryInfos, size_t* numEntryInfos)
{
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

    // Example of listing all the key-value pairs of any type under specified partition and namespace
    size_t i          = 0;
    nvs_iterator_t it = NULL;
    esp_err_t res     = nvs_entry_find(namespace, NULL, NVS_TYPE_ANY, &it);
    while (res == ESP_OK)
    {
        if (outEntryInfos != NULL)
        {
            nvs_entry_info(it, &((*outEntryInfos)[i]));
            i++;
        }
        res = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);

    if (freeOutStats)
    {
        free(outStats);
    }

    if (outEntryInfos == NULL)
    {
        *numEntryInfos = i;
    }
    return true;
}

/**
 * @brief Read info about each used entry in the default NVS namespace. Typically, this should be called once with NULL passed for
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
