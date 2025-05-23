/**
 * @file trophy.c
 * @author Jeremy.Stintzcum@gmail.com
 * @brief Trophies for swadge modes
 * @date 2025-05-22
 */

//==============================================================================
// Includes
//==============================================================================

#include "trophy.h"

// C
#include <string.h>
#include <inttypes.h>

// Hardware
#include "hdw-nvs.h"
#include "hdw-tft.h"
#include "esp_log.h"

// ModeList
#include "modeIncludeList.h"

// Drawing
#include "fs_font.h"
#include "fs_wsg.h"
#include "fill.h"
#include "shapes.h"

// Data Structure
#include "linked_list.h"

//==============================================================================
// Defines
//==============================================================================

// Standard defines
#define DEFAULT_MILESTONE 25

// Visuals
#define BANNER_HEIGHT           48
#define BANNER_MAX_ICON_DIM     36
#define SCREEN_CORNER_CLEARANCE 19
#define IMAGE_BUFFER            8
#define NUMBER_TEXT_BUFFER      25
#define NUM_COLORS              6

//==============================================================================
// Consts
//==============================================================================

static const char* const NVSstrings[] = {"trophy", "points", "latest", "mode"};

static const char* const platStrings[] = {"All ", " Trophies Won!", "Win all the trophies for "};

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    trophyData_t trophyData; ///< Individual trophy data
    int32_t currentVal;      ///< Saved value of the trophy
    wsg_t image;             ///< Where the image is loaded
    bool active;             ///< If this slot is loaded and ready to animate
} trophyDataWrapper_t;

// Used to display the list of trophies
typedef struct
{
    int* heights;                 ///< Total height of the stack
    int platHeight;               ///< Height of ther plat frame
    wsg_t* images;                ///< Array of images to display
    trophyListDisplayMode_t mode; ///< Current display mode

    // Colors
    paletteColor_t colorList[NUM_COLORS];
} trophyDisplayList_t;

// System variables for display,
typedef struct
{
    // Data
    trophyDataList_t* data; ///< The settings of how the trophies behave
    list_t trophyQueue;     ///< List of trophy updates to display. Holds type ::trophyDataWrapper_t*
    int numTrophiesScore;   ///< Num of trophies adjusted for difficulty

    // Platinum
    trophyData_t plat; ///< Platinum trophy data
    int32_t platVal;   ///< Value of the platinum trophy
    wsg_t platImg;     ///< Platinum's image

    // Drawing
    bool active;                ///< If the mode should be drawing a banner
    int32_t animTimer;          ///< Timer used for sliding in and out
    wsgPalette_t grayPalette;   ///< Grayscale palette for locked trophies
    wsgPalette_t normalPalette; ///< Normal colors

    // Draw list of trophies
    trophyDisplayList_t tdl; ///< Display list data
} trophySystem_t;

//==============================================================================
// Variables
//==============================================================================

static trophySystem_t trophySystem = {}; ///< Should be one instance per swadge, should always be in memory

//==============================================================================
// Static function declarations
//==============================================================================

// NVS

/**
 * @brief Truncates a string down to a specified length
 *
 * @param to The string/buffer to copy the text into
 * @param from The source string
 * @param len Length to copy
 */
static void _truncateStr(char* to, const char* from, int len);

/**
 * @brief Saves a trophy.
 *
 * @param t Data to write
 * @param newVal Value to save
 */
static void _save(trophyDataWrapper_t* t, int newVal);

/**
 * @brief Loads data into a wrapper
 *
 * @param tw Trophy Wrapper pointer
 * @param t Trophy Data
 */
static void _load(trophyDataWrapper_t* tw, trophyData_t t);

/**
 * @brief Saves the latest unlocked trophy to NVS for later retrieval
 *
 * @param tw TrophyWrapper_t to save
 */
static void _saveLatestWin(trophyDataWrapper_t* tw);

/**
 * @brief Loads ther index of the latest win
 *
 * @param buffer Array to shove modename into
 * @return int32_t index of the trophy
 */
int32_t _loadLatestWin(char* buffer);

/**
 * @brief Saves new points value to NVS. Saves both overall value and mode-specific
 *
 * @param points Adjustment value
 */
static void _setPoints(int points);

/**
 * @brief Load value from NVS. Can select between overall total or mode specific
 *
 * @param total True returns overall score, false returns score for mode
 * @return int Current score
 */
static int _loadPoints(bool total, const char* modeName);

// Trophy handling

/**
 * @brief Checks if the final trophy should be triggering
 *
 * @return true Final trophy should be drawn
 * @return false Not the final trophy's turn
 */
static bool _isFinalTrophy(void);

/**
 * @brief Called when a trophy is won
 *
 * @param tw Trophy data to work with
 * @return true If the final trophy has been won
 * @return false If the final trophy has not been won
 */
static bool _trophyIsWon(trophyDataWrapper_t* tw);

// Checklist Helpers

/**
 * @brief Returns the number of flags set
 *
 * @param t Trophy to evaluate
 * @param maxFlags If we're getting the max number of flags for this trophy or number currently owned
 * @return int32_t number of flags captured
 */
static int32_t _GetNumFlags(trophyDataWrapper_t* t, bool maxFlags);

// Drawing

/**
 * @brief Draws the banner
 *
 * @param t Trophy to draw
 * @param yOffset y coordinate to start at
 * @param fnt Font used in draw call
 */
static void _drawAtYCoord(trophyDataWrapper_t* t, int yOffset, font_t* fnt);

