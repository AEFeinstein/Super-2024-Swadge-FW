//==============================================================================
// Includes
//==============================================================================

#include "sudoku_data.h"
#include "sudoku_game.h"

//==============================================================================
// Const data
//==============================================================================

/// @brief The NVS key to store a bitmap of boolean settings
static const char settingKeyOptionBits[] = "sdku_settings";

/// @brief The default settings bitmap
static const sudokuSettingBit_t defaultSettings = SSB_WRITE_ON_SELECT | SSB_HIGHLIGHT_POSSIBILITIES | SSB_MARK_MISTAKES;

//==============================================================================
// Functions
//==============================================================================

bool loadSudokuData(const uint8_t* data, size_t length, sudokuGrid_t* game)
{
    if (!length || !data)
    {
        return false;
    }

    // Clear the sudoku game
    deinitSudokuGame(game);

    const uint8_t* cur = data;

    // 8 bytes of header
    // + (boxFormat) ? ((size * size + 1) / 2) : 0
    // + (gridFormat) ? (size * size) : (1 + (numCount + 1) / 2 * 3)
    // + (noteFormat) ? (size * size * 2) : (1 + noteCount * 3)
    // + (flagFormat) ? (size * size) : (1 + flagCount * 2)
    // Version 0 only for now
    uint8_t version = *cur++;

    // Mode:
    // - 0: Regular
    // - 1: Jigsaw, 2: X?
    sudokuMode_t mode = (sudokuMode_t)*cur++;

    // Grid size: grid size in each dimension
    uint8_t size = *cur++;

    // Base: total number of numbers/rows/boxes
    uint8_t base = *cur++;

    // Box data format:
    // - 0: Predefined (square)
    // - 1: Individual (full grid)
    uint8_t boxFormat = *cur++;

    // Grid format:
    // 0: Mapped (smaller for sparse puzzles)
    // 1: Individual (full grid)
    uint8_t gridFormat = *cur++;

    // Notes format:
    // - 0: Mapped
    // - 1: Individual (full grid)
    uint8_t noteFormat = *cur++;

    // Flags format:
    // - 0: Mapped
    // - 1: Individual (full grid)
    uint8_t flagsFormat = *cur++;

    ESP_LOGD("Swadgedoku", "Loading sudoku puzzle");
    ESP_LOGD("Swadgedoku", "- Mode: %" PRIu8, mode);
    ESP_LOGD("Swadgedoku", "- Size: %" PRIu8 "x%" PRIu8, size, size);
    ESP_LOGD("Swadgedoku", "- Base: %" PRIu8, base);
    ESP_LOGD("Swadgedoku", "- Format (Box/Grid/Note/Flags): %" PRIu8 "/%" PRIu8 "/%" PRIu8 "/%" PRIu8, boxFormat,
             gridFormat, noteFormat, flagsFormat);

    if (version != 0)
    {
        ESP_LOGE("Swadgedoku", "Cannot load unsupported version %" PRIu8, version);
        return false;
    }

    if (!initSudokuGame(game, size, base, mode))
    {
        return false;
    }

    switch (boxFormat)
    {
        case 0:
        {
            // Standard
            // Boxes are boring squares
            int baseRoot = 0;
            switch (base)
            {
                case 1:
                    baseRoot = 1;
                    break;
                case 4:
                    baseRoot = 2;
                    break;
                case 9:
                    baseRoot = 3;
                    break;
                case 16:
                    baseRoot = 4;
                    break;
                default:
                    break;
            }

            if (baseRoot == 0)
            {
                ESP_LOGE("Swadgedoku", "Invalid 'standard' box configuration for non-square base %" PRIu8, base);
                deinitSudokuGame(game);
                return false;
            }

            // Setup square boxes!
            for (int box = 0; box < base; box++)
            {
                for (int n = 0; n < base; n++)
                {
                    int x                      = (box % baseRoot) * baseRoot + (n % baseRoot);
                    int y                      = (box / baseRoot) * baseRoot + (n / baseRoot);
                    game->boxMap[y * size + x] = box;
                }
            }

            break;
        }

        case 1:
        {
            // Individual
            // Boxes are defined in a <size>X<size> map (packed slightly)
            const uint8_t* boxMapStart = cur;
            // We can store two boxes per byte
            // Since boxes can never be larger than 16
            const uint8_t* boxMapEnd = cur + (size * size + 1) / 2;

            while (cur < boxMapEnd)
            {
                int n = (cur - boxMapStart) * 2;

                // First box in the pair is stored in the most-significant nibble
                game->boxMap[n] = (*cur >> 4) & 0x0F;

                // Make sure we don't have an empty LSB here
                if (n + 1 < size * size)
                {
                    // Second box in the pair is stored in the least-significant nibble
                    game->boxMap[n + 1] = *cur & 0x0F;
                }

                cur++;
            }
            break;
        }

        default:
        {
            ESP_LOGE("Swagedoku", "Invalid box format %" PRIu8, boxFormat);
            return false;
        }
    }

    switch (gridFormat)
    {
        case 0:
        {
            // Mapped
            // mapLength is the number of VALUES used by the map
            uint8_t mapLength = *cur++;
            // Convert mapLength to the total number of bytes
            const uint8_t* mapEnd = cur + (mapLength + 1) / 2 * 3;

            int n = 0;

            while (cur < mapEnd)
            {
                // Map format:
                // Byte 0 is location, with row in the most-significant nibble
                // and column in the least-significant nibble
                // Byte 1 is a second location
                // Byte 2 is two values. The most-significant nibble is the value at the location in byte 0
                // The least-significant nibble is the value at the location in byte 1
                uint8_t loc0 = *cur++;
                uint8_t loc1 = *cur++;
                uint8_t vals = *cur++;

                game->grid[((loc0 >> 4) & 0x0F) * game->size + (loc0 & 0x0F)] = ((vals >> 4) & 0x0F) + 1;
                n++;

                if (n < mapLength)
                {
                    game->grid[((loc1 >> 4) & 0x0F) * game->size + (loc1 & 0x0F)] = ((vals & 0x0F)) + 1;
                    n++;
                }
            }
            break;
        }

        case 1:
        {
            // Full grid
            // We'd like to pack the grid a bit more but it won't fit
            // We need to be able to store a 0 (no digit) as well as the digit
            // So, we need a full byte, with 0 and 1-16 -- technically 5 bytes
            // 3 wasted bits, unless we use it to store pen/not-pen status? no...
            const uint8_t* gridStart = cur;
            const uint8_t* gridEnd   = cur + game->size * game->size;

            while (cur < gridEnd)
            {
                game->grid[cur - gridStart] = *cur;
                cur++;
            }

            break;
        }
    }

    switch (noteFormat)
    {
        case 0:
        {
            // Mapped
            // It's 3 bytes per square here!!!
            // 1 byte for the location and 2 for the actual notes
            uint8_t mapLength       = *cur++;
            const uint8_t* notesEnd = cur + 3 * mapLength;

            while (cur < notesEnd)
            {
                uint8_t pos = *cur++;
                // MSB first
                uint16_t note = (*cur++) << 8;
                // LSB next
                note |= *cur++;
                game->notes[((pos >> 4) & 0x0F) * game->size + (pos & 0x0F)] = note;
            }
            break;
        }

        case 1:
        {
            const uint8_t* notesStart = cur;
            const uint8_t* notesEnd   = cur + 2 * game->size * game->size;

            while (cur < notesEnd)
            {
                uint16_t note = (*cur++) << 8;
                note |= *cur++;

                game->notes[(cur - notesStart - 2) / 2] = note;
            }
            break;
        }
    }

    switch (flagsFormat)
    {
        case 0:
        {
            // Mapped
            // It's 2 bytes per square so we can have up to 8 flags
            uint8_t mapLength     = *cur++;
            const uint8_t* mapEnd = cur + 2 * mapLength;

            while (cur < mapEnd)
            {
                uint8_t pos  = *cur++;
                uint8_t flag = *cur++;

                game->flags[((pos >> 4) & 0x0F) * game->size + (pos & 0x0F)] = (sudokuFlag_t)flag;
            }
            break;
        }
        case 1:
        {
            // Individual
            const uint8_t* flagsStart = cur;
            const uint8_t* flagsEnd   = cur + game->size * game->size;

            while (cur < flagsEnd)
            {
                game->flags[cur - flagsStart] = (sudokuFlag_t)*cur;
                cur++;
            }
            break;
        }
    }

    return true;
}

