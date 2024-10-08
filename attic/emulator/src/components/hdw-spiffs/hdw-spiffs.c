//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

#include "esp_log.h"
#include "hdw-spiffs.h"
#include "emu_main.h"

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
    return true;
}

/**
 * @brief De-initialize the SPI file system (SPIFFS)
 *
 * @return true if SPIFFS was de-initialized, false if it was not
 */
bool deinitSpiffs(void)
{
    return true;
}

/**
 * @brief Read a file from SPIFFS into an output array. Files that are in the
 * assets_image folder before compilation and flashing will automatically
 * be included in the firmware
 *
 * @param fname   The name of the file to load
 * @param outsize A pointer to a size_t to return how much data was read
 * @param readToSpiRam true to use SPI RAM, false to use normal RAM
 * @return A pointer to the read data if successful, or NULL if there is a failure
 *         This data must be freed when done
 */
uint8_t* spiffsReadFile(const char* fname, size_t* outsize, bool readToSpiRam)
{
    uint8_t* output;

    // Make sure the file exists, case sensitive
    bool fileExists = false;
    DIR* d;
    d = opendir("./assets_image/");
    if (d)
    {
        struct dirent* dir;
        while ((dir = readdir(d)) != NULL)
        {
            if (0 == strcmp(dir->d_name, fname))
            {
                fileExists = true;
                break;
            }
        }
        closedir(d);
    }

    // If the file does not exist
    if (false == fileExists)
    {
        // Print the error, then quit.
        // Abnormal quitting is a strong indicator something failed
        ESP_LOGE("SPIFFS", "%s doesnt exist!!!!", fname);
        return NULL;
    }

    // Read and display the contents of a small text file
    // ESP_LOGD("SPIFFS", "Reading %s", fname);

    // Open for reading the given file
    char fnameFull[128] = "./assets_image/";
    strcat(fnameFull, fname);
    FILE* f = fopen(fnameFull, "rb");
    if (f == NULL)
    {
        // ESP_LOGE("SPIFFS", "Failed to open %s", fnameFull);
        return NULL;
    }

    // Get the file size
    fseek(f, 0L, SEEK_END);
    *outsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    // Read the file into an array
    output = (uint8_t*)calloc((*outsize + 1), sizeof(uint8_t));
    if (1 != fread(output, *outsize, 1, f))
    {
        ESP_LOGE("SPIFFS", "%s fread error!!!!", fname);
        return NULL;
    }

    // Close the file
    fclose(f);

    // Display the read contents from the file
    // ESP_LOGD("SPIFFS", "Read from %s: %d bytes", fname, (uint32_t)(*outsize));
    return output;
}

int spiffsListFiles(char* out, size_t outsize, void** handle)
{
    DIR* dir = NULL;
    if (!handle)
    {
        return 0;
    }

    if (*handle)
    {
        dir = (DIR*)*handle;
    }
    else
    {
        dir = opendir("/spiffs");
        if (!dir)
        {
            return 0;
        }

        *handle = dir;
    }

    if (!dir)
    {
        return 0;
    }

    struct dirent* entry = readdir(dir);
    if (entry)
    {
        if (outsize < strlen(entry->d_name) + 1)
        {
            return -1;
        }

        strncpy(out, entry->d_name, outsize);
        out[outsize - 1] = '\0';
        return strlen(entry->d_name);
    }
    else
    {
        // No more entries
        closedir(dir);
        *handle = NULL;
        return 0;
    }
}