/**
 * @brief Loads the palette for the grayscale conversion
 *
 */
static void _loadPalette(void);

/**
 * @brief Get the currently displayed trophy
 */
static trophyDataWrapper_t* _getCurrentDisplayTrophy(void);

/**
 * @brief Gets the height of a a trophy on the list
 *
 * @param t Trophy to display
 * @param fnt Font used
 * @return int Height
 */
static int _getListItemHeight(trophyData_t t, font_t* fnt);

/**
 * @brief Draws the trophy list item
 *
 * @param t Trophy data
 * @param yOffset Offset into the list
 * @param height Height of the individual list item
 * @param fnt Font used
 * @param image The image used, if any
 */
static void _drawTrophyListItem(trophyData_t t, int yOffset, int height, font_t* fnt, wsg_t* image);

/**
 * @brief Loads the default image to the wsg_t slot provided based on difficulty
 *
 * @param td Difficulty to get trophy for
 * @param image Place to store image
 */
static void _loadDefaultTrophyImage(trophyDifficulty_t td, wsg_t* image);

// Points

/**
 * @brief Get appropriate number of points based on teh difficulty
 *
 * @param td Difficulty to evaluate
 * @return int Score
 */
static int _genPoints(trophyDifficulty_t td);

/**
 * @brief Generates the platinum trophy
 *
 */
static void _genPlat(const char* modeName);

//==============================================================================
// Functions
//==============================================================================

// System

void trophySystemInit(trophyDataList_t* data, const char* modeName)
{
    // Reset timer
    trophySystem.animTimer = 0;

    // Copy settings
    trophySystem.data = data;

    // If no namespace is provided, auto generate
    if (strcmp(data->settings->namespaceKey, "") == 0)
    {
        char buffer[NVS_KEY_NAME_MAX_SIZE];
        _truncateStr(buffer, modeName, NVS_KEY_NAME_MAX_SIZE);
        strcpy(trophySystem.data->settings->namespaceKey, buffer);
    }

    // Generate plat
    _genPlat(modeName);

    // Calculate number of trophies and adjustment for difficulty
    trophySystem.numTrophiesScore = trophySystem.plat.difficulty; // Plat trophy
    for (int idx = 0; idx < trophySystem.data->length; idx++)
    {
        switch (trophySystem.data->list[idx].difficulty)
        {
            case TROPHY_DIFF_EXTREME:
            {
                trophySystem.numTrophiesScore += 4;
                break;
            }
            case TROPHY_DIFF_HARD:
            {
                trophySystem.numTrophiesScore += 3;
                break;
            }
            case TROPHY_DIFF_MEDIUM:
            {
                trophySystem.numTrophiesScore += 2;
                break;
            }
            default:
            {
                trophySystem.numTrophiesScore += 1;
                break;
            }
        }
    }

    // Init palette
    _loadPalette();

    // Clear queue, freeing all entries
    trophyDataWrapper_t* toFree;
    while ((toFree = pop(&trophySystem.trophyQueue)))
    {
        freeWsg(&toFree->image);
        heap_caps_free(toFree);
    }
}

// Trophies

bool trophyUpdate(trophyData_t t, int newVal, bool drawUpdate)
{
    // Load
    trophyDataWrapper_t* tw = heap_caps_calloc(1, sizeof(trophyDataWrapper_t), MALLOC_CAP_8BIT);
    _load(tw, t);
    bool final = false;

    // Check if trophy is already won and return if true
    if (tw->trophyData.type == TROPHY_TYPE_CHECKLIST)
    {
        if (tw->currentVal == tw->trophyData.maxVal)
        {
            heap_caps_free(tw);
            return false;
        }
    }
    else if (tw->trophyData.type == TROPHY_TYPE_TRIGGER)
    {
        if (tw->currentVal >= 1)
        {
            heap_caps_free(tw);
            return false;
        }
    }
    else
    {
        if (tw->currentVal >= tw->trophyData.maxVal)
        {
            heap_caps_free(tw);
            return false;
        }
    }

    // If the new value is the same as the previously saved value, return
    // - Unless it's a checklist
    if (tw->trophyData.type == TROPHY_TYPE_CHECKLIST)
    {
        // If check removed, don't draw
        drawUpdate = tw->currentVal < newVal;

        // If the newValue has exceeded currVal, Save value
        tw->currentVal = newVal;
        _save(tw, newVal);

        // If the trophy has been won, add to score
        if (tw->currentVal == tw->trophyData.maxVal)
        {
            final = _trophyIsWon(tw);
        }
    }
    else if (tw->currentVal >= newVal)
    {
        heap_caps_free(tw);
        return false;
    }
    else
    {
        // If the newValue has exceeded currVal, Save value
        tw->currentVal = newVal;
        _save(tw, newVal);

        // If the trophy has been won, add to score
        if (tw->currentVal >= tw->trophyData.maxVal)
        {
            final = _trophyIsWon(tw);
        }
    }

    // Load into draw queue if requested
    if (drawUpdate)
    {
        // Push into queue
        push(&trophySystem.trophyQueue, tw);

        // Load sprite
        if (!tw->trophyData.noImage)
        {
            if (tw->trophyData.image == NO_IMAGE_SET)
            {
                // Use default image
                _loadDefaultTrophyImage(tw->trophyData.difficulty, &tw->image);
            }
            else
            {
                // Use dev defined image
                loadWsg(tw->trophyData.image, &tw->image, true);
            }
        }

        // Set system and this index to active
        if (!trophySystem.active)
        {
            trophySystem.active    = true;
            trophySystem.animTimer = 0;
        }

        // If final trophy has been won, add to queue
        // Done here to be the last trophy pushed.
        if (final)
        {
            trophyDataWrapper_t* twf = heap_caps_calloc(1, sizeof(trophyDataWrapper_t), MALLOC_CAP_8BIT);
            _load(twf, trophySystem.plat);
            push(&trophySystem.trophyQueue, twf);
            twf->currentVal = trophySystem.platVal;
            _setPoints(_genPoints(twf->trophyData.difficulty));
            _saveLatestWin(twf);
            loadWsg(twf->trophyData.image, &twf->image, true);
        }
        // drawing an update
        return true;
    }
    // not drawing an update
    return false;
}

