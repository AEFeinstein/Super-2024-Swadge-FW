/**
 * @file trophy.c
 * @author Jeremy.Stintzcum@gmail.com
 * @brief Trophies for swadge modes
 * @version 0.1
 * @date 2025-01-13
 *
 * @copyright Copyright (c) 2025
 *
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
#define MAX_NVS_KEY_LEN   16
#define DEFAULT_MILESTONE 25

// Visuals
#define TROPHY_BANNER_HEIGHT           48
#define TROPHY_BANNER_MAX_ICON_DIM     36
#define TROPHY_SCREEN_CORNER_CLEARANCE 19
#define TROPHY_IMAGE_BUFFER            8

//==============================================================================
// Consts
//==============================================================================

static const char* const systemPointsNVS[] = {"trophy", "points"};

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    trophyData_t trophyData; //< Individual trophy data
    int currentVal;          //< Saved value of the trophy
    wsg_t image;             //< Where the image is loaded
    bool active;             //< If this slot is loaded and ready to animate
} trophyDataWrapper_t;

// System variables for display,
typedef struct
{
    // Settings
    trophyDataList_t* data; //< The settings of how the trophies behave

    // Data
    list_t trophyQueue;   //< List of trophy updates to display. Holds type \ref trophyDataWrapper_t*
    int numTrophiesScore; //< Num of trophies adjusted for difficulty

    // Drawing
    bool active;                //< If the mode should be drawing a banner
    int32_t animTimer;          //< Timer used for sliding in and out
    wsgPalette_t grayPalette;   //< Grayscale palette for locked trophies
    wsgPalette_t normalPalette; //< Normal colors
} trophySystem_t;

//==============================================================================
// Variables
//==============================================================================

static trophySystem_t trophySystem = {}; //< Should be one instance per swadge, should always be in memory

//==============================================================================
// Static function declarations
//==============================================================================

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
 * @brief Draws the banner
 *
 * @param t Trophy to draw
 * @param yOffset y coordinate to start at
 * @param fnt Font used in draw call
 */
static void _drawAtYCoord(trophyDataWrapper_t* t, int yOffset, font_t* fnt);

/**
 * @brief Truncates a string down to a specified length
 *
 * @param to The string/buffer to copy the text into
 * @param from The source string
 * @param len Length to copy
 */
static void _truncateStr(char* to, const char* from, int len);

/**
 * @brief Loads the palette for the grayscale conversion
 *
 */
static void _loadPalette(void);

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
static int _loadPoints(bool total, char* modeName);

static int _genPoints(trophyDifficulty_t td);

/**
 * @brief Get the currently displayed trophy
 */
static trophyDataWrapper_t* _getCurrentDisplayTrophy(void);

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

    // Calculate number of trophies and adjustment for difficulty
    for (int idx = 0; idx < trophySystem.data->length; idx++)
    {
        switch (trophySystem.data->list[idx].difficulty)
        {
            case TROPHY_DIFF_MEDIUM:
            {
                trophySystem.numTrophiesScore += 2;
                break;
            }
            case TROPHY_DIFF_HARD:
            {
                trophySystem.numTrophiesScore += 3;
                break;
            }
            case TROPHY_DIFF_EXTREME:
            {
                trophySystem.numTrophiesScore += 4;
                break;
            }
            default:
            {
                trophySystem.numTrophiesScore += 1;
                break;
            }
        }
    }

    // If no namespace is provided, auto generate
    if (strcmp(data->settings->namespaceKey, "") == 0)
    {
        char buffer[MAX_NVS_KEY_LEN];
        _truncateStr(buffer, modeName, MAX_NVS_KEY_LEN);
        strcpy(trophySystem.data->settings->namespaceKey, buffer);
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

    // Draw defaults unless re-called by dev
    // FIXME: Add default values
    // trophyDrawListInit();
}

// Trophies

void trophyUpdate(trophyData_t t, int newVal, bool drawUpdate)
{
    // Load
    trophyDataWrapper_t* tw = heap_caps_calloc(1, sizeof(trophyDataWrapper_t), MALLOC_CAP_8BIT);
    _load(tw, t);

    // Check if trophy is already won and return if true
    if (tw->trophyData.type == TROPHY_TYPE_CHECKLIST)
    {
        if (tw->currentVal == tw->trophyData.maxVal)
        {
            heap_caps_free(tw);
            return;
        }
    }
    else if (tw->trophyData.type == TROPHY_TYPE_TRIGGER)
    {
        if (tw->currentVal >= 1)
        {
            heap_caps_free(tw);
            return;
        }
    }
    else
    {
        if (tw->currentVal >= tw->trophyData.maxVal)
        {
            heap_caps_free(tw);
            return;
        }
    }

    // If the new value is the same as the previously saved value, return
    if (tw->currentVal >= newVal)
    {
        heap_caps_free(tw);
        return;
    }
    else
    {
        // If the newValue has exceeded maxVal, Save value
        tw->currentVal = newVal;
        _save(tw, newVal);

        // Update score
        if (tw->currentVal >= tw->trophyData.maxVal)
        {
            _setPoints(_genPoints(t.difficulty));
        }
    }

    // Load into draw queue if requested
    if (drawUpdate)
    {
        // Push into queue
        push(&trophySystem.trophyQueue, tw);

        // Load sprite
        loadWsg(tw->trophyData.imageString, &tw->image, true);

        // Set system and this index to active
        if (!trophySystem.active)
        {
            trophySystem.active    = true;
            trophySystem.animTimer = 0;
        }
    }
}

