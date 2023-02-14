//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "heatshrink_helper.h"
#include "hdw-spiffs.h"
#include "spiffs_sng.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Load a SNG from ROM to RAM. SNGs placed in the spiffs_image folder
 * before compilation will be automatically flashed to ROM
 *
 * @param name The filename of the SNG to load. The ::song_t is not allocated by this function.
 * @param song  A handle to load the SNG to
 * @return true if the SNG was loaded successfully,
 *         false if the SNG load failed and should not be used
 */
bool loadSng(char* name, song_t* song)
{
    return loadSngSpiRam(name, song, false);
}

/**
 * @brief Load a SNG from ROM to RAM. SNGs placed in the spiffs_image folder
 * before compilation will be automatically flashed to ROM
 *
 * @param name The filename of the SNG to load
 * @param sng  A handle to load the SNG to
 * @param spiRam true to load to SPI RAM, false to load to normal RAM
 * @return true if the SNG was loaded successfully,
 *         false if the SNG load failed and should not be used
 */
bool loadSngSpiRam(char* name, song_t* sng, bool spiRam)
{
    // Read and decompress file
    uint32_t decompressedSize = 0;
    uint8_t* decompressedBuf  = readHeatshrinkFile(name, &decompressedSize, spiRam);

    // Save the decompressed info to the sng. The first four bytes are the number of notes
    sng->numNotes
        = (decompressedBuf[0] << 24) | (decompressedBuf[1] << 16) | (decompressedBuf[2] << 8) | decompressedBuf[3];

    // Default values, not currently saved in the file format
    sng->loopStartNote = 0;
    sng->shouldLoop    = false;

    // The rest of the bytes are notes
    if (spiRam)
    {
        sng->notes = (musicalNote_t*)heap_caps_malloc(sizeof(musicalNote_t) * sng->numNotes, MALLOC_CAP_SPIRAM);
    }
    else
    {
        sng->notes = (musicalNote_t*)malloc(sizeof(musicalNote_t) * sng->numNotes);
    }

    if (NULL != sng->notes)
    {
        // Start after the number of notes
        uint32_t dbIdx = 4;

        // Copy all note data
        for (uint32_t noteIdx = 0; noteIdx < sng->numNotes; noteIdx++)
        {
            sng->notes[noteIdx].note = (decompressedBuf[dbIdx] << 8) | (decompressedBuf[dbIdx + 1]);
            dbIdx += 2;
            sng->notes[noteIdx].timeMs = (decompressedBuf[dbIdx] << 8) | (decompressedBuf[dbIdx + 1]);
            dbIdx += 2;
        }
        free(decompressedBuf);
        return true;
    }

    // all done
    free(decompressedBuf);
    return false;
}

/**
 * @brief Free the memory for a loaded SNG
 *
 * @param sng The SNG handle to free memory from
 */
void freeSng(song_t* sng)
{
    free(sng->notes);
}
