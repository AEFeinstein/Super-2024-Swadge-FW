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

// Hardware
#include "hdw-nvs.h"
#include "hdw-tft.h"

// Drawing
#include "fs_font.h"
#include "fs_wsg.h"
#include "fill.h"
#include "shapes.h"

//==============================================================================
// Defines
//==============================================================================

// Standard defines
#define TENTH_SECOND    10000
#define STATIC_POSITION -1
#define MAX_NVS_KEY_LEN 16

// Visuals
#define TROPHY_MAX_BANNERS             5
#define TROPHY_BANNER_HEIGHT           48
#define TROPHY_BANNER_MAX_ICON_DIM     36
#define TROPHY_SCREEN_CORNER_CLEARANCE 19
#define TROPHY_IMAGE_BUFFER            8

//==============================================================================
// Consts
//==============================================================================

static const char* const systemPointsNVS[] = {"trophy", "TotalPoints"};

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
    trophySettings_t* settings; //< The settings of how the trophies behave

    // Data
    trophyDataWrapper_t trophyQueue[TROPHY_MAX_BANNERS]; //< List of trophy updates to display
    int8_t idx;                                          //< Index of next queue slot to fill
    int8_t currIdx;                                      //< The current display index

    // Drawing
    bool active;                //< If the mode should be drawing a banner
    bool beingDrawn;            //< If banner is currently being drawn statically
    int32_t drawTimer;          //< Accumulates until more than Max duration
    wsgPalette_t grayPalette;   //< Grayscale palette for locked trophies
    wsgPalette_t normalPalette; //< Normal colors

    // Animation
    bool sliding;          //< If currently sliding
    int32_t slideTimer;    //< Accumulates until more than Max duration
    int16_t animationTick; //< What tick of the animation is active based on timer
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
static void _save(trophyDataWrapper_t t, int newVal);

/**
 * @brief Loads data into a wrapper
 *
 * @param tw Trophy Wrapper pointer
 * @param t Trophy Data
 */
static void _load(trophyDataWrapper_t* tw, trophyData_t t);

/**
 * @brief Copies data from a trophyData to a wrapper
 *
 * @param tw Wrapper object to copy to
 * @param t Trophy data to copy
 */
static void _copyTrophy(trophyDataWrapper_t* tw, trophyData_t t);

/**
 * @brief Grabs all trophy data from NVS
 *
 * @param modeName Name of the mode to grab data for. NULL is all modes
 * @param tList Outfile of list of Trophy data from NVS. Will eat RAM
 * @param tLen Number of trophies to grab
 * @param offset Where to start in list
 */
static void _getTrophyData(char* modeName, trophyData_t* tList, int tLen, int offset);

/**
 * @brief Draws the banner
 *
 * @param t Data to feed
 * @param frame Which frame is being drawn. STATIC_POSITION is fully visible, other values change the offset.
 * @param fnt Font used in draw call
 */
static void _draw(trophyDataWrapper_t t, int frame, font_t* fnt);

/**
 * @brief Sub function of _draw() so code can be reused in list draw command
 *
 * @param t Trophy to draw
 * @param yOffset y coordinate to start at
 * @param fnt Font used in draw call
 */
static void _drawAtYCoord(trophyDataWrapper_t t, int yOffset, font_t* fnt);

/**
 * @brief Truncates a string down to a specified length
 *
 * @param to The string/buffer to copy the text into
 * @param from The source string
 * @param len Length to copy
 */
static void _truncateStr(char* to, char* from, int len);

/**
 * @brief Automatically resets number to 0 after it gets up to max val
 *
 * @param val current value
 * @param max Number to reset at, non-inclusive
 * @return int number
 */
static int _incWithOverflow(int val, int max);

/**
 * @brief Loads the palette for the grescale conversion
 *
 */
static void _loadPalette(void);

//==============================================================================
// Functions
//==============================================================================

// System