bool trophyUpdateMilestone(trophyData_t t, int newVal, int threshold)
{
    trophyDataWrapper_t tw = {};
    _load(&tw, t);

    // Check if completed
    if (newVal >= t.maxVal)
    {
        return trophyUpdate(t, newVal, true);
    }

    // Check if past a milestone for the first time
    // Verify Percentage isn't out of bounds
    if (threshold < 1 || threshold >= 100)
    {
        threshold = DEFAULT_MILESTONE;
    }
    int32_t prev = (tw.currentVal * 100 / t.maxVal) / threshold; // Intentionally truncates to an integer
    int32_t new  = (newVal * 100 / t.maxVal) / threshold;
    return trophyUpdate(t, newVal, prev < new);
}

int32_t trophyGetSavedValue(trophyData_t t)
{
    int32_t val;
    char buffer[NVS_KEY_NAME_MAX_SIZE];
    _truncateStr(buffer, t.title, NVS_KEY_NAME_MAX_SIZE);
    if (readNamespaceNvs32(trophySystem.data->settings->namespaceKey, buffer, &val))
    {
        return val;
    }
    return 0;
}

bool trophySetChecklistTask(trophyData_t t, int32_t flag, bool set, bool drawUpdate)
{
    // If not a checklist, abort
    if (t.type != TROPHY_TYPE_CHECKLIST)
    {
        return false;
    }

    // Load
    trophyDataWrapper_t* tw = heap_caps_calloc(1, sizeof(trophyDataWrapper_t), MALLOC_CAP_8BIT);
    _load(tw, t);

    int32_t newVal = tw->currentVal;
    setBitFlag(&newVal, flag, set);

    // Run Update
    bool ret = trophyUpdate(t, newVal, drawUpdate);

    heap_caps_free(tw);

    return ret;
}

void trophyClear(trophyData_t t)
{
    trophyDataWrapper_t tw = {};
    _load(&tw, t);
    // Reset points
    if (tw.currentVal >= t.maxVal)
    {
        _setPoints(_genPoints(t.difficulty) * -1);
    }
    _save(&tw, 0);

    // if Final trophy is set, clear it
    if (trophySystem.platVal > 0)
    {
        trophySystem.platVal = 0;
        _load(&tw, trophySystem.plat);
        _setPoints(_genPoints(tw.trophyData.difficulty) * -1);
        _save(&tw, 0);
    }
}

// Helpers

bool checkBitFlag(int32_t flags, int8_t idx)
{
    return ((flags & (1 << idx)) != 0);
}

void setBitFlag(int32_t* flags, int8_t idx, bool setTrue)
{
    if (!setTrue)
    {
        *flags &= ~(1 << idx);
    }
    else
    {
        *flags |= 1 << idx;
    }
}

int trophyGetPoints(bool total, const char* modeName)
{
    return _loadPoints(total, modeName);
}

trophyData_t trophyGetLatest()
{
    // Fetch data from NVS
    char buffer[NVS_KEY_NAME_MAX_SIZE];
    int trophyIdx   = _loadLatestWin(buffer);
    trophyData_t td = {};

    // Match mode
    for (int idx = 0; idx < modeListGetCount(); idx++)
    {
        if (allSwadgeModes[idx]->trophyData == NULL)
        {
            continue;
        }
        else if (strcmp(buffer, allSwadgeModes[idx]->trophyData->settings->namespaceKey) == 0)
        {
            td = allSwadgeModes[idx]->trophyData->list[trophyIdx];
        }
    }
    return td;
}

void trophySetSystemData(trophyDataList_t* dl, const char* modeName)
{
    trophySystem.data = dl;
    _truncateStr(trophySystem.data->settings->namespaceKey, modeName, NVS_KEY_NAME_MAX_SIZE);
    _genPlat(modeName);
    _loadPalette();
}

// Draw

bool isTrophyDrawing(void)
{
    return trophySystem.active && _getCurrentDisplayTrophy();
}

