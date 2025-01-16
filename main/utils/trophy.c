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
#define MAX_BANNERS     5
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

typedef struct
{
    // Assets
    font_t font;                    //< Font in use. Should not be set by user.
    wsg_t imageArray[MAX_BANNERS]; //< WSGs loaded for next five trophies.

    // Drawing
    trophy_t tBannerList[MAX_BANNERS]; //< List of banners to display.
    bool active;                       //< If the mode should be drawing a banner
    bool beingDrawn;                   //< If banner is currently being drawn statically
    bool drawFromBottom;               //< If banner should be drawn from the bottom of the screen
    int32_t drawMaxDuration;           //< How long the banner will be drawn fully extended
    int32_t drawTimer;                 //< Accumulates until more than Max duration

    // Animation
    bool animated;            //< If being animated
    bool sliding;             //< If currently sliding
    int32_t slideMaxDuration; //< How long the banner will be drawn fully extended
    int32_t slideTimer;       //< Accumulates until more than Max duration
    int16_t animationTick;    //< What tick of the animation is active based on timer
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
 */
static void _draw(trophy_t t, int frame);

/**
 * @brief Sub function of _draw() so code can be reused in list draw command
 *
 * @param t Trophy to draw
 * @param yOffset y coordinate to start at
 */
static void _drawAtYCoord(trophy_t t, int yOffset);

//==============================================================================
// Functions
//==============================================================================

// System

void trophySystemInit(bool bottom, int displayDuration, bool animate, int slideDuration)
{
    // Defaults
    tSystem.beingDrawn = false;
    tSystem.drawTimer  = 0;
    tSystem.slideTimer = 0;

    // Sets the direction the banner comes from
    tSystem.drawFromBottom = bottom;

    // Set Display duration
    tSystem.drawMaxDuration = displayDuration;

    // Set if animating
    tSystem.animated = animate;

    // Set the scroll speed
    tSystem.slideMaxDuration = slideDuration;
}

void trophySystemSetFont(char* font)
{
    // Sets the system font
    // NOTE: Should load automatically on Swadge boot/loading a mode
    loadFont(font, &tSystem.font, true);
}

void trophySystemClearFont()
{
    // Unloads the system font
    // NOTE: Should only be used if the trophy system isn't being used and the font is taking up too much space
    freeFont(&tSystem.font);
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

void trophyDraw(char* modeName, char* title, int64_t elapsedUs)
{
    // Exit immediately if not being drawn
    if (!tSystem.active)
    {
        return;
    }
    // Draws the trophy
    if (tSystem.animated && tSystem.sliding) // Sliding in or out
    {
        tSystem.slideTimer += elapsedUs;
        int frameLen = (tSystem.slideMaxDuration * TENTH_SECOND) / TROPHY_BANNER_HEIGHT;
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
        _draw(tSystem.tBannerList[0], tSystem.animationTick);
    }
    else if (tSystem.beingDrawn) // Static on screen
    {
        // Regular timer
        tSystem.drawTimer += elapsedUs;
        if (tSystem.drawTimer >= tSystem.drawMaxDuration)
        {
            // Stop drawing
            tSystem.beingDrawn = false;
            tSystem.sliding    = tSystem.animated; // Starts sliding again if set
        }
        // Draw
        _draw(tSystem.tBannerList[0], STATIC_POSITION);
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

static void _draw(trophy_t t, int frame)
{
    // Get offset
    int yOffset;
    if (frame == STATIC_POSITION)
    {
        if (tSystem.drawFromBottom)
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
            if (tSystem.drawFromBottom)
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
            if (tSystem.drawFromBottom)
            {
                yOffset = TFT_HEIGHT - (TROPHY_BANNER_HEIGHT * 2 - frame);
            }
            else
            {
                yOffset = TROPHY_BANNER_HEIGHT - frame;
            }
        }
    }
    _drawAtYCoord(t, yOffset);
}

static void _drawAtYCoord(trophy_t t, int yOffset)
{
    // Draw box (Gray box, black border)
    fillDisplayArea(0, yOffset, TFT_WIDTH, TROPHY_BANNER_HEIGHT, c111);
    drawRect(0, yOffset, TFT_WIDTH, TROPHY_BANNER_HEIGHT, c000);
    // Draw image if not NULL
    int xOffset;
    if (t.imageString == NULL)
    {
        // Draw text at start of buffer area
        xOffset = TROPHY_SCREEN_CORNER_CLEARANCE;
    }
    else
    {
        // Draw icon
        // TODO: If in progress, draw grayscale? Outline?
        // Set xOffset to be 2* H_BUFFER + ICON_WIDTH
        xOffset = TROPHY_BANNER_MAX_ICON_DIM + TROPHY_SCREEN_CORNER_CLEARANCE + TROPHY_IMAGE_BUFFER;
    }
    // Draw Image + shadowbox
    int16_t startX = TROPHY_SCREEN_CORNER_CLEARANCE;
    int16_t startY = yOffset + ((TROPHY_BANNER_HEIGHT - TROPHY_BANNER_MAX_ICON_DIM) >> 1);
    drawRectFilled(startX, startY, startX + TROPHY_BANNER_MAX_ICON_DIM, startY + TROPHY_BANNER_MAX_ICON_DIM, c222);

    // TODO: Load WSG properly
    drawWsgSimple(&tSystem.imageArray[0], startX + ((TROPHY_BANNER_MAX_ICON_DIM - tSystem.imageArray[0].w) >> 1), startY + ((TROPHY_BANNER_MAX_ICON_DIM - tSystem.imageArray[0].h) >> 1));

    // Draw text, starting after image if present
    drawText(&tSystem.font, c555, t.title, xOffset, yOffset + 4); // Title
    startX = xOffset;
    startY = yOffset + 20;
    drawTextWordWrap(&tSystem.font, c444, t.description, &startX, &startY, TFT_WIDTH - TROPHY_SCREEN_CORNER_CLEARANCE,
                     TROPHY_BANNER_HEIGHT); // Description

    if (!t.type == TROPHY_TYPE_TRIGGER)
    {
        char buffer[32];
        snprintf(buffer, sizeof(buffer) - 1, "%d/%d", t.currentValue, t.maxValue);
        int16_t xOff         = TFT_WIDTH - (textWidth(&tSystem.font, buffer) + TROPHY_SCREEN_CORNER_CLEARANCE + 8);
        paletteColor_t color = c500;
        if (t.currentValue >= t.maxValue)
        {
            color = c050;
        }
        drawText(&tSystem.font, color, buffer, xOff, yOffset + 4);
    }
}

//==============================================================================
// Test funcs
//==============================================================================

// FIXME: Delete when done using

void trophyDrawDataDirectly(trophy_t t, int y)
{
    _drawAtYCoord(t, y);
}

void loadImage(int idx, char* string)
{
    loadWsg(string, &tSystem.imageArray[idx], true);
}

void unloadImage(int idx)
{
    freeWsg(&tSystem.imageArray[idx]);
}