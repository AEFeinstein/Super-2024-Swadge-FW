//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include <string.h>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include "cnfs.h"

#if EMULATOR

// Stubs for emulator.

bool initSpiffs(void) { return true; }

bool deinitSpiffs(void) { return true; }

uint8_t * spiffsReadFile(const char* fname, size_t* outsize, bool readToSpiRam)
{
   #ifndef PATH_MAX
   #define PATH_MAX 4096
   #endif
   char stfilename[PATH_MAX];
   sprintf( stfilename, "spiffs_image/%s", fname );
   FILE * f = fopen( stfilename, "rb" );
   if( !f )
   {
      fprintf( stderr, "Error: could not open %s\n", stfilename );
      return 0;
   }
   fseek( f, 0, SEEK_END );
   int len = ftell( f );
   fseek( f, 0, SEEK_SET );
   uint8_t * output = malloc( len );
   if( fread( output, 1, len, f ) == len )
   {
      *outsize = len;
      return output;
   }
   else
   {
      fprintf( stderr, "Error: issue reading %s\n", stfilename );
      free( output );
      return 0;
   }
}

#else

#include "cnfs_image.h"

//==============================================================================
// Variables
//==============================================================================

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the CNFS file system. This is used to store assets like
 * WSGs and fonts
 *
 * @return true if CNFS was initialized and can be used, false if it failed
 */
bool initSpiffs(void)
{
    /* Debug print */
    ESP_LOGI("CNFS", "Size: %d", sizeof( cnfs_data ) );
    return sizeof( cnfs_data ) != 0;
}

/**
 * @brief De-initialize the CNFS file system (right now a noop)
 *
 * @return true if de-initialize ok, false if it was not
 */
bool deinitSpiffs(void)
{
    return true;
}

/**
 * @brief Read a file from CNFS into an output array. Files that are in the
 * spiffs_image folder before compilation and flashing will automatically
 * be included in the firmware.
 *
 * @param fname   The name of the file to load
 * @param outsize A pointer to a size_t to return how much data was read
 * @param readToSpiRam true to use SPI RAM, false to use normal RAM
 * @return A pointer to the read data if successful, or NULL if there is a failure
 *         This data must be freed when done
 */
uint8_t* spiffsReadFile(const char* fname, size_t* outsize, bool readToSpiRam)
{
    int low = 0;
    int high = NR_FILES - 1;
    int mid = (low+high)/2;

    // Binary search the file list, since it's sorted.
    while (low <= high)
    {
        const struct cnfsFileEntry * e = cnfs_files + mid;
        int sc = strcmp( e->name, fname );
        if( sc < 0)
        {
            low = mid + 1;
        }
        else if ( sc == 0)
        {
            *outsize = e->len;
            uint8_t * output;

            if (readToSpiRam)
                output = (uint8_t*)heap_caps_calloc((*outsize + 1), sizeof(uint8_t), MALLOC_CAP_SPIRAM);
            else
                output = (uint8_t*)calloc((*outsize + 1), sizeof(uint8_t));

            memcpy( output, &cnfs_data[e->offset], e->len );
            return output;
        }
        else
            high = mid - 1;
        mid = (low + high)/2;
    }

    ESP_LOGE( "CNFS", "Failed to open %s", fname );

    return 0;
}

#endif