void trophyDraw(font_t* fnt, int64_t elapsedUs)
{
    // Exit immediately if not being drawn or there is no trophy
    if (!trophySystem.active || !_getCurrentDisplayTrophy())
    {
        return;
    }

    // Calculate the yOffset from the animation time
    int16_t yOffset  = 0;
    int32_t slideUs  = trophySystem.data->settings->slideDurationUs;
    int32_t staticUs = trophySystem.data->settings->staticDurationUs;

    trophySystem.animTimer += elapsedUs;
    if (trophySystem.animTimer >= 2 * slideUs + staticUs)
    {
        // Finished off screen.
        // trophySystem.animTimer is after (2 * slideUs + staticUs)
        // Offset is an easy -BANNER_HEIGHT (off screen)
        yOffset = -BANNER_HEIGHT;

        // Remove the trophy wrapper from the queue and free memory
        trophyDataWrapper_t* tw = shift(&trophySystem.trophyQueue);
        freeWsg(&tw->image);
        heap_caps_free(tw);

        // Reset animation
        trophySystem.animTimer = 0;
        // Remain active if the queue isn't empty
        trophySystem.active = (trophySystem.trophyQueue.length > 0);

        // Return before drawing because we're all done
        return;
    }
    else if (trophySystem.animTimer >= slideUs + staticUs)
    {
        // Sliding out.
        // trophySystem.animTimer is between (slideUs + staticUs) and (2 * slideUs + staticUs)
        // Offset is between 0 and -BANNER_HEIGHT
        yOffset = (-BANNER_HEIGHT * (trophySystem.animTimer - (slideUs + staticUs))) / slideUs;
    }
    else if (trophySystem.animTimer >= slideUs)
    {
        // Static on screen.
        // trophySystem.animTimer is between (slideUs) and (slideUs + staticUs)
        // Offset is an easy 0
        yOffset = 0;
    }
    else
    {
        // Sliding in.
        // trophySystem.animTimer is between 0 and slideUs
        // Offset is between -BANNER_HEIGHT and 0
        yOffset = ((BANNER_HEIGHT * trophySystem.animTimer) / slideUs) - BANNER_HEIGHT;
    }

    // yOffset is assumed to be from the top, so flip it if necessary
    if (trophySystem.data->settings->drawFromBottom)
    {
        yOffset = TFT_HEIGHT - yOffset - BANNER_HEIGHT;
    }

    // Draw the banner
    _drawAtYCoord(_getCurrentDisplayTrophy(), yOffset, fnt);
}

void trophyDrawListInit(trophyListDisplayMode_t mode)
{
    // Set mode
    trophySystem.tdl.mode = mode;

    // Colors
    trophyDrawListColors(c000, c111, c222, c444, c555, c050);

    // Load all the WSGs
    trophySystem.tdl.images = heap_caps_calloc(trophySystem.data->length, sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < trophySystem.data->length; idx++)
    {
        if (trophySystem.data->list[idx].image == NO_IMAGE_SET)
        {
            if (!trophySystem.data->list[idx].noImage)
            {
                _loadDefaultTrophyImage(trophySystem.data->list[idx].difficulty, &trophySystem.tdl.images[idx]);
            }
        }
        else
        {
            loadWsg(trophySystem.data->list[idx].image, &trophySystem.tdl.images[idx], true);
        }
    }
}

void trophyDrawListColors(paletteColor_t background, paletteColor_t panel, paletteColor_t shadowBoxes,
                          paletteColor_t dimText, paletteColor_t titleText, paletteColor_t checkmark)
{
    trophySystem.tdl.colorList[0] = background;  // Clear
    trophySystem.tdl.colorList[1] = panel;       // Text box
    trophySystem.tdl.colorList[2] = shadowBoxes; // Shadow box + check box
    trophySystem.tdl.colorList[3] = dimText;     // Numbers + desc text
    trophySystem.tdl.colorList[4] = titleText;   // Title text
    trophySystem.tdl.colorList[5] = checkmark;   // Check mark
}

void trophyDrawListDeinit()
{
    for (int idx = 0; idx < trophySystem.data->length; idx++)
    {
        freeWsg(&trophySystem.tdl.images[idx]);
    }
    heap_caps_free(trophySystem.tdl.images);
    heap_caps_free(trophySystem.tdl.heights);

    trophySystem.tdl.heights = NULL;
}

