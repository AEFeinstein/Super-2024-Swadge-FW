#include "menu.h"
#include "menuLogbookRenderer.h"
#include "marbles.h"
#include "font.h"
#include "wsg.h"
#include "hdw-tft.h"
#include "linked_list.h"
#include "geometry.h"
#include "trigonometry.h"

#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>

//==============================================================================
// Defines
//==============================================================================

/// Whether to draw the paths as a debug option
#define DRAW_PATH 1

#define MARBLE_R 6

//==============================================================================
// Const Variables
//==============================================================================

static const char marblesName[] = "Marbles";
static const char marblesPlay[] = "Play";

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    MAIN_MENU,
    IN_LEVEL,
} marblesScreen_t;

typedef enum
{
    MARBLE,
    MARBLE_GROUP,
    SHOOTER,
} marblesEntType_t;

typedef enum
{
    NORMAL,
} marbleType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    marbleType_t type;
    paletteColor_t color;
} marble_t;

typedef struct
{
    marblesEntType_t type; ///< Type of this entity

    int16_t x;        ///< Entity's X coordinate
    int16_t y;        ///< Entity's Y coordinate
    int16_t velocity; ///< Entity's velocity
    int16_t angle;    ///< Entity's heading angle, if not on a track

    uint8_t track;    ///< If nonzero, this entity is on the track with this number
    int32_t trackPos; ///< If on a track, this is the entity's position along its track

    union
    {
        marble_t marble; ///< Entity data for a single marble

        /// @brief Entity data for a touching group of marbles
        struct
        {
            uint8_t count;     ///< Number of marbles in this group
            marble_t* marbles; ///< Each marble in this group, starting from the tip
        } marbleGroup;

        /// @brief Entity data for a marble shooter
        struct
        {
            bool hasMarble;
            marble_t nextMarble;
        } shooter;
    };
} marblesEntity_t;

typedef struct
{
    uint16_t index;    ///< The index at which the special should appear, replacing the normal marble
    marbleType_t type; ///< The type of special marble to generate
} marblesSpecial_t;

typedef struct
{
    uint16_t x;
    uint16_t y;
} point_t;

/// @brief Defines the starting state for a level
typedef struct
{
    uint8_t numColors;          ///< Number of marble colors to generate
    paletteColor_t* colors;     ///< Array of marble colors
    uint16_t numSpecials;       ///< The number of special marbles
    marblesSpecial_t* specials; ///< An array of info defining when to generate special marbles

    uint16_t numMarbles; ///< The total number of marbles to generate this level

    /// @brief Contains the path info. Will probably get more complicated later.
    struct
    {
        uint8_t length;  ///< The number of points in the track
        point_t* points; ///< The array of points
    } track;

    point_t shooterLoc; ///< The location of the center of the player's marble shooter
} marblesLevel_t;

/// @brief Holds the state for the entire mode
typedef struct
{
    font_t ibm;
    menu_t* menu;
    menuLogbookRenderer_t* renderer;

    wsg_t arrow18;

    buttonBit_t buttonState;

    marblesScreen_t screen;
    marblesLevel_t* level; ///< The current level's initial settings
    list_t entities;       ///< The entities in the current level
} marblesMode_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void marblesEnterMode(void);
static void marblesExitMode(void);
static void marblesMainLoop(int64_t elapsedUs);
static void marblesBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void marblesMenuCb(const char*, bool selected, uint32_t settingVal);

static void marblesHandleButton(buttonEvt_t evt);
static void marblesUpdatePhysics(int64_t elapsedUs);
static void marblesDrawLevel(void);
static void marblesLoadLevel(void);
static void marblesUnloadLevel(void);

static void marblesCalculateTrackPos(uint8_t trackLength, const point_t* points, int32_t position, int16_t* x,
                                     int16_t* y);
