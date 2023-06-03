//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "heatshrink_helper.h"
#include "hdw-spiffs.h"
#include "spiffs_song.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Load a SNG from ROM to RAM. SNGs placed in the spiffs_image folder
 * before compilation will be automatically flashed to ROM
 *
 * @param name The filename of the SNG to load
 * @param song  A handle to load the SNG to
 * @param spiRam true to load to SPI RAM, false to load to normal RAM. SPI RAM is more plentiful but slower to access
 * than normal RAM
 * @return true if the SNG was loaded successfully,
 *         false if the SNG load failed and should not be used
 */
bool loadSong(char* name, song_t* song, bool spiRam)
{
    // Read and decompress file
    uint32_t decompressedSize = 0;
    uint8_t* decompressedBuf  = readHeatshrinkFile(name, &decompressedSize, spiRam);

    if (NULL == decompressedBuf)
    {
        return false;
    }

    // Save the decompressed info to the song. The first four bytes are the number of notes
    song->numNotes
        = (decompressedBuf[0] << 24) | (decompressedBuf[1] << 16) | (decompressedBuf[2] << 8) | decompressedBuf[3];

    // Default values, not currently saved in the file format
    song->loopStartNote = 0;
    song->shouldLoop    = false;

    // The rest of the bytes are notes
    if (spiRam)
    {
        song->notes = (musicalNote_t*)heap_caps_malloc(sizeof(musicalNote_t) * song->numNotes, MALLOC_CAP_SPIRAM);
    }
    else
    {
        song->notes = (musicalNote_t*)malloc(sizeof(musicalNote_t) * song->numNotes);
    }

    if (NULL != song->notes)
    {
        // Start after the number of notes
        uint32_t dbIdx = 4;

        // Copy all note data
        for (uint32_t noteIdx = 0; noteIdx < song->numNotes; noteIdx++)
        {
            song->notes[noteIdx].note = (decompressedBuf[dbIdx] << 8) | (decompressedBuf[dbIdx + 1]);
            dbIdx += 2;
            song->notes[noteIdx].timeMs = (decompressedBuf[dbIdx] << 8) | (decompressedBuf[dbIdx + 1]);
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
 * @param song The SNG handle to free memory from
 */
void freeSong(song_t* song)
{
    free(song->notes);
}
