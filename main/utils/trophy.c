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
#define TENTH_SECOND    100000
#define STATIC_POSITION -1

// Visuals
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

// System variables for display,
typedef struct
{
    // Assets
    wsg_t imageArray[TROPHY_MAX_BANNERS]; //< WSGs loaded for next five trophies.
    trophySettings_t* tSettings; //< The settings of how the trophies behave

    // Drawing
    bool active;                              //< If the mode should be drawing a banner
    trophy_t tBannerList[TROPHY_MAX_BANNERS]; //< List of banners to display.
    bool beingDrawn;                          //< If banner is currently being drawn statically
    int32_t drawTimer;          //< Accumulates until more than Max duration
    wsgPalette_t grayPalette;   //< Grayscale palette for locked trophies
    wsgPalette_t normalPalette; //< Normal colors

    // Animation
    bool sliding; //< If currently sliding
    int32_t slideTimer;    //< Accumulates until more than Max duration
    int16_t animationTick; //< What tick of the animation is active based on timer
} trophySystem_t;

//==============================================================================
// Variables
//==============================================================================

static trophySystem_t tSystem = {}; //< Should be one instance per swadge, should always be in memory

//==============================================================================
// Static function declarations
//==============================================================================

/**
 * @brief Saves a trophy. Checks data is actually fresh before saving to avoid hammering NVS
 *
 * @param modeName Name of the mode for namespace
 * @param t Data to write
 */
static void _save(char* modeName, trophy_t t);

/**
 * @brief Loads data from NVS given a string. Returns NULL if not found
 *
 * @param modeName Name of the mode
 * @param title Title of trophy to load
 * @return trophy_t Trophy data. NULL is not found
 */
static trophy_t _load(char* modeName, char* title);

/**
 * @brief Grabs all trophy data from NVS
 *
 * @param modeName Name of the mode to grab data for. NULL is all modes
 * @param tList Outfile of list of Trophy data from NVS. Will eat RAM
 * @param tLen Number of trophies to grab
 * @param offset Where to start in list
 */
static void _getTrophyData(char* modeName, trophy_t* tList, int tLen, int offset);

/**
 * @brief Draws the banner
 *
 * @param t Data to feed
 * @param frame Which frame is being drawn. STATIC_POSITION is fully visible, other values change the offset.
 * @param fnt Font used in draw call
 */
static void _draw(trophy_t t, int frame, font_t* fnt);

/**
 * @brief Sub function of _draw() so code can be reused in list draw command
 *
 * @param t Trophy to draw
 * @param yOffset y coordinate to start at
 * @param fnt Font used in draw call
 */
static void _drawAtYCoord(trophy_t t, int yOffset, font_t* fnt);

//==============================================================================
// Functions
//==============================================================================

// System

void trophySystemInit(trophySettings_t* settings)
{
    // Defaults
    tSystem.beingDrawn = false;
    tSystem.drawTimer  = 0;
    tSystem.slideTimer = 0;

    // Copy settings
    tSystem.tSettings = settings;

    // Init palette
    wsgPaletteReset(&tSystem.normalPalette);
    for (int idx = 0; idx < 217; idx++)
    {
        if (idx == cTransparent) // Transparent
        {
            wsgPaletteSet(&tSystem.grayPalette, idx, cTransparent);
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
                    wsgPaletteSet(&tSystem.grayPalette, idx, c333);
                    break;
                }
                default:
                {
                    wsgPaletteSet(&tSystem.grayPalette, idx, c444);
                    break;
                }
            }
        }
    }
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

void trophyInit(char* modeName, char* title, char* imgStr, char* desc, trophyTypes_t type, int8_t points, int maxVal)
{
    // Check if the trophy has already been initialized to avoid overwriting
    // Save data to NVS
    // If first trophy (namespace doesn't exist) spawn completionist trophy
}