void trophySystemInit(trophySettings_t* settings, char* modeName)
{
    // Defaults
    trophySystem.beingDrawn = false;
    trophySystem.drawTimer  = 0;
    trophySystem.slideTimer = 0;

    // Copy settings
    trophySystem.settings = settings;

    // If no namespace is provided, auto generate
    if (strcmp(settings->namespaceKey, "") == 0)
    {
        char buffer[MAX_NVS_KEY_LEN];
        _truncateStr(buffer, modeName, MAX_NVS_KEY_LEN);
        strcpy(trophySystem.settings->namespaceKey, buffer);
    }

    // Init palette
    _loadPalette();

    // Clear queue
    for (int idx = 0; idx < TROPHY_MAX_BANNERS; idx++)
    {
        trophySystem.trophyQueue[idx].active = false;
    }

    // Draw defaults unless re-called by dev
    // FIXME: Add default values
    // trophyDrawListInit();
}

int trophySystemGetPoints(char* modeName)
{
    // Return the current points value for the mode.
    int32_t points;
    if (modeName == NULL)
    {
        // Return total points
        return 0;
    }
    if (readNamespaceNvs32(systemPointsNVS[0], modeName, &points))
    {
        if (points > 1000)
        {
            return points;
        }
        return 1000;
    }
    else
    {
        // No value found
        return 0;
    }
}

// Trophies

void trophyUpdate(trophyData_t t, int newVal, bool drawUpdate)
{
    // Load
    trophyDataWrapper_t tw = {};
    _load(&tw, t);

    // Check if trophy is already won and return if true
    if (tw.trophyData.type == TROPHY_TYPE_CHECKLIST)
    {
        // TODO: check if all flags are set
    }
    else if (tw.trophyData.type == TROPHY_TYPE_TRIGGER)
    {
        if (tw.currentVal >= 1)
        {
            return;
        }
    }
    else
    {
        if (tw.currentVal >= tw.trophyData.maxVal)
        {
            return;
        }
    }

    // If the new value is the same as the previously saved value, return
    if (tw.currentVal >= newVal)
    {
        return;
    }
    else
    { // If the newValue has exceeded maxVal, Save value (should work for checklist)
        tw.currentVal = newVal;
        _save(tw, newVal);
        // TODO: Check if won
        // TODO: Update trophyScore
    }

    // Load into draw queue is requested
    if (drawUpdate)
    {
        trophySystem.trophyQueue[trophySystem.idx] = tw;
        // Load sprite
        loadWsg(trophySystem.trophyQueue[trophySystem.idx].trophyData.imageString,
                &trophySystem.trophyQueue[trophySystem.idx].image, true);

        // Set system and this index to active
        trophySystem.active                               = true;
        trophySystem.beingDrawn                           = true;
        trophySystem.trophyQueue[trophySystem.idx].active = true;

        // Reset timers
        trophySystem.animationTick = 0;
        trophySystem.drawTimer     = 0;
        trophySystem.slideTimer    = 0;
        trophySystem.sliding       = trophySystem.settings->animated;
        // TODO: the rest of this

        // TODO: Play sound

        // Increment idx
        trophySystem.idx = _incWithOverflow(trophySystem.idx, TROPHY_MAX_BANNERS);
    }
}

void trophyUpdateMilestone(char* modeName, char* title, int value)
{
    // Checks if first time past 25%, 50%, 75%, or 100% completion.
    // Load saved data
    // Compare saved MavVal to value
    // if (value >= maxVal >> 2 && oldValue < maxVal >> 2) || (value >= maxVal >> 1 && oldValue < maxVal >> 1) || (value
    // >= (maxVal >> 2) * 3 && oldValue < (maxVal >> 2) * 3)
    // Load into queue to draw
    // Else, just save data
}

void trophyClear(trophyData_t t)
{
    trophyDataWrapper_t tw = {};
    _copyTrophy(&tw, t);
    _save(tw, 0);
}

// Helpers

int trophyGetNumTrophies(char* modeName)
{
    // Gets the number of trophies associated with this mode, or all modes if NULL
    return 0;
}

void trophyGetTrophyList(char* modeName, trophyData_t* tList, int* tLen, int offset)
{
    // If modeName is NULL, get all trophies. Order by game, but do not sorting.
    // Populate tList starting from offset up to tLen
}