static bool collideMarbles(const marblesEntity_t* a, const marblesEntity_t* b);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t marblesMode = {
    .modeName                 = marblesName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .fnEnterMode              = marblesEnterMode,
    .fnExitMode               = marblesExitMode,
    .fnMainLoop               = marblesMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = marblesBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

marblesMode_t* marbles;

//==============================================================================
// Functions
//==============================================================================

/**
 * This function is called when this mode is started. It should initialize
 * variables and start the mode.
 */
static void marblesEnterMode(void)
{
    marbles = calloc(1, sizeof(marblesMode_t));
    loadFont("ibm_vga8.font", &marbles->ibm, false);

    loadWsg("arrow18.wsg", &marbles->arrow18, false);

    marbles->menu = initMenu(marblesName, marblesMenuCb);
    addSingleItemToMenu(marbles->menu, marblesPlay);

    marbles->renderer = initMenuLogbookRenderer(&marbles->ibm);

    marbles->screen = MAIN_MENU;
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void marblesExitMode(void)
{
    deinitMenuLogbookRenderer(marbles->renderer);
    deinitMenu(marbles->menu);
    freeWsg(&marbles->arrow18);
    freeFont(&marbles->ibm);
    free(marbles);
}

static void marblesHandleButton(buttonEvt_t evt)
{
    switch (marbles->screen)
    {
        case MAIN_MENU:
        {
            marbles->menu = menuButton(marbles->menu, evt);
            break;
        }

        case IN_LEVEL:
        {
            switch (evt.button)
            {
                case PB_UP:
                    break;

                case PB_DOWN:
                    break;

                case PB_LEFT:
                    break;

                case PB_RIGHT:
                    break;

                case PB_A:
                    break;

                case PB_START:
                    marblesUnloadLevel();
                    marblesLoadLevel();
                    break;

                case PB_SELECT:
                    break;

                case PB_B:
                    marbles->screen = MAIN_MENU;
                    marblesUnloadLevel();
                    break;

                case TB_0:
                case TB_1:
                case TB_2:
                case TB_3:
                case TB_4:
                    break;
            }
            marbles->buttonState = evt.state;
            break;
        }
    }
}

/**
 * @brief Update the physics and perform collision checks on all entities
 *
 * @param elapsedUs Time since the last call
 */
static void marblesUpdatePhysics(int64_t elapsedUs)
{
    marblesEntity_t* prevEntity = NULL;

    // Loop over entities, handle movement, do collision checks
    node_t* node = marbles->entities.first;
    while (node != NULL)
    {
        bool cull               = false;
        marblesEntity_t* entity = (marblesEntity_t*)(node->val);
        switch (entity->type)
        {
            case MARBLE:
            {
                if (entity->track)
                {
                    entity->trackPos += entity->velocity;
                    marblesCalculateTrackPos(marbles->level->track.length, marbles->level->track.points,
                                             entity->trackPos, &entity->x, &entity->y);

                    if (prevEntity && prevEntity->type == MARBLE && prevEntity->track)
                    {
                        while (collideMarbles(prevEntity, entity) && entity->trackPos > 0)
                        {
                            // go in the reverse of the actual velocity
                            entity->trackPos += (entity->velocity >= 0 ? -1 : 1);
                            marblesCalculateTrackPos(marbles->level->track.length, marbles->level->track.points,
                                                     entity->trackPos, &entity->x, &entity->y);
                        }
                    }
                }
                else
                {
                    entity->x += entity->velocity * getCos1024(entity->angle) / 1024;
                    entity->y += entity->velocity * getSin1024(entity->angle) / 1024;

                    if (entity->x + MARBLE_R < 0 || entity->y + MARBLE_R < 0 || entity->x > TFT_WIDTH + MARBLE_R
                        || entity->y > TFT_HEIGHT + MARBLE_R)
                    {
                        // Shot marble is out of bounds, cull it
                        cull = true;
                    }
                }
                break;
            }

            case MARBLE_GROUP:
            {
                break;
            }

            case SHOOTER:
            {
                if (marbles->buttonState & PB_LEFT)
                {
                    entity->angle = (entity->angle + 5) % 360;
                }
                else if (marbles->buttonState & PB_RIGHT)
                {
                    entity->angle = (entity->angle + 355) % 360;
                }
                break;
            }
        }

        node = node->next;
        if (cull)
        {
            free(removeEntry(&marbles->entities, node->prev));
        }
        else
        {
            prevEntity = entity;
        }
    }
}

/**
 * @brief Draw the game level and entities
 *
 */
static void marblesDrawLevel(void)
{
#ifdef DRAW_PATH
    for (uint8_t i = 0; i < marbles->level->track.length - 1; i++)
    {
        drawLine(marbles->level->track.points[i].x, marbles->level->track.points[i].y,
                 marbles->level->track.points[i + 1].x, marbles->level->track.points[i + 1].y, c111, 0);
    }
#endif

    node_t* node = marbles->entities.first;
    while (node != NULL)
    {
        marblesEntity_t* entity = (marblesEntity_t*)(node->val);
        switch (entity->type)
        {
            case MARBLE:
            {
                drawCircleFilled(entity->x, entity->y, MARBLE_R, entity->marble.color);
                break;
            }

            case MARBLE_GROUP:
            {
                break;
            }

            case SHOOTER:
            {
                drawWsg(&marbles->arrow18, entity->x, entity->y, false, false, 359 - (entity->angle + 270) % 360);
                break;
            }
        }

        node = node->next;
    }
}

/**
 * @brief Loads a level into the mode structure
 *
 */
static void marblesLoadLevel(void)
{
    marblesLevel_t* level = calloc(1, sizeof(marblesLevel_t));

    /////////////////////////////////////////////////////////////////////
    // Simulate loading the level from a blob or other level generator //
    /////////////////////////////////////////////////////////////////////
    level->numColors = 4;
    level->colors    = calloc(level->numColors, sizeof(paletteColor_t));
    // TODO pick more accessible colors
    level->colors[0] = c500; // red
    level->colors[1] = c550; // yellow
    level->colors[2] = c050; // green
    level->colors[3] = c055; // cyan

    level->numMarbles  = 10;
    level->numSpecials = 0;
    level->specials    = calloc(level->numSpecials, sizeof(marblesSpecial_t));

    level->shooterLoc.x = TFT_WIDTH / 2;
    level->shooterLoc.y = TFT_HEIGHT / 2;

    level->track.length = 4;
    level->track.points = calloc(level->track.length, sizeof(point_t));

    level->track.points[0].x = TFT_WIDTH;
    level->track.points[0].y = TFT_HEIGHT / 4;

    level->track.points[1].x = TFT_WIDTH / 2;
    level->track.points[1].y = TFT_HEIGHT / 4;

    level->track.points[2].x = TFT_WIDTH / 4;
    level->track.points[2].y = TFT_HEIGHT / 2;

    level->track.points[3].x = TFT_WIDTH / 3;
    level->track.points[3].y = TFT_HEIGHT - 1;

    marbles->level = level;

    /////////////////////////////////////////////////////////////////////
    // Handle actually setting up the game after loading the level     //
    /////////////////////////////////////////////////////////////////////
    marblesEntity_t* shooter = calloc(1, sizeof(marblesEntity_t));

    shooter->type  = SHOOTER;
    shooter->x     = level->shooterLoc.x;
    shooter->y     = level->shooterLoc.y;
    shooter->angle = 90;
    push(&marbles->entities, shooter);

    for (uint8_t i = 0; i < 4; i++)
    {
        marblesEntity_t* marble = calloc(1, sizeof(marblesEntity_t));
        marble->track           = 1;
        marble->trackPos        = 50 * (3 - i);
        marble->velocity        = 5;
        marble->marble.type     = NORMAL;
        marble->marble.color    = level->colors[i];
        push(&marbles->entities, marble);
    }
}

static void marblesUnloadLevel(void)
{
    if (marbles->level->specials)
    {
        free(marbles->level->specials);
    }

    free(marbles->level->track.points);
    free(marbles->level->colors);

    free(marbles->level);
    marbles->level = NULL;

    marblesEntity_t* val = NULL;
    while (NULL != (val = pop(&marbles->entities)))
    {
        free(val);
    }
}

/**
 * This function is called from the main loop. It's pretty quick, but the
 * timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
static void marblesMainLoop(int64_t elapsedUs)
{
    clearPxTft();

    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        marblesHandleButton(evt);
    }

    switch (marbles->screen)
    {
        case MAIN_MENU:
        {
            drawMenuLogbook(marbles->menu, marbles->renderer, elapsedUs);
            break;
        }

        case IN_LEVEL:
        {
            marblesUpdatePhysics(elapsedUs);
            marblesDrawLevel();
            break;
        }
    }
}

/**
 * This function is called when the display driver wishes to update a
 * section of the display.
 *
 * @param disp The display to draw to
 * @param x the x coordiante that should be updated
 * @param y the x coordiante that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param upNum update number denominator
 */
static void marblesBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, c555);
}