size_t writeSudokuData(uint8_t* data, const sudokuGrid_t* game)
{
    int boxFmt, gridFmt, noteFmt, flagFmt;
    getSudokuSaveSize(game, &boxFmt, &gridFmt, &noteFmt, &flagFmt);

    uint8_t* out = data;

    // write header
    // version
    *out++ = 0;
    // game mode
    *out++ = (uint8_t)game->mode;
    // grid size
    *out++ = game->size;
    // base
    *out++ = game->base;
    // box format
    *out++ = boxFmt;
    // grid format
    *out++ = gridFmt;
    // note format
    *out++ = noteFmt;
    // flags format
    *out++ = flagFmt;

    switch (boxFmt)
    {
        case 0:
            // Write no data
            break;

        case 1:
        {
            for (int n = 0; n < game->size * game->size; n += 2)
            {
                uint8_t boxPair = (game->boxMap[n] & 0x0F) << 4;

                if (n + 1 < game->size * game->size)
                {
                    boxPair |= (game->boxMap[n + 1] & 0x0F);
                }

                *out++ = boxPair;
            }
            break;
        }
    }

    switch (gridFmt)
    {
        case 0:
        {
            // Mapped
            int count = 0;
            for (int n = 0; n < game->size * game->size; n++)
            {
                if (game->grid[n] != 0)
                {
                    count++;
                }
            }

            *out++ = (count & 0xFF);

            // Holds the nibble for the cell value
            uint8_t buf  = 0;
            bool pending = false;

            for (int n = 0; n < game->size * game->size; n++)
            {
                if (game->grid[n] != 0)
                {
                    int r = n / game->size;
                    int c = n % game->size;

                    if (pending)
                    {
                        // Write second location of pair
                        *out++ = ((r & 0x0F) << 4) | (c & 0x0F);
                        buf |= (game->grid[n] - 1);
                        // Write accompanying numerical values
                        *out++  = buf;
                        pending = false;
                    }
                    else
                    {
                        // Write first location of pair
                        *out++  = ((r & 0x0F) << 4) | (c & 0x0F);
                        buf     = (game->grid[n] - 1) << 4;
                        pending = true;
                    }
                }
            }

            // Write any remaining value
            if (pending)
            {
                // Write an empty location
                *out++ = 0;

                // Write the pending value
                *out++ = buf;
            }
            break;
        }

        case 1:
        {
            // Individual
            for (int n = 0; n < game->size * game->size; n++)
            {
                *out++ = game->grid[n];
            }
            break;
        }
    }

    switch (noteFmt)
    {
        case 0:
        {
            // Mapped

            int count = 0;
            for (int n = 0; n < game->size * game->size; n++)
            {
                if (game->notes[n] != 0)
                {
                    count++;
                }
            }

            *out++ = (count & 0xFF);

            for (int n = 0; n < game->size * game->size; n++)
            {
                if (game->notes[n] != 0)
                {
                    int r  = n / game->size;
                    int c  = n % game->size;
                    *out++ = ((r & 0x0F) << 4) | (c & 0x0F);
                    *out++ = (game->notes[n] >> 8) & 0xFF;
                    *out++ = (game->notes[n]) & 0xFF;
                }
            }

            break;
        }

        case 1:
        {
            // Individual
            for (int n = 0; n < game->size * game->size; n++)
            {
                *out++ = (game->notes[n] >> 8) & 0xFF;
                *out++ = game->notes[n] & 0xFF;
            }
            break;
        }
    }

    switch (flagFmt)
    {
        case 0:
        {
            // Mapped
            int count = 0;
            for (int n = 0; n < game->size * game->size; n++)
            {
                if (game->flags[n] != SF_NONE)
                {
                    count++;
                }
            }

            *out++ = (count & 0xFF);
            for (int n = 0; n < game->size * game->size; n++)
            {
                if (game->flags[n] != SF_NONE)
                {
                    int r  = n / game->size;
                    int c  = n % game->size;
                    *out++ = ((r & 0x0F) << 4) | (c & 0x0F);
                    *out++ = (uint8_t)game->flags[n];
                }
            }
            break;
        }

        case 1:
        {
            // Individual
            for (int n = 0; n < game->size * game->size; n++)
            {
                *out++ = (uint8_t)game->flags[n];
            }
            break;
        }
    }

    return out - data;
}