void trophyDrawList(font_t* fnt, int yOffset)
{
    trophyDisplayList_t* tdl = &trophySystem.tdl;
    // Check if wee need to calculate height
    if (tdl->heights == NULL)
    {
        tdl->heights = heap_caps_calloc(trophySystem.data->length, sizeof(int), MALLOC_CAP_8BIT);
        for (int idx = 0; idx < trophySystem.data->length; idx++)
        {
            tdl->heights[idx] = _getListItemHeight(trophySystem.data->list[idx], fnt);
        }
        tdl->platHeight = _getListItemHeight(trophySystem.plat, fnt);
    }

    // Draw
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, tdl->colorList[0]);
    int cumulativeHeight = 0;

    // Add Plat
    switch (tdl->mode)
    {
        case TROPHY_DISPLAY_LOCKED:
        {
            if (trophySystem.platVal == 0)
            {
                loadWsg(trophySystem.plat.image, &trophySystem.platImg, true);
                _drawTrophyListItem(trophySystem.plat, -yOffset, tdl->platHeight, fnt, &trophySystem.platImg);
                cumulativeHeight += tdl->platHeight;
                freeWsg(&trophySystem.platImg);
            }
            break;
        }
        case TROPHY_DISPLAY_UNLOCKED:
        {
            if (trophySystem.platVal == 1)
            {
                loadWsg(trophySystem.plat.image, &trophySystem.platImg, true);
                _drawTrophyListItem(trophySystem.plat, -yOffset, tdl->platHeight, fnt, &trophySystem.platImg);
                cumulativeHeight += tdl->platHeight;
                freeWsg(&trophySystem.platImg);
            }
            break;
        }
        default:
        {
            loadWsg(trophySystem.plat.image, &trophySystem.platImg, true);
            _drawTrophyListItem(trophySystem.plat, -yOffset, tdl->platHeight, fnt, &trophySystem.platImg);
            cumulativeHeight += tdl->platHeight;
            freeWsg(&trophySystem.platImg);
            break;
        }
    }

    // All others
    for (int idx = 0; idx < trophySystem.data->length; idx++)
    {
        switch (tdl->mode)
        {
            case TROPHY_DISPLAY_ALL:
            {
                // If hidden and not unlocked, skip
                trophyDataWrapper_t tw = {};
                _load(&tw, trophySystem.data->list[idx]);
                if (!trophySystem.data->list[idx].hidden || (trophySystem.data->list[idx].maxVal <= tw.currentVal))
                {
                    _drawTrophyListItem(trophySystem.data->list[idx], -yOffset + cumulativeHeight, tdl->heights[idx],
                                        fnt, &tdl->images[idx]);
                    cumulativeHeight += tdl->heights[idx];
                }
                break;
            }
            case TROPHY_DISPLAY_UNLOCKED:
            {
                // If hidden and not unlocked or just not unlocked, skip
                trophyDataWrapper_t tw = {};
                _load(&tw, trophySystem.data->list[idx]);
                if (trophySystem.data->list[idx].maxVal <= tw.currentVal)
                {
                    _drawTrophyListItem(trophySystem.data->list[idx], -yOffset + cumulativeHeight, tdl->heights[idx],
                                        fnt, &tdl->images[idx]);
                    cumulativeHeight += tdl->heights[idx];
                }
                break;
            }
            case TROPHY_DISPLAY_LOCKED:
            {
                // If hidden and unlocked, skip
                trophyDataWrapper_t tw = {};
                _load(&tw, trophySystem.data->list[idx]);
                if (!trophySystem.data->list[idx].hidden && trophySystem.data->list[idx].maxVal > tw.currentVal)
                {
                    _drawTrophyListItem(trophySystem.data->list[idx], -yOffset + cumulativeHeight, tdl->heights[idx],
                                        fnt, &tdl->images[idx]);
                    cumulativeHeight += tdl->heights[idx];
                }
                break;
            }
            default:
            {
                _drawTrophyListItem(trophySystem.data->list[idx], -yOffset + cumulativeHeight, tdl->heights[idx], fnt,
                                    &tdl->images[idx]);
                cumulativeHeight += tdl->heights[idx];
            }
            break;
        }
    }
}

//==============================================================================
// Static functions
//==============================================================================

// NVS

static void _truncateStr(char* to, const char* from, int len)
{
    strncpy(to, from, len);
    to[len - 1] = '\0';
}

static void _save(trophyDataWrapper_t* t, int newVal)
{
    char buffer[NVS_KEY_NAME_MAX_SIZE];
    _truncateStr(buffer, t->trophyData.title, NVS_KEY_NAME_MAX_SIZE);
    writeNamespaceNvs32(trophySystem.data->settings->namespaceKey, buffer, newVal);
}

static void _load(trophyDataWrapper_t* tw, trophyData_t t)
{
    // Copy t into tw
    strcpy(tw->trophyData.title, t.title);
    strcpy(tw->trophyData.description, t.description);
    tw->trophyData.image      = t.image;
    tw->trophyData.type       = t.type;
    tw->trophyData.difficulty = t.difficulty;
    tw->trophyData.maxVal     = t.maxVal;
    tw->trophyData.hidden     = t.hidden;
    tw->trophyData.noImage    = t.noImage;

    // Pull Current Val from disk
    int32_t val;
    char buffer[NVS_KEY_NAME_MAX_SIZE];
    _truncateStr(buffer, t.title, NVS_KEY_NAME_MAX_SIZE);
    if (readNamespaceNvs32(trophySystem.data->settings->namespaceKey, buffer, &val))
    {
        tw->currentVal = val;
    }
    else
    {
        tw->currentVal = 0;
        writeNamespaceNvs32(trophySystem.data->settings->namespaceKey, buffer, 0);
    }
}

static void _saveLatestWin(trophyDataWrapper_t* tw)
{
    for (int idx = 0; idx < trophySystem.data->length; idx++)
    {
        if (strcmp(tw->trophyData.title, trophySystem.data->list[idx].title) == 0)
        {
            writeNamespaceNvs32(NVSstrings[0], NVSstrings[2], idx);
            writeNamespaceNvsBlob(NVSstrings[0], NVSstrings[3], trophySystem.data->settings->namespaceKey,
                                  NVS_KEY_NAME_MAX_SIZE);
            return;
        }
    }
}

int32_t _loadLatestWin(char* buffer)
{
    int32_t idx;
    size_t blobLen;
    readNamespaceNvs32(NVSstrings[0], NVSstrings[2], &idx);
    readNamespaceNvsBlob(NVSstrings[0], NVSstrings[3], NULL, &blobLen);
    readNamespaceNvsBlob(NVSstrings[0], NVSstrings[3], buffer, &blobLen);
    return idx;
}

static void _setPoints(int points)
{
    int32_t prevVal;
    // Mode specific
    if (!readNamespaceNvs32(trophySystem.data->settings->namespaceKey, NVSstrings[1], &prevVal))
    {
        prevVal = 0;
    }
    prevVal += points;
    writeNamespaceNvs32(trophySystem.data->settings->namespaceKey, NVSstrings[1], prevVal);

    // Overall
    if (!readNamespaceNvs32(NVSstrings[0], NVSstrings[1], &prevVal))
    {
        prevVal = 0;
    }
    prevVal += points;
    writeNamespaceNvs32(NVSstrings[0], NVSstrings[1], prevVal);
}