void trophyUpdate(char* modeName, char* title, int value, bool drawUpdate)
{
    // Check if trophy exists, throw error if not
    // Check if trophy is already won and bail if true
    // Check if value is greater than previous value - Do not draw if false
    // Check if this update has won the trophy and add to points if so
    // Save value to NVS
    // If drawUpdate, save trophy to list to display
    // - Load into next slot in system
    // - Load WSG
    // - reset all timers, etc if not already active
    // - Set system to active if not set
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

void trophyClear(char* modeName, char* title)
{
    // If Trophy exists, reset it to zero
    // If was won, delete points
}

// Helpers

int trophyGetNumTrophies(char* modeName)
{
    // Gets the number of trophies associated with this mode, or all modes if NULL
    return 0;
}

void trophyGetTrophyList(char* modeName, trophy_t* tList, int* tLen, int offset)
{
    // If modeName is NULL, get all trophies. Order by game, but do not sorting.
    // Populate tList starting from offset up to tLen
}

trophy_t trophyGetData(char* modeName, char* title)
{
    // Check if trophy exists
    // If so, return trophy_t
    // Else, return null
    trophy_t t = {};
    return t;
}

// Draw

void trophyDraw(char* modeName, font_t* fnt, int64_t elapsedUs)
{
    // Exit immediately if not being drawn
    if (!tSystem.active)
    {
        return;
    }
    // Draws the trophy
    if (tSystem.tSettings->animated && tSystem.sliding) // Sliding in or out
    {
        tSystem.slideTimer += elapsedUs;
        int frameLen = (tSystem.tSettings->slideMaxDuration * TENTH_SECOND) / TROPHY_BANNER_HEIGHT;
        while (tSystem.slideTimer >= frameLen)
        {
            tSystem.animationTick++;
            tSystem.slideTimer -= frameLen;
        }
        if (tSystem.animationTick == TROPHY_BANNER_HEIGHT)
        {
            tSystem.sliding    = false;
            tSystem.beingDrawn = true;
        }
        else if (tSystem.animationTick == TROPHY_BANNER_HEIGHT * 2)
        {
            tSystem.sliding = false; // Disables drawing
            tSystem.active  = false; // Stops rawing altogether
        }
        _draw(tSystem.tBannerList[0], tSystem.animationTick, fnt);
    }
    else if (tSystem.beingDrawn) // Static on screen
    {
        // Regular timer
        tSystem.drawTimer += elapsedUs;
        if (tSystem.drawTimer >= tSystem.tSettings->drawMaxDuration)
        {
            // Stop drawing
            tSystem.beingDrawn = false;
            tSystem.sliding    = tSystem.tSettings->animated; // Starts sliding again if set
        }
        // Draw
        _draw(tSystem.tBannerList[0], STATIC_POSITION, fnt);
    }
    else // Has ended
    {
        tSystem.active = false;
    }
    // TODO: play sound (?)
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

static void _save(char* modeName, trophy_t t)
{
    // Check if the data in the trophy is identical to incoming data, and exit if so
    // Save the trophy
}

static trophy_t _load(char* modeName, char* title)
{
    // Loads trophy from NVS
    trophy_t t = {};
    return t;
}

static void _getTrophyData(char* modeName, trophy_t* tList, int tLen, int offset)
{
    // Load all trophies associated with a mode
}

static void _draw(trophy_t t, int frame, font_t* fnt)
{
    // Get offset
    int yOffset;
    if (frame == STATIC_POSITION)
    {
        if (tSystem.tSettings->drawFromBottom)
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
            if (tSystem.tSettings->drawFromBottom)
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
            if (tSystem.tSettings->drawFromBottom)
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

static void _drawAtYCoord(trophy_t t, int yOffset, font_t* fnt)
{
    // Draw box (Gray box, black border)
    fillDisplayArea(0, yOffset, TFT_WIDTH, yOffset + TROPHY_BANNER_HEIGHT, c111);
    drawRect(0, yOffset, TFT_WIDTH, yOffset + TROPHY_BANNER_HEIGHT, c000);
    // Draw image if not NULL
    int xOffset;
    // FIXME: Limit length of title text based on if there's space and there's no numbers
    if (t.imageString == NULL)
    {
        // Draw text at start of buffer area
        xOffset = TROPHY_SCREEN_CORNER_CLEARANCE;
    }
    else
    {
        // Set xOffset to be 2* H_BUFFER + ICON_WIDTH
        xOffset = TROPHY_BANNER_MAX_ICON_DIM + TROPHY_SCREEN_CORNER_CLEARANCE + TROPHY_IMAGE_BUFFER;
    }
    // Draw Image + shadowbox
    int16_t startX = TROPHY_SCREEN_CORNER_CLEARANCE;
    int16_t startY = yOffset + ((TROPHY_BANNER_HEIGHT - TROPHY_BANNER_MAX_ICON_DIM) >> 1);
    drawRectFilled(startX, startY, startX + TROPHY_BANNER_MAX_ICON_DIM, startY + TROPHY_BANNER_MAX_ICON_DIM, c222);

    // Draw WSG
    wsgPalette_t* wp = &tSystem.grayPalette;
    if (t.currentValue >= t.maxValue)
    {
        wp = &tSystem.normalPalette;
    }
    drawWsgPaletteSimple(&tSystem.imageArray[0], startX + ((TROPHY_BANNER_MAX_ICON_DIM - tSystem.imageArray[0].w) >> 1),
                         startY + ((TROPHY_BANNER_MAX_ICON_DIM - tSystem.imageArray[0].h) >> 1), wp);

    // Draw text, starting after image if present
    drawText(fnt, c555, t.title, xOffset, yOffset + 4); // Title
    startX = xOffset;
    startY = yOffset + 20;
    drawTextWordWrap(fnt, c444, t.description, &startX, &startY, TFT_WIDTH - TROPHY_SCREEN_CORNER_CLEARANCE,
                     yOffset + TROPHY_BANNER_HEIGHT); // Description

    if (!t.type == TROPHY_TYPE_TRIGGER)
    {
        char buffer[32];
        snprintf(buffer, sizeof(buffer) - 1, "%d/%d", t.currentValue, t.maxValue);
        int16_t xOff         = TFT_WIDTH - (textWidth(fnt, buffer) + TROPHY_SCREEN_CORNER_CLEARANCE + 8);
        paletteColor_t color = c500;
        if (t.currentValue >= t.maxValue)
        {
            color = c050;
        }
        drawText(fnt, color, buffer, xOff, yOffset + 4);
    }
}

//==============================================================================
// Test funcs
//==============================================================================

// FIXME: Delete when done using

void trophyDrawDataDirectly(trophy_t t, int y, font_t* fnt)
{
    _drawAtYCoord(t, y, fnt);
}

void loadImage(int idx, char* string)
{
    loadWsg(string, &tSystem.imageArray[idx], true);
}

void unloadImage(int idx)
{
    freeWsg(&tSystem.imageArray[idx]);
}