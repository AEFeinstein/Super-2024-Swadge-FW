/*! \file hdw-spiffs.c
 *
 * \section spiffs_design Design Philosophy
 *
 * TODO doxygen
 *
 * \section spiffs_usage Usage
 *
 * TODO doxygen
 *
 * \section spiffs_example Example
 *
 * \code{.c}
 * TODO doxygen
 * \endcode
 */

//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "hdw-spiffs.h"
#include "spiffs_config.h"

//==============================================================================
// Variables
//==============================================================================

/* Config data */
static const esp_vfs_spiffs_conf_t conf
    = {.base_path = "/spiffs", .partition_label = NULL, .max_files = 5, .format_if_mount_failed = false};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the SPI file system (SPIFFS). This is used to store assets
 * like WSGs and fonts
 *
 * @return true if SPIFFS was initialized and can be used, false if it failed
 */
bool initSpiffs(void)
{
    /* Initialize SPIFFS
     * Use settings defined above to initialize and mount SPIFFS filesystem.
     * Note: esp_vfs_spiffs_register is an all-in-one convenience function.
     */
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    /* Debug print */
    size_t total = 0, used = 0;
    ESP_ERROR_CHECK(esp_spiffs_info(NULL, &total, &used));
    ESP_LOGI("SPIFFS", "Partition size: total: %d, used: %d", total, used);

    return true;
}

/**
 * @brief De-initialize the SPI file system (SPIFFS)
 *
 * @return true if SPIFFS was de-initialized, false if it was not
 */
bool deinitSpiffs(void)
{
    return (ESP_OK == esp_vfs_spiffs_unregister(conf.partition_label));
}

/**
 * @brief Read a file from SPIFFS into an output array. Files that are in the
 * spiffs_image folder before compilation and flashing will automatically
 * be included in the firmware
 *
 * @param fname   The name of the file to load
 * @param output  A pointer to a pointer to return the read data in. This memory
 *                will be allocated with calloc(). Must be NULL to start
 * @param outsize A pointer to a size_t to return how much data was read
 * @param readToSpiRam true to use SPI RAM, false to use normal RAM
 * @return true if the file was read successfully, false otherwise
 */
bool spiffsReadFile(const char* fname, uint8_t** output, size_t* outsize, bool readToSpiRam)
{
    // Make sure the output pointer is NULL to begin with
    if (NULL != *output)
    {
        ESP_LOGE("SPIFFS", "output not NULL");
        return false;
    }

    // Read and display the contents of a small text file
    ESP_LOGI("SPIFFS", "Reading %s", fname);

    // Open for reading the given file
    char fnameFull[128] = "/spiffs/";
    strcat(fnameFull, fname);
    FILE* f = fopen(fnameFull, "rb");
    if (f == NULL)
    {
        ESP_LOGE("SPIFFS", "Failed to open %s", fnameFull);
        return false;
    }

    // Get the file size
    fseek(f, 0L, SEEK_END);
    *outsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    // Read the file into an array
    if (readToSpiRam)
    {
        *output = (uint8_t*)heap_caps_calloc((*outsize + 1), sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    }
    else
    {
        *output = (uint8_t*)calloc((*outsize + 1), sizeof(uint8_t));
    }
    fread(*output, *outsize, 1, f);
    // Add null terminator
    (*output)[*outsize] = 0;

    // Close the file
    fclose(f);

    // Display the read contents from the file
    ESP_LOGI("SPIFFS", "Read from %s: %u bytes", fname, *outsize);
    return true;
}