static int _loadPoints(bool total, const char* modeName)
{
    int32_t val;
    if (!total)
    {
        if (modeName == NULL)
        {
            if (!readNamespaceNvs32(trophySystem.data->settings->namespaceKey, NVSstrings[1], &val))
            {
                val = 0;
            }
        }
        else
        {
            char buffer[NVS_KEY_NAME_MAX_SIZE];
            _truncateStr(buffer, modeName, NVS_KEY_NAME_MAX_SIZE);
            if (!readNamespaceNvs32(buffer, NVSstrings[1], &val))
            {
                val = 0;
            }
        }
    }
    else
    {
        if (!readNamespaceNvs32(NVSstrings[0], NVSstrings[1], &val))
        {
            val = 0;
        }
    }
    return val;
}

// Trophy

static bool _isFinalTrophy(void)
{
    bool final = true;
    if (trophySystem.platVal > 0)
    {
        // Trophy already got
        return false;
    }
    for (int idx = 0; idx < trophySystem.data->length; idx++)
    {
        trophyDataWrapper_t tw = {};
        _load(&tw, trophySystem.data->list[idx]);
        if (tw.currentVal < tw.trophyData.maxVal)
        {
            final = false;
            break;
        }
    }
    return final;
}

static bool _trophyIsWon(trophyDataWrapper_t* tw)
{
    _setPoints(_genPoints(tw->trophyData.difficulty));
    _saveLatestWin(tw);
    if (_isFinalTrophy())
    {
        trophySystem.platVal    = 1;
        trophyDataWrapper_t twf = {};
        _load(&twf, trophySystem.plat);
        _save(&twf, 1);
        return true;
    }
    return false;
}

// Checklist Helpers

static int32_t _GetNumFlags(trophyDataWrapper_t* t, bool maxFlags)
{
    int total = 0;
    if (maxFlags)
    {
        for (int idx = 0; idx < 32; idx++)
        {
            total += checkBitFlag(t->trophyData.maxVal, idx) ? 1 : 0;
        }
    }
    else
    {
        for (int idx = 0; idx < 32; idx++)
        {
            total += checkBitFlag(t->currentVal, idx) ? 1 : 0;
        }
    }
    return total;
}

// Drawing

static void _drawAtYCoord(trophyDataWrapper_t* t, int yOffset, font_t* fnt)
{
    // Draw box (Gray box, black border)
    fillDisplayArea(0, yOffset, TFT_WIDTH, yOffset + BANNER_HEIGHT, c111);
    drawRect(0, yOffset, TFT_WIDTH, yOffset + BANNER_HEIGHT, c000);

    int endX = TFT_WIDTH - SCREEN_CORNER_CLEARANCE;
    // Draw numbers if required
    if (t->trophyData.type != TROPHY_TYPE_TRIGGER && t->currentVal < t->trophyData.maxVal)
    {
        char buffer[NUMBER_TEXT_BUFFER];
        if (t->trophyData.type == TROPHY_TYPE_CHECKLIST)
        {
            snprintf(buffer, sizeof(buffer) - 1, "%" PRId32 "/%" PRId32, _GetNumFlags(t, false), _GetNumFlags(t, true));
        }
        else
        {
            snprintf(buffer, sizeof(buffer) - 1, "%" PRId32 "/%" PRId32, t->currentVal, t->trophyData.maxVal);
        }

        int16_t xOff = TFT_WIDTH - (textWidth(fnt, buffer) + SCREEN_CORNER_CLEARANCE + 8);
        endX         = xOff - 8;
        drawText(fnt, c444, buffer, xOff, yOffset + 4);
    }

    // Calculate clearance
    int xOffset;
    int16_t startX = SCREEN_CORNER_CLEARANCE;
    int16_t startY = yOffset + ((BANNER_HEIGHT - BANNER_MAX_ICON_DIM) >> 1);
    if (t->trophyData.noImage)
    {
        // Draw text at start of buffer area
        xOffset = SCREEN_CORNER_CLEARANCE;
    }
    else
    {
        // Set xOffset to be 2* H_BUFFER + ICON_WIDTH
        xOffset = BANNER_MAX_ICON_DIM + SCREEN_CORNER_CLEARANCE + IMAGE_BUFFER;

        // Draw shadowbox
        drawRectFilled(startX, startY, startX + BANNER_MAX_ICON_DIM, startY + BANNER_MAX_ICON_DIM, c222);

        // Draw WSG
        wsgPalette_t* wp = &trophySystem.grayPalette;
        if (t->currentVal >= t->trophyData.maxVal)
        {
            wp = &trophySystem.normalPalette;
        }
        drawWsgPaletteSimple(&t->image, startX + ((BANNER_MAX_ICON_DIM - t->image.w) >> 1),
                             startY + ((BANNER_MAX_ICON_DIM - t->image.h) >> 1), wp);
    }

    // Draw text, starting after image if present
    startX = xOffset;
    startY = yOffset + 4;
    if (drawTextWordWrap(fnt, c555, t->trophyData.title, &startX, &startY, endX, yOffset + 18) != NULL) // Title
    {
        // Draw a gray box and ellipses
        fillDisplayArea(endX - (textWidth(fnt, "...") + 4), yOffset + 4, endX, yOffset + 16, c111);
        drawText(fnt, c555, "...", endX - textWidth(fnt, "..."), yOffset + 4);
    }
    startX = xOffset;
    startY = yOffset + 20;
    if (drawTextWordWrap(fnt, c444, t->trophyData.description, &startX, &startY, TFT_WIDTH - SCREEN_CORNER_CLEARANCE,
                         yOffset + BANNER_HEIGHT)
        != NULL) // Description
    {
        fillDisplayArea(endX - textWidth(fnt, "..."), yOffset + BANNER_HEIGHT - fnt->height, endX,
                        yOffset + BANNER_HEIGHT, c111);
        drawText(fnt, c555, "...", endX - (textWidth(fnt, "...") + 4), yOffset + BANNER_HEIGHT - (fnt->height + 7));
    }

    // Draw check box
    fillDisplayArea(TFT_WIDTH - 16, yOffset + 20, TFT_WIDTH - 11, yOffset + 25, c222);

    // Draw check if done
    if (t->currentVal >= t->trophyData.maxVal)
    {
        drawLine(TFT_WIDTH - 14, yOffset + 23, TFT_WIDTH - 8, yOffset + 13, c050, 0);
        drawLine(TFT_WIDTH - 14, yOffset + 23, TFT_WIDTH - 19, yOffset + 20, c050, 0);
    }
}

