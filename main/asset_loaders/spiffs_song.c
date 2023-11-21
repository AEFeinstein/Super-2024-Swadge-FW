//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "heatshrink_helper.h"
#include "cnfs.h"
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
bool loadSong(const char* name, song_t* song, bool spiRam)
{
    uint32_t caps = spiRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_DEFAULT;

    // Read and decompress file
    uint32_t decompressedSize = 0;
    uint8_t* decompressedBuf  = readHeatshrinkFile(name, &decompressedSize, spiRam);

    if (NULL == decompressedBuf)
    {
        return false;
    }

    // Default value, not in the file format
    song->shouldLoop = false;

    uint32_t dbIdx = 0;

    // Save the number of channels in this song
    song->numTracks = (decompressedBuf[dbIdx] << 24) | (decompressedBuf[dbIdx + 1] << 16)
                      | (decompressedBuf[dbIdx + 2] << 8) | decompressedBuf[dbIdx + 3];
    dbIdx += 4;

    // Allocate each channel
    song->tracks = heap_caps_calloc(song->numTracks, sizeof(songTrack_t), caps);

    if (NULL == song->tracks)
    {
        // Allocation failed
        free(decompressedBuf);
        return false;
    }

    // For each channel
    for (int16_t cIdx = 0; cIdx < song->numTracks; cIdx++)
    {
        // Get a convenience pointer to this channel
        songTrack_t* ch = &song->tracks[cIdx];

        // Default values, not currently saved in the file format
        ch->loopStartNote = 0;

        // Save the number of notes in this channel
        ch->numNotes = (decompressedBuf[dbIdx] << 24) | (decompressedBuf[dbIdx + 1] << 16)
                       | (decompressedBuf[dbIdx + 2] << 8) | decompressedBuf[dbIdx + 3];
        dbIdx += 4;

        // The rest of the bytes are notes
        ch->notes = (musicalNote_t*)heap_caps_calloc(ch->numNotes, sizeof(musicalNote_t), caps);
        if (NULL != ch->notes)
        {
            // Copy all note data
            for (uint32_t noteIdx = 0; noteIdx < ch->numNotes; noteIdx++)
            {
                ch->notes[noteIdx].note = (decompressedBuf[dbIdx] << 8) | (decompressedBuf[dbIdx + 1]);
                dbIdx += 2;
                ch->notes[noteIdx].timeMs = (decompressedBuf[dbIdx] << 8) | (decompressedBuf[dbIdx + 1]);
                dbIdx += 2;
            }
        }
        else
        {
            // Allocation failed
            free(decompressedBuf);
            return false;
        }
    }

    // all done
    free(decompressedBuf);
    return true;
}

/**
 * @brief Free the memory for a loaded SNG
 *
 * @param song The SNG handle to free memory from
 */
void freeSong(song_t* song)
{
    for (int16_t cIdx = 0; cIdx < song->numTracks; cIdx++)
    {
        free(song->tracks[cIdx].notes);
    }
    free(song->tracks);
}