size_t getSudokuSaveSize(const sudokuGrid_t* game, int* boxFmt, int* gridFmt, int* noteFmt, int* flagFmt)
{
    // 8 bytes of header
    // + (boxFormat) ? ((size * size + 1) / 2) : 0
    // + (gridFormat) ? (size * size) : (1 + (numCount + 1) / 2 * 3)
    // + (noteFormat) ? (size * size * 2) : (1 + noteCount * 3)
    // + (flagFormat) ? (size * size) : (1 + flagCount * 2)

    // Default to square boxes
    int boxFormat = 0;

    // Check for regular boxes
    int baseRoot = 0;
    switch (game->base)
    {
        case 1:
            baseRoot = 1;
            break;
        case 4:
            baseRoot = 2;
            break;
        case 9:
            baseRoot = 3;
            break;
        case 16:
            baseRoot = 4;
            break;
        default:
            break;
    }

    if (baseRoot != 0)
    {
        // Setup square boxes!
        for (int box = 0; box < game->base && !boxFormat; box++)
        {
            for (int n = 0; n < game->base; n++)
            {
                int x = (box % baseRoot) * baseRoot + (n % baseRoot);
                int y = (box / baseRoot) * baseRoot + (n / baseRoot);

                if (game->boxMap[y * game->size + x] != box)
                {
                    // We can't use square boxes, stop checking
                    boxFormat = 1;
                    break;
                }
            }
        }
    }
    else
    {
        boxFormat = 1;
    }

    int numCount  = 0;
    int noteCount = 0;
    int flagCount = 0;

    // Count number of elements we'd need to save
    for (int n = 0; n < game->size * game->size; n++)
    {
        if (game->grid[n] != 0)
        {
            numCount++;
        }

        if (game->notes[n] != 0)
        {
            noteCount++;
        }

        if (game->flags[n] != SF_NONE)
        {
            flagCount++;
        }
    }

    const int gridArea = game->size * game->size;

    // Decide which is smaller
    int gridLenF0 = (1 + (numCount + 1) / 2 * 3);
    int gridLenF1 = game->size * game->size;

    int noteLenF0 = (1 + noteCount * 3);
    int noteLenF1 = (game->size * game->size * 2);

    int flagLenF0 = (1 + flagCount * 2);
    int flagLenF1 = (game->size * game->size);

    int gridFormat = (gridLenF0 < gridLenF1) ? 0 : 1;
    int noteFormat = (noteLenF0 < noteLenF1) ? 0 : 1;
    int flagFormat = (flagLenF0 < flagLenF1) ? 0 : 1;

    int boxLen  = boxFormat ? ((gridArea + 1) / 2) : 0;
    int gridLen = gridFormat ? gridLenF1 : gridLenF0;
    int noteLen = noteFormat ? noteLenF1 : noteLenF0;
    int flagLen = flagFormat ? flagLenF1 : flagLenF0;

    // Now we know how big it will be!

    if (boxFmt)
    {
        *boxFmt = boxFormat;
    }

    if (gridFmt)
    {
        *gridFmt = gridFormat;
    }

    if (noteFmt)
    {
        *noteFmt = noteFormat;
    }

    if (flagFmt)
    {
        *flagFmt = flagFormat;
    }

    size_t totalLen = 8 + boxLen + gridLen + noteLen + flagLen;

    return totalLen;
}