static void _loadPalette(void)
{
    wsgPaletteReset(&trophySystem.normalPalette);
    for (int idx = 0; idx < 217; idx++)
    {
        if (idx == cTransparent) // Transparent
        {
            wsgPaletteSet(&trophySystem.grayPalette, idx, cTransparent);
        }
        else
        {
            switch (idx)
            {
                case c000:
                case c001:
                case c002:
                case c003:
                case c010:
                case c011:
                case c012:
                case c013:
                case c020:
                case c021:
                case c022:
                case c023:
                case c030:
                case c031:
                case c032:
                case c033:
                case c100:
                case c101:
                case c102:
                case c103:
                case c110:
                case c111:
                case c112:
                case c113:
                case c120:
                case c121:
                case c122:
                case c123:
                case c130:
                case c131:
                case c132:
                case c133:
                case c200:
                case c201:
                case c202:
                case c203:
                case c210:
                case c211:
                case c212:
                case c213:
                case c220:
                case c221:
                case c222:
                case c223:
                case c230:
                case c231:
                case c232:
                case c233:
                case c300:
                case c301:
                case c302:
                case c303:
                case c310:
                case c311:
                case c312:
                case c313:
                case c320:
                case c321:
                case c322:
                case c323:
                case c330:
                case c331:
                case c332:
                case c333:
                {
                    wsgPaletteSet(&trophySystem.grayPalette, idx, c333);
                    break;
                }
                default:
                {
                    wsgPaletteSet(&trophySystem.grayPalette, idx, c444);
                    break;
                }
            }
        }
    }
}

static trophyDataWrapper_t* _getCurrentDisplayTrophy(void)
{
    if (trophySystem.trophyQueue.first)
    {
        return trophySystem.trophyQueue.first->val;
    }
    return NULL;
}

static int _getListItemHeight(trophyData_t t, font_t* fnt)
{
    // Load data
    trophyDataWrapper_t tw = {};
    _load(&tw, t);

    // Calculate space
    int boxHeight  = 0;
    int titleStart = SCREEN_CORNER_CLEARANCE;
    int titleEnd   = TFT_WIDTH - SCREEN_CORNER_CLEARANCE;
    // Get numbers space
    if (t.type != TROPHY_TYPE_TRIGGER)
    {
        char buffer[NUMBER_TEXT_BUFFER];
        if (t.type != TROPHY_TYPE_CHECKLIST)
        {
            snprintf(buffer, sizeof(buffer) - 1, "%" PRId32 "/%" PRId32, tw.currentVal, tw.trophyData.maxVal);
        }
        else
        {
            snprintf(buffer, sizeof(buffer) - 1, "%" PRId32 "/%" PRId32, _GetNumFlags(&tw, false),
                     _GetNumFlags(&tw, true));
        }
        titleEnd -= textWidth(fnt, buffer) + 16;
    }

    // Get image space
    if (!tw.trophyData.noImage)
    {
        titleStart += BANNER_MAX_ICON_DIM + IMAGE_BUFFER;
    }

    // Lay out text
    boxHeight = textWordWrapHeight(fnt, t.title, titleEnd - titleStart, 100) + IMAGE_BUFFER;
    if (BANNER_MAX_ICON_DIM + IMAGE_BUFFER > boxHeight && !tw.trophyData.noImage)
    {
        boxHeight = BANNER_MAX_ICON_DIM + IMAGE_BUFFER;
    }
    boxHeight += textWordWrapHeight(fnt, t.description, TFT_WIDTH - (SCREEN_CORNER_CLEARANCE * 2), 300) + IMAGE_BUFFER;

    // Close out
    return boxHeight;
}

