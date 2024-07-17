//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>
#include "heatshrink_helper.h"
#include "badapple.h"

//==============================================================================
// Defines
//==============================================================================

#define NUM_BA_FRAMES 6569
#define BA_WIDTH      (TFT_WIDTH / 2)
#define BA_HEIGHT     (TFT_HEIGHT / 2)

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    uint8_t* rlcBuf;         ///< An array to decompress heatshrunk data into run length encoded data
    uint8_t* frameBits;      ///< An array to decode run length data into bitpacked data
    uint8_t* diffBits;       ///< The difference between two frames
    int32_t frameIdx;        ///< The current frame index
    heatshrink_decoder* hsd; ///< A persistent heatshrink decoder
    midiFile_t baSong;       ///< A song to play in the background
} badApple_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void badAppleMainLoop(int64_t elapsedUs);
static void badAppleEnterMode(void);
static void badAppleExitMode(void);
static void badAppleBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

//==============================================================================
// Strings
//==============================================================================

static const char badAppleName[] = "Bad Apple";

//==============================================================================
// Variables
//==============================================================================

/// The Swadge mode for BadApple
swadgeMode_t badAppleMode = {
    .modeName                 = badAppleName,
    .wifiMode                 = false,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = badAppleEnterMode,
    .fnExitMode               = badAppleExitMode,
    .fnMainLoop               = badAppleMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = badAppleBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

/// All state information for the BadApple mode.
badApple_t* badApple = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter BadApple mode, allocate required memory, and initialize required variables
 *
 */
static void badAppleEnterMode(void)
{
    // Allocate space
    badApple            = heap_caps_calloc(1, sizeof(badApple_t), MALLOC_CAP_DEFAULT);
    badApple->rlcBuf    = heap_caps_calloc(1, BA_WIDTH * BA_HEIGHT / 4, MALLOC_CAP_DEFAULT);
    badApple->frameBits = heap_caps_calloc(1, BA_WIDTH * BA_HEIGHT / 8, MALLOC_CAP_DEFAULT);
    badApple->diffBits  = heap_caps_calloc(1, BA_WIDTH * BA_HEIGHT / 8, MALLOC_CAP_DEFAULT);
    badApple->frameIdx  = 1;

    // Create the decoder
    badApple->hsd = heatshrink_decoder_alloc(256, 7, 3);

    // Load and play a song
    loadMidiFile("badapple.mid", &badApple->baSong, false);
    globalMidiPlayerPlaySong(&badApple->baSong, MIDI_BGM);

    // Turn off LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);

    // Set to 30FPS
    setFrameRateUs(33333);

    // Clear the display, just once
    clearPxTft();
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void badAppleExitMode(void)
{
    // Stop and free the song
    globalMidiPlayerStop(true);
    unloadMidiFile(&badApple->baSong);

    // Free the decoder
    heatshrink_decoder_free(badApple->hsd);

    // Free allocated memory
    free(badApple->rlcBuf);
    free(badApple->frameBits);
    free(badApple->diffBits);
    free(badApple);
}

/**
 * @brief The main function is responsible for decompressing and decoding data.
 * The background function will fill XOR it and fill the framebuffer
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void badAppleMainLoop(int64_t elapsedUs)
{
    // Get the name of the frame
    char fname[32] = {0};
    sprintf(fname, "ba%04" PRId32 ".bin", badApple->frameIdx);

    // Get a pointer to the file data and decompress it
    size_t fsize;
    const uint8_t* shrunkFrame = cnfsGetFile(fname, &fsize);
    uint32_t rlcSize
        = heatshrinkDecompressBuf(badApple->hsd, shrunkFrame, fsize, badApple->rlcBuf, BA_WIDTH * BA_HEIGHT / 4);

    // Undo run-length-coding
    int32_t dbIdx   = 0;
    uint8_t* rlcBuf = badApple->rlcBuf;
    while (rlcSize)
    {
        // Read a value byte
        uint8_t value = *(rlcBuf++);
        rlcSize--;

        // If it's not a zero
        if (value != 0x00)
        {
            // Copy non-zero values
            badApple->diffBits[dbIdx++] = value;
        }
        else
        {
            // Zeros indicate runs, get the count
            uint32_t count    = 0;
            uint8_t countPart = 0;
            do
            {
                countPart = *(rlcBuf++);
                rlcSize--;
                count += countPart;
            } while (0xFF == countPart);

            // Fill the run with zeros
            memset(&badApple->diffBits[dbIdx], 0x00, count);
            dbIdx += count;
        }
    }

    // Increment to the next frame
    badApple->frameIdx++;

    // If we've reached the end
    if (NUM_BA_FRAMES == badApple->frameIdx)
    {
        // Loop again
        badApple->frameIdx = 0;
        memset(badApple->rlcBuf, 0, BA_WIDTH * BA_HEIGHT / 4);
        memset(badApple->frameBits, 0, BA_WIDTH * BA_HEIGHT / 8);
        memset(badApple->diffBits, 0, BA_WIDTH * BA_HEIGHT / 8);
    }
}

/**
 * @brief Write from badApple->frameBits to the framebuffer
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void badAppleBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Get the framebuffer at the given location
    paletteColor_t* frameBuffer = getPxTftFramebuffer() + (TFT_WIDTH * y);
    paletteColor_t* lineStart   = frameBuffer;
    int32_t lineIdx             = 0;

    // For the given area
    int32_t iStart = ((BA_WIDTH * (y / 2)) + (x / 2)) / 8;
    int32_t iEnd   = ((BA_WIDTH * ((y + h) / 2)) + ((x) / 2)) / 8;
    for (int32_t i = iStart; i < iEnd; i++)
    {
        // XOR the prior frame
        badApple->frameBits[i] ^= badApple->diffBits[i];
        uint8_t byte = badApple->frameBits[i];

        // Draw each bit as a pixel
        for (int32_t j = 0; j < 8; j++)
        {
            // Double the line horizontally
            *(frameBuffer++) = (byte & (1 << j)) ? c555 : c000;
            *(frameBuffer++) = (byte & (1 << j)) ? c555 : c000;
            lineIdx += 2;

            // A the end of each line
            if (TFT_WIDTH == lineIdx)
            {
                // Double the line vertically
                memcpy(frameBuffer, lineStart, TFT_WIDTH);
                frameBuffer += TFT_WIDTH;
                lineStart = frameBuffer;
                lineIdx   = 0;
            }
        }
    }
}
