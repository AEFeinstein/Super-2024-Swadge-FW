//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include <string.h>

#include <esp_err.h>
#include "../../tools/cnfs/image.h"
#include "../../tools/cnfs/image.c"
#include <esp_log.h>
#include <esp_heap_caps.h>
#include "hdw-spiffs.h"

//==============================================================================
// Variables
//==============================================================================

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
    /* Debug print */
    ESP_LOGI("CNFS", "Size: %d\n", sizeof( cnfs_data ) );
    return true;
}

/**
 * @brief De-initialize the SPI file system (SPIFFS)
 *
 * @return true if SPIFFS was de-initialized, false if it was not
 */
bool deinitSpiffs(void)
{
    return ESP_OK;
}

/**
 * @brief Read a file from SPIFFS into an output array. Files that are in the
 * spiffs_image folder before compilation and flashing will automatically
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
	const struct cnfsFileEntry * e = cnfs_files;
	const struct cnfsFileEntry * eend = e + NR_FILES;

	// You can binary search if you want.  But we don't have that many files, so it's plenty fast to just search.
	while( e != eend )
	{
		if( strcmp( e->name, fname ) == 0 )
		{
			*outsize = e->len;
			uint8_t * output;
			// Read the file into an array
			if (readToSpiRam)
			    output = (uint8_t*)heap_caps_calloc((*outsize + 1), sizeof(uint8_t), MALLOC_CAP_SPIRAM);
			else
			    output = (uint8_t*)calloc((*outsize + 1), sizeof(uint8_t));

			memcpy( output, &cnfs_data[e->offset], e->len );
			return output; 
		}
		e++;
	}
	ESP_LOGE( "CNFS", "Failed to open %s", fname );
	return 0;
}