void trophyUpdateMilestone(trophyData_t t, int newVal, int threshold)
{
    trophyDataWrapper_t* tw = heap_caps_calloc(1, sizeof(trophyDataWrapper_t), MALLOC_CAP_8BIT);
    _load(tw, t);

    // Check if completed
    if (newVal >= t.maxVal)
    {
        trophyUpdate(t, newVal, true);
        heap_caps_free(tw);
        return;
    }

    // Check if past a milestone for the first time
    // Verify Percentage isn't out of bounds
    if (threshold < 1 || threshold >= 100)
    {
        threshold = DEFAULT_MILESTONE;
    }
    int32_t prev = (tw->currentVal * 100 / t.maxVal) / threshold; // Intentionally truncates to an integer
    int32_t new  = (newVal * 100 / t.maxVal) / threshold;
    trophyUpdate(t, newVal, prev < new);
    heap_caps_free(tw);
}

int32_t trophyGetSavedValue(trophyData_t t)
{
    int32_t val;
    char buffer[MAX_NVS_KEY_LEN];
    _truncateStr(buffer, t.title, MAX_NVS_KEY_LEN);
    if (readNamespaceNvs32(trophySystem.data->settings->namespaceKey, buffer, &val))
    {
        return val;
    }
    return 0;
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
    
    // Finalize
    _save(&tw, 0);
}

// Helpers

bool checkBitFlag(int32_t flags, int8_t idx)
{
    return ((flags & (1 << idx)) != 0);
}

int trophyGetPoints(bool total, char* modeName)
{
    return _loadPoints(total, modeName);
}

int trophyGetNumTrophies(char* modeName)
{
    // Gets the number of trophies associated with a mode, or current mode if NULL
    return 0;
}

void trophyGetTrophyList(char* modeName, trophyData_t* tList, int* tLen, int offset)
{
    // If modeName is NULL, get all trophies. Order by game, but do no sorting.
    // Populate tList starting from offset up to tLen
}

// Draw

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
        // Offset is an easy -TROPHY_BANNER_HEIGHT (off screen)
        yOffset = -TROPHY_BANNER_HEIGHT;

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
        // Offset is between 0 and -TROPHY_BANNER_HEIGHT
        yOffset = (-TROPHY_BANNER_HEIGHT * (trophySystem.animTimer - (slideUs + staticUs))) / slideUs;
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
        // Offset is between -TROPHY_BANNER_HEIGHT and 0
        yOffset = ((TROPHY_BANNER_HEIGHT * trophySystem.animTimer) / slideUs) - TROPHY_BANNER_HEIGHT;
    }

    // yOffset is assumed to be from the top, so flip it if necessary
    if (trophySystem.data->settings->drawFromBottom)
    {
        yOffset = TFT_HEIGHT - yOffset - TROPHY_BANNER_HEIGHT;
    }

    // Draw the banner
    _drawAtYCoord(_getCurrentDisplayTrophy(), yOffset, fnt);
}

void trophyDrawListInit(void)
{
    // Sets the colors for the trophy screen
}

void trophyDrawList(char* modeName, int idx)
{
    // Draws then entire trophy list starting at idx
    // Draws check mark nex tto completed trophies
    // Draws special stuff if all trophies for mode are completed
}

//==============================================================================
// Static functions
//==============================================================================

static void _save(trophyDataWrapper_t* t, int newVal)
{
    char buffer[MAX_NVS_KEY_LEN];
    _truncateStr(buffer, t->trophyData.title, MAX_NVS_KEY_LEN);
    writeNamespaceNvs32(trophySystem.data->settings->namespaceKey, buffer, newVal);
}

