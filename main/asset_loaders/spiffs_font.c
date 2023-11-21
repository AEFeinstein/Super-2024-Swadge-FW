//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "cnfs.h"
#include "spiffs_font.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Load a font from ROM to RAM. Fonts are bitmapped image files that have
 * a single height, all ASCII characters, and a width for each character.
 * PNGs placed in the assets folder before compilation will be automatically
 * flashed to ROM
 *
 * @param name The name of the font to load. The ::font_t is not allocated by this function
 * @param font A handle to load the font to
 * @param spiRam true to load to SPI RAM, false to load to normal RAM. SPI RAM is more plentiful but slower to access
 * than normal RAM
 * @return true if the font was loaded successfully
 *         false if the font failed to load and should not be used
 */
bool loadFont(const char* name, font_t* font, bool spiRam)
{
    // Read font from file
    size_t bufIdx = 0;
    uint8_t chIdx = 0;
    size_t sz;
    uint8_t* buf = spiffsReadFile(name, &sz, true);
    if (NULL == buf)
    {
        ESP_LOGE("FONT", "Failed to read %s", name);
        return false;
    }

    // Read the data into a font struct
    font->height = buf[bufIdx++];

    // Read each char
    while (bufIdx < sz)
    {
        // Get an easy reference to this character
        font_ch_t* this = &font->chars[chIdx++];

        // Read the width
        this->width = buf[bufIdx++];

        // Figure out what size the char is
        int pixels = font->height * this->width;
        int bytes  = (pixels / 8) + ((pixels % 8 == 0) ? 0 : 1);

        // Allocate space for this char and copy it over
        if (spiRam)
        {
            this->bitmap = (uint8_t*)heap_caps_malloc(sizeof(uint8_t) * bytes, MALLOC_CAP_SPIRAM);
        }
        else
        {
            this->bitmap = (uint8_t*)malloc(sizeof(uint8_t) * bytes);
        }
        memcpy(this->bitmap, &buf[bufIdx], bytes);
        bufIdx += bytes;
    }

    // Zero out any unused chars
    while (chIdx <= '~' - ' ' + 1)
    {
        font->chars[chIdx].bitmap  = NULL;
        font->chars[chIdx++].width = 0;
    }

    // Free the SPIFFS data
    free(buf);

    return true;
}

/**
 * @brief Free the memory allocated for a font
 *
 * @param font The font handle to free memory from
 */
void freeFont(font_t* font)
{
    // using uint8_t instead of char because a char will overflow to -128 after the last char is freed (\x7f)
    for (uint8_t idx = 0; idx <= '~' - ' ' + 1; idx++)
    {
        if (font->chars[idx].bitmap != NULL)
        {
            free(font->chars[idx].bitmap);
        }
    }
}