static void _drawTrophyListItem(trophyData_t t, int yOffset, int height, font_t* fnt, wsg_t* image)
{
    // Load relevant data
    trophyDisplayList_t* tdl = &trophySystem.tdl;
    trophyDataWrapper_t tw   = {};
    _load(&tw, t);

    // Draw
    fillDisplayArea(1, yOffset, TFT_WIDTH - 2, yOffset + height, tdl->colorList[1]);
    drawRect(0, yOffset, TFT_WIDTH, yOffset + height, tdl->colorList[0]);

    // Calculate space
    int titleStart = SCREEN_CORNER_CLEARANCE;
    int titleEnd   = TFT_WIDTH - SCREEN_CORNER_CLEARANCE;

    // Draw Numbers
    if (t.type != TROPHY_TYPE_TRIGGER)
    {
        char buffer[NUMBER_TEXT_BUFFER];
        if (t.type != TROPHY_TYPE_CHECKLIST)
        {
            snprintf(buffer, sizeof(buffer) - 1, "%" PRId32 "/%" PRId32, tw.currentVal, tw.trophyData.maxVal);
        }
        else
        {
            snprintf(buffer, sizeof(buffer) - 1, "%" PRId32 "/%" PRId32, _GetNumFlags(&tw, false),
                     _GetNumFlags(&tw, true));
        }
        titleEnd -= textWidth(fnt, buffer) + 16;
        drawText(fnt, (tw.currentVal >= t.maxVal) ? tdl->colorList[5] : tdl->colorList[3], buffer, titleEnd + 8,
                 yOffset + 4);
    }

    // Draw image
    int16_t startX = SCREEN_CORNER_CLEARANCE;
    int16_t startY = yOffset + ((BANNER_HEIGHT - BANNER_MAX_ICON_DIM) >> 1);
    if (!tw.trophyData.noImage)
    {
        titleStart += BANNER_MAX_ICON_DIM + IMAGE_BUFFER;
        // Draw shadowbox
        drawRectFilled(startX, startY, startX + BANNER_MAX_ICON_DIM, startY + BANNER_MAX_ICON_DIM, tdl->colorList[2]);

        // Draw WSG
        wsgPalette_t* wp = &trophySystem.grayPalette;
        if (tw.currentVal >= tw.trophyData.maxVal)
        {
            wp = &trophySystem.normalPalette;
        }
        drawWsgPaletteSimple(image, startX + ((BANNER_MAX_ICON_DIM - image->w) >> 1),
                             startY + ((BANNER_MAX_ICON_DIM - image->h) >> 1), wp);
    }

    // Draw text, starting after image if present
    startX = titleStart;
    startY = yOffset + 4;
    drawTextWordWrap(fnt, tdl->colorList[4], tw.trophyData.title, &startX, &startY, titleEnd, yOffset + height);
    startX = SCREEN_CORNER_CLEARANCE;
    startY = yOffset + 12;
    startY += tw.trophyData.noImage ? fnt->height : BANNER_MAX_ICON_DIM;
    drawTextWordWrap(fnt, tdl->colorList[3], tw.trophyData.description, &startX, &startY,
                     TFT_WIDTH - SCREEN_CORNER_CLEARANCE, yOffset + height);

    // Draw check box
    fillDisplayArea(TFT_WIDTH - 16, yOffset + (height >> 1) - 2, TFT_WIDTH - 11, yOffset + (height >> 1) + 3,
                    tdl->colorList[2]);

    // Draw check if done
    if (tw.currentVal >= tw.trophyData.maxVal)
    {
        drawLine(TFT_WIDTH - 14, yOffset + (height >> 1), TFT_WIDTH - 8, yOffset + (height >> 1) - 10,
                 tdl->colorList[5], 0);
        drawLine(TFT_WIDTH - 14, yOffset + (height >> 1), TFT_WIDTH - 19, yOffset + (height >> 1) - 3,
                 tdl->colorList[5], 0);
    }
}

static void _loadDefaultTrophyImage(trophyDifficulty_t td, wsg_t* image)
{
    switch (td)
    {
        case TROPHY_DIFF_EXTREME:
        {
            loadWsg(WINGED_TROPHY_WSG, image, true);
            break;
        }
        case TROPHY_DIFF_HARD:
        {
            loadWsg(GOLD_TROPHY_WSG, image, true);
            break;
        }
        case TROPHY_DIFF_MEDIUM:
        {
            loadWsg(SILVER_TROPHY_WSG, image, true);
            break;
        }
        case TROPHY_DIFF_EASY:
        default:
        {
            loadWsg(BRONZE_TROPHY_WSG, image, true);
            break;
        }
    }
}

// Points

static int _genPoints(trophyDifficulty_t td)
{
    int scorePer = 1000 / trophySystem.numTrophiesScore;
    if (td == TROPHY_DIFF_FINAL)
    {
        scorePer = 1000 - (scorePer * trophySystem.numTrophiesScore);
        return scorePer;
    }
    else
    {
        return scorePer *= td;
    }
}

static void _genPlat(const char* modeName)
{
    char buffer[TROPHY_MAX_TITLE_LEN];
    snprintf(buffer, sizeof(buffer) - 1, "%s%s%s", platStrings[0], modeName, platStrings[1]);
    strcpy(trophySystem.plat.title, buffer);
    char buffer2[TROPHY_MAX_DESC_LEN];
    snprintf(buffer2, sizeof(buffer2) - 1, "%s%s", platStrings[2], modeName);
    strcpy(trophySystem.plat.description, buffer2);
    trophySystem.plat.image      = MMX_TROPHY_WSG;
    trophySystem.plat.type       = TROPHY_TYPE_TRIGGER;
    trophySystem.plat.difficulty = TROPHY_DIFF_FINAL;
    trophySystem.plat.maxVal     = 1;

    // get current val
    trophyDataWrapper_t tw = {};
    _load(&tw, trophySystem.plat);
    trophySystem.platVal = tw.currentVal;
}