void swadgedokuLoadSettings(sudokuSettings_t* settings)
{
    int32_t allSettings = 0;
    if (!readNvs32(settingKeyOptionBits, &allSettings))
    {
        allSettings = (int32_t)defaultSettings;
    }

    settings->writeOnSelect          = (allSettings & SSB_WRITE_ON_SELECT) ? true : false;
    settings->autoAnnotate           = (allSettings & SSB_AUTO_ANNOTATE) ? true : false;
    settings->highlightPossibilities = (allSettings & SSB_HIGHLIGHT_POSSIBILITIES) ? true : false;
    settings->highlightOnlyOptions   = (allSettings & SSB_HIGHLIGHT_ONLY_OPTIONS) ? true : false;
    settings->markMistakes           = (allSettings & SSB_MARK_MISTAKES) ? true : false;
}

void swadgedokuSaveSettings(const sudokuSettings_t* settings)
{
    int32_t allSettings = 0;
    allSettings |= settings->writeOnSelect ? SSB_WRITE_ON_SELECT : 0;
    allSettings |= settings->autoAnnotate ? SSB_AUTO_ANNOTATE : 0;
    allSettings |= settings->highlightPossibilities ? SSB_HIGHLIGHT_POSSIBILITIES : 0;
    allSettings |= settings->highlightOnlyOptions ? SSB_HIGHLIGHT_ONLY_OPTIONS : 0;
    allSettings |= settings->markMistakes ? SSB_MARK_MISTAKES : 0;

    if (!writeNvs32(settingKeyOptionBits, allSettings))
    {
        ESP_LOGE("Swadgedoku", "Could not save swadgedoku settings!");
    }
}