trophyData_t trophyGetData(char* modeName, char* title)
{
    // Check if trophy exists
    // If so, return trophy_t
    // Else, return null
    trophyData_t t = {};
    return t;
}

// Draw

void trophyDraw(font_t* fnt, int64_t elapsedUs)
{
    // Exit immediately if not being drawn
    if (!trophySystem.active)
    {
        return;
    }

    // Handle which part of the animation is happening (sliding, displaying, or ending)
    if (trophySystem.settings->animated && trophySystem.sliding) // Sliding in or out
    {
        // Based on delta time, update current frame
        trophySystem.slideTimer += elapsedUs;
        int frameLen = (trophySystem.settings->slideMaxDuration * TENTH_SECOND) / TROPHY_BANNER_HEIGHT;
        while (trophySystem.slideTimer >= frameLen)
        {
            trophySystem.animationTick++;
            trophySystem.slideTimer -= frameLen;
        }
        // Change state if fully slid
        if (trophySystem.animationTick == TROPHY_BANNER_HEIGHT)
        {
            // Halfway through
            trophySystem.sliding    = false;
            trophySystem.beingDrawn = true;
        }
        else if (trophySystem.animationTick == TROPHY_BANNER_HEIGHT * 2)
        {
            // End of the animation
            trophySystem.sliding = false; // Disables drawing
            trophySystem.active  = false; // Stops drawing altogether
            trophySystem.currIdx = _incWithOverflow(trophySystem.currIdx, TROPHY_MAX_BANNERS);
        }
        // Draw
        _draw(trophySystem.trophyQueue[trophySystem.currIdx], trophySystem.animationTick, fnt);
    }
    else if (trophySystem.beingDrawn) // Static on screen
    {
        // Regular timer
        trophySystem.drawTimer += elapsedUs;
        if (trophySystem.drawTimer >= trophySystem.settings->drawMaxDuration * TENTH_SECOND)
        {
            // Stop drawing
            trophySystem.beingDrawn                               = false;
            trophySystem.trophyQueue[trophySystem.currIdx].active = false;
            trophySystem.sliding = trophySystem.settings->animated; // Starts sliding again if set
        }
        // Draw
        _draw(trophySystem.trophyQueue[trophySystem.currIdx], STATIC_POSITION, fnt);
    }
    else // Has ended
    {
        // Seek next active queue slot
        for (int idx = 0; idx < TROPHY_MAX_BANNERS; idx++)
        {
            trophySystem.currIdx = _incWithOverflow(trophySystem.currIdx, TROPHY_MAX_BANNERS);
            if (trophySystem.trophyQueue[trophySystem.currIdx].active)
            {
                // Reset for new run
                trophySystem.animationTick = 0;
                trophySystem.drawTimer     = 0;
                trophySystem.slideTimer    = 0;
                trophySystem.beingDrawn    = true;
                trophySystem.sliding       = trophySystem.settings->animated;
                return;
            }
        }
        trophySystem.active = false;
    }
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

static void _save(trophyDataWrapper_t t, int newVal)
{
    char buffer[MAX_NVS_KEY_LEN];
    _truncateStr(buffer, t.trophyData.title, MAX_NVS_KEY_LEN);
    writeNamespaceNvs32(trophySystem.settings->namespaceKey, buffer, newVal);
}

static void _load(trophyDataWrapper_t* tw, trophyData_t t)
{
    // Copy t into tw
    _copyTrophy(tw, t);

    // Pull Current Val from disk
    int32_t val;
    char buffer[MAX_NVS_KEY_LEN];
    _truncateStr(buffer, t.title, MAX_NVS_KEY_LEN);
    if (readNamespaceNvs32(trophySystem.settings->namespaceKey, buffer, &val))
    {
        tw->currentVal = val;
    }
    else
    {
        tw->currentVal = 0;
        writeNamespaceNvs32(trophySystem.settings->namespaceKey, buffer, 0);
    }
}

static void _copyTrophy(trophyDataWrapper_t* tw, trophyData_t t)
{
    strcpy(tw->trophyData.title, t.title);
    strcpy(tw->trophyData.description, t.description);
    strcpy(tw->trophyData.imageString, t.imageString);
    tw->trophyData.type       = t.type;
    tw->trophyData.difficulty = t.difficulty;
    tw->trophyData.maxVal     = t.maxVal;
}

static void _getTrophyData(char* modeName, trophyData_t* tList, int tLen, int offset)
{
    // Load all trophies associated with a mode
}

static void _draw(trophyDataWrapper_t t, int frame, font_t* fnt)
{
    // Get offset
    int yOffset;
    if (frame == STATIC_POSITION)
    {
        if (trophySystem.settings->drawFromBottom)
        {
            yOffset = TFT_HEIGHT - TROPHY_BANNER_HEIGHT;
        }
        else
        {
            yOffset = 0;
        }
    }
    else
    {
        if (frame < TROPHY_BANNER_HEIGHT)
        {
            if (trophySystem.settings->drawFromBottom)
            {
                yOffset = TFT_HEIGHT - frame;
            }
            else
            {
                yOffset = -TROPHY_BANNER_HEIGHT + frame;
            }
        }
        else
        {
            if (trophySystem.settings->drawFromBottom)
            {
                yOffset = TFT_HEIGHT - (TROPHY_BANNER_HEIGHT * 2 - frame);
            }
            else
            {
                yOffset = TROPHY_BANNER_HEIGHT - frame;
            }
        }
    }
    _drawAtYCoord(t, yOffset, fnt);
}

static void _drawAtYCoord(trophyDataWrapper_t t, int yOffset, font_t* fnt)
{
    // Draw box (Gray box, black border)
    fillDisplayArea(0, yOffset, TFT_WIDTH, yOffset + TROPHY_BANNER_HEIGHT, c111);
    drawRect(0, yOffset, TFT_WIDTH, yOffset + TROPHY_BANNER_HEIGHT, c000);
    int xOffset;
    int16_t startX = TROPHY_SCREEN_CORNER_CLEARANCE;
    int16_t startY = yOffset + ((TROPHY_BANNER_HEIGHT - TROPHY_BANNER_MAX_ICON_DIM) >> 1);
    if (strcmp(t.trophyData.imageString, "") == 0)
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
        if (t.currentVal >= t.trophyData.maxVal)
        {
            wp = &trophySystem.normalPalette;
        }
        drawWsgPaletteSimple(&t.image, startX + ((TROPHY_BANNER_MAX_ICON_DIM - t.image.w) >> 1),
                             startY + ((TROPHY_BANNER_MAX_ICON_DIM - t.image.h) >> 1), wp);
    }

    // Draw text, starting after image if present
    // FIXME: Limit length of title text based on if there's space and there's no numbers
    drawText(fnt, c555, t.trophyData.title, xOffset, yOffset + 4); // Title
    startX = xOffset;
    startY = yOffset + 20;
    drawTextWordWrap(fnt, c444, t.trophyData.description, &startX, &startY, TFT_WIDTH - TROPHY_SCREEN_CORNER_CLEARANCE,
                     yOffset + TROPHY_BANNER_HEIGHT); // Description
    // FIXME: Need to display all types differently
    if (!t.trophyData.type == TROPHY_TYPE_TRIGGER)
    {
        char buffer[32];
        snprintf(buffer, sizeof(buffer) - 1, "%d/%d", t.currentVal, t.trophyData.maxVal);
        int16_t xOff         = TFT_WIDTH - (textWidth(fnt, buffer) + TROPHY_SCREEN_CORNER_CLEARANCE + 8);
        paletteColor_t color = c500;
        if (t.currentVal >= t.trophyData.maxVal)
        {
            color = c050;
        }
        drawText(fnt, color, buffer, xOff, yOffset + 4);
    }
}

static void _truncateStr(char* to, char* from, int len)
{
    strncpy(to, from, len);
    to[len - 1] = '\0';
}

static int _incWithOverflow(int val, int max)
{
    val++;
    if (val >= max)
    {
        val = 0;
    }
    return val;
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