/**
 * @brief Callback for when menu items are selected
 *
 * @param label The menu item that was selected or moved to
 * @param selected true if the item was selected, false if it was moved to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
static void marblesMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (label == marblesPlay)
    {
        marblesLoadLevel();
        marbles->screen = IN_LEVEL;
    }
}

/**
 * @brief Calculates the position of a point a certain distance along a track
 *
 * @param trackLength The number of points in the track path
 * @param points An array of points defining the track's path
 * @param position The position along the path, in thousandths-of-segment increments
 * @param[out] x A pointer to an int to be updated to the result X-coordinate
 * @param[out] y A pointer to an int to be updated to the result Y-coordinate
 */
static void marblesCalculateTrackPos(uint8_t trackLength, const point_t* points, int32_t distance, int16_t* x,
                                     int16_t* y)
{
    if (distance > (trackLength - 1) * 1000)
    {
        return;
    }

    int32_t startPoint = distance / 1000;

    if (startPoint == (trackLength - 1))
    {
        // Return the last point if it's right at the end
        *x = points[trackLength - 1].x;
        *y = points[trackLength - 1].y;
    }
    else
    {
        int32_t n = distance % 1000;
        // Otherwise, linear interpolation!
        const point_t* p0 = (points + startPoint);
        const point_t* p1 = (points + startPoint + 1);

        *x = ((999 - n) * p0->x + n * p1->x) / 1000;
        *y = ((999 - n) * p0->y + n * p1->y) / 1000;
    }
}

static bool collideMarbles(const marblesEntity_t* a, const marblesEntity_t* b)
{
    circle_t circleA, circleB;
    circleA.x      = a->x;
    circleA.y      = a->y;
    circleA.radius = MARBLE_R;

    circleB.x      = b->x;
    circleB.y      = b->y;
    circleB.radius = MARBLE_R;

    return circleCircleIntersection(circleA, circleB);
}