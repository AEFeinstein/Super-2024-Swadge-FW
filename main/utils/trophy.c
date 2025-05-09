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

// Data Structure
#include "linked_list.h"

//==============================================================================
// Defines
//==============================================================================

// Standard defines
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
    list_t trophyQueue; //< List of trophy updates to display. Holds type \ref trophyDataWrapper_t*
    node_t* currDisp;   //< The current trophy being displayed. Holds type \ref trophyDataWrapper_t*

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
static void _save(trophyDataWrapper_t* t, int newVal);

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
static void _draw(trophyDataWrapper_t* t, int frame, font_t* fnt);

/**
 * @brief Sub function of _draw() so code can be reused in list draw command
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
 * @brief Get the currently displayed trophy
 */
static trophyDataWrapper_t* getCurrentDisplayTrophy(void);

//==============================================================================
// Functions
//==============================================================================

// System

void trophySystemInit(trophySettings_t* settings, const char* modeName)
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

    // Clear queue, freeing all entries
    void* toFree;
    while ((toFree = pop(&trophySystem.trophyQueue)))
    {
        heap_caps_free(toFree);
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
    trophyDataWrapper_t* tw = heap_caps_calloc(1, sizeof(trophyDataWrapper_t), MALLOC_CAP_8BIT);
    _load(tw, t);

    // Check if trophy is already won and return if true
    if (tw->trophyData.type == TROPHY_TYPE_CHECKLIST)
    {
        // TODO: Handle just sending bit flags?
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
    { // If the newValue has exceeded maxVal, Save value (should work for checklist)
        tw->currentVal = newVal;
        _save(tw, newVal);

        // Update score
        if (tw->currentVal >= tw->trophyData.maxVal)
        {
            // TODO: Update score
        }
    }

    // Load into draw queue is requested
    if (drawUpdate)
    {
        // Push into queue
        push(&trophySystem.trophyQueue, tw);

        // Load sprite
        loadWsg(tw->trophyData.imageString, &tw->image, true);

        // Set system and this index to active
        if (!trophySystem.active)
        {
            trophySystem.active        = true;
            trophySystem.animationTick = 0;
            trophySystem.drawTimer     = 0;
            trophySystem.slideTimer    = 0;
            trophySystem.sliding       = trophySystem.settings->animated;
            trophySystem.beingDrawn    = true;
        }

        // Set active
        tw->active = true;

        // Set active
        tw->active = true;

        // TODO: the rest of this

        // TODO: Play sound
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

int32_t trophyGetSavedValue(trophyData_t t)
{
    int32_t val;
    char buffer[MAX_NVS_KEY_LEN];
    _truncateStr(buffer, t.title, MAX_NVS_KEY_LEN);
    if (readNamespaceNvs32(trophySystem.settings->namespaceKey, buffer, &val))
    {
        return val;
    }
    return 0;
}

void trophyClear(trophyData_t t)
{
    trophyDataWrapper_t tw = {};
    _copyTrophy(&tw, t);
    _save(&tw, 0);
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
        int frameLen = (trophySystem.settings->slideMaxDurationUs) / TROPHY_BANNER_HEIGHT;
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
            // FIXME: Stops drawing, probably not iterating right
            trophySystem.currDisp = getNextWraparound(&trophySystem.trophyQueue, trophySystem.currDisp);
            if (getCurrentDisplayTrophy()->active)
            {
                // Reset for new run
                trophySystem.animationTick = 0;
                trophySystem.drawTimer     = 0;
                trophySystem.slideTimer    = 0;
                trophySystem.sliding       = trophySystem.settings->animated;
                trophySystem.beingDrawn    = true;
                return;
            }
            trophySystem.currDisp = getNextWraparound(&trophySystem.trophyQueue, trophySystem.currDisp);
            trophySystem.active   = false;
        }
        // Draw
        _draw(getCurrentDisplayTrophy(), trophySystem.animationTick, fnt);
    }
    else if (trophySystem.beingDrawn) // Static on screen
    {
        // Regular timer
        trophySystem.drawTimer += elapsedUs;
        if (trophySystem.drawTimer >= trophySystem.settings->drawMaxDurationUs)
        {
            // Stop drawing
            trophySystem.beingDrawn           = false;
            getCurrentDisplayTrophy()->active = false;
            trophySystem.sliding              = trophySystem.settings->animated; // Starts sliding again if set
        }
        // Draw
        _draw(getCurrentDisplayTrophy(), STATIC_POSITION, fnt);
    }
    else // Has ended
    {
        // Seek next active queue slot
        for (int idx = 0; idx < TROPHY_MAX_BANNERS; idx++)
        {
            trophySystem.currDisp = getNextWraparound(&trophySystem.trophyQueue, trophySystem.currDisp);
            if (getCurrentDisplayTrophy()->active)
            {
                // Reset for new run
                trophySystem.animationTick = 0;
                trophySystem.drawTimer     = 0;
                trophySystem.slideTimer    = 0;
                trophySystem.sliding       = trophySystem.settings->animated;
                trophySystem.beingDrawn    = true;
                return;
            }
        }
        trophySystem.currDisp = getNextWraparound(&trophySystem.trophyQueue, trophySystem.currDisp);
        trophySystem.active   = false;
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

static void _save(trophyDataWrapper_t* t, int newVal)
{
    char buffer[MAX_NVS_KEY_LEN];
    _truncateStr(buffer, t->trophyData.title, MAX_NVS_KEY_LEN);
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

static void _draw(trophyDataWrapper_t* t, int frame, font_t* fnt)
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
        snprintf(buffer, sizeof(buffer) - 1, "%d/%d", t->currentVal, t->trophyData.maxVal);
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

static trophyDataWrapper_t* getCurrentDisplayTrophy(void)
{
    // If there's nothing being currently displayed, set it
    if (!trophySystem.currDisp)
    {
        trophySystem.currDisp = trophySystem.trophyQueue.first;
    }

    // Make sure there's something to display
    if (trophySystem.currDisp)
    {
        return (trophyDataWrapper_t*)trophySystem.currDisp->val;
    }

    // Nothing in the queue...
    return NULL;
}