static void _load(trophyDataWrapper_t* tw, trophyData_t t)
{
    // Copy t into tw
    strcpy(tw->trophyData.title, t.title);
    strcpy(tw->trophyData.description, t.description);
    strcpy(tw->trophyData.imageString, t.imageString);
    tw->trophyData.type       = t.type;
    tw->trophyData.difficulty = t.difficulty;
    tw->trophyData.maxVal     = t.maxVal;

    // Pull Current Val from disk
    int32_t val;
    char buffer[MAX_NVS_KEY_LEN];
    _truncateStr(buffer, t.title, MAX_NVS_KEY_LEN);
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

static void _drawAtYCoord(trophyDataWrapper_t* t, int yOffset, font_t* fnt)
{
    // Draw box (Gray box, black border)
    fillDisplayArea(0, yOffset, TFT_WIDTH, yOffset + TROPHY_BANNER_HEIGHT, c111);
    drawRect(0, yOffset, TFT_WIDTH, yOffset + TROPHY_BANNER_HEIGHT, c000);

    int endX = TFT_WIDTH - TROPHY_SCREEN_CORNER_CLEARANCE;
    // Draw numbers if required
    if ((t->trophyData.type == TROPHY_TYPE_ADDITIVE || t->trophyData.type == TROPHY_TYPE_PROGRESS)
        && t->currentVal < t->trophyData.maxVal)
    {
        char buffer[32];
        snprintf(buffer, sizeof(buffer) - 1, "%d/%" PRId32, t->currentVal, t->trophyData.maxVal);
        int16_t xOff = TFT_WIDTH - (textWidth(fnt, buffer) + TROPHY_SCREEN_CORNER_CLEARANCE + 8);
        endX         = xOff - 8;
        drawText(fnt, c444, buffer, xOff, yOffset + 4);
    }

    // Calculate clearance
    int xOffset;
    int16_t startX = TROPHY_SCREEN_CORNER_CLEARANCE;
    int16_t startY = yOffset + ((TROPHY_BANNER_HEIGHT - TROPHY_BANNER_MAX_ICON_DIM) >> 1);
    if (strcmp(t->trophyData.imageString, "") == 0)
    {
        // Draw text at start of buffer area
        xOffset = TROPHY_SCREEN_CORNER_CLEARANCE;
    }
    else
    {
        // Set xOffset to be 2* H_BUFFER + ICON_WIDTH
        xOffset = TROPHY_BANNER_MAX_ICON_DIM + TROPHY_SCREEN_CORNER_CLEARANCE + TROPHY_IMAGE_BUFFER;

        // Draw shadowbox
        drawRectFilled(startX, startY, startX + TROPHY_BANNER_MAX_ICON_DIM, startY + TROPHY_BANNER_MAX_ICON_DIM, c222);

        // Draw WSG
        wsgPalette_t* wp = &trophySystem.grayPalette;
        if (t->currentVal >= t->trophyData.maxVal)
        {
            wp = &trophySystem.normalPalette;
        }
        drawWsgPaletteSimple(&t->image, startX + ((TROPHY_BANNER_MAX_ICON_DIM - t->image.w) >> 1),
                             startY + ((TROPHY_BANNER_MAX_ICON_DIM - t->image.h) >> 1), wp);
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
    if (drawTextWordWrap(fnt, c444, t->trophyData.description, &startX, &startY,
                         TFT_WIDTH - TROPHY_SCREEN_CORNER_CLEARANCE, yOffset + TROPHY_BANNER_HEIGHT)
        != NULL) // Description
    {
        fillDisplayArea(endX - textWidth(fnt, "..."), yOffset + TROPHY_BANNER_HEIGHT - fnt->height, endX,
                        yOffset + TROPHY_BANNER_HEIGHT, c111);
        drawText(fnt, c555, "...", endX - (textWidth(fnt, "...") + 4),
                 yOffset + TROPHY_BANNER_HEIGHT - (fnt->height + 7));
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

static void _truncateStr(char* to, const char* from, int len)
{
    strncpy(to, from, len);
    to[len - 1] = '\0';
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

static void _setPoints(int points)
{
    int prevVal;
    // Mode specific
    if (!readNamespaceNvs32(trophySystem.data->settings->namespaceKey, systemPointsNVS[1], &prevVal))
    {
        prevVal = 0;
    }
    prevVal += points;
    writeNamespaceNvs32(trophySystem.data->settings->namespaceKey, systemPointsNVS[1], prevVal);

    // Overall
    if (!readNamespaceNvs32(systemPointsNVS[0], systemPointsNVS[1], &prevVal))
    {
        prevVal = 0;
    }
    prevVal += points;
    writeNamespaceNvs32(systemPointsNVS[0], systemPointsNVS[1], prevVal);
}

static int _loadPoints(bool total, char* modeName)
{
    int val;
    if (!total)
    {
        if (modeName == NULL)
        {
            if (!readNamespaceNvs32(trophySystem.data->settings->namespaceKey, systemPointsNVS[1], &val))
            {
                val = 0;
            }
        }
        else
        {
            if (!readNamespaceNvs32(modeName, systemPointsNVS[1], &val))
            {
                val = 0;
            }
        }
    }
    else
    {
        if (!readNamespaceNvs32(systemPointsNVS[0], systemPointsNVS[1], &val))
        {
            val = 0;
        }
    }
    return val;
}

static int _genPoints(trophyDifficulty_t td)
{
    int scorePer = 1000 / trophySystem.numTrophiesScore; 

    switch (td)
    {
        case TROPHY_DIFF_EXTREME:
        {
            return scorePer * 4;
        }
        case TROPHY_DIFF_HARD:
        {
            return scorePer * 3;
        }
        case TROPHY_DIFF_MEDIUM:
        {
            return scorePer * 2;
        }
        default:
        {
            return scorePer;
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
