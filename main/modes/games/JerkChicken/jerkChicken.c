/**
 * @file jerkChicken.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A mode about jerking the chicken. No, not like the food. Not that way either.
 * @version 0.1
 * @date 2025-06-03
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "jerkChicken.h"

//==============================================================================
// Defines
//==============================================================================

// Settings
#define JERK_VALUE 1024
#define STEP_SIZE  32

// Pixel offsets
#define PLAYER_X_POS     32
#define PLAYER_Y_POS     64
#define CHICKEN_X_OFFSET 200
#define CHICKEN_Y_POS    160

//==============================================================================
// Consts
//==============================================================================

const char chickenModeName[] = "Jerk Chicken";

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CHICKEN_STATIC,        // Chicken standing still
    CHICKEN_WALK_FORWARD,  // Chicken walking toward player
    CHICKEN_WALK_BACKWARD, // Chicken walking away from player
    CHICKEN_LUNGE_PREPARE, // Chicken preparing to launch itself forward
    CHICKEN_LUNGE,         // Chicken charging forward
    CHICKEN_PECK,          // Chicken pecking at the ground
} chickenStates_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int16_t xComp, yComp; // Tilt components
    bool jerked;          // If jerk was caused
    bool playerAnimating; // If player is already taking an action
} player_t;

typedef struct
{
    int32_t position; // X position in the game
    chickenStates_t state;
} chicken_t;

typedef struct
{
    // Player
    player_t player;

    // Chicken
    chicken_t chicken;

    // Level
    int32_t position;        // X position of the world
    //int32_t targetPosition;  // Target X position
} chickenData_t;

//==============================================================================
// Function Declarations
//==============================================================================

// SwadgeMode functions
static void enterChicken(void);
static void exitChicken(void);
static void chickenLoop(int64_t elapsedUs);

// Game logic
static void initGame(void);

// Draw routines
static void drawChicken(int64_t elapsedUs);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t chickenMode = {
    .modeName          = chickenModeName,
    .fnEnterMode       = enterChicken,
    .fnExitMode        = exitChicken,
    .fnMainLoop        = chickenLoop,
    .wifiMode          = NO_WIFI,
    .usesAccelerometer = true,
};

chickenData_t* cd;

//==============================================================================
// Functions
//==============================================================================

static void enterChicken(void)
{
    cd = (chickenData_t*)heap_caps_calloc(1, sizeof(chickenData_t), MALLOC_CAP_8BIT);

    initGame();
}

static void exitChicken(void)
{
    heap_caps_free(cd);
}

static void chickenLoop(int64_t elapsedUs)
{
    // Input
    accelIntegrate();
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && !cd->player.playerAnimating)
        {
            if (evt.button & PB_LEFT)
            {
                // Step backward
                cd->position += STEP_SIZE;
            }
            else if (evt.button & PB_RIGHT)
            {
                // Step forward
                cd->position -= STEP_SIZE;
            }
            else if (evt.button & PB_UP)
            {
                // Jerk rod
                cd->player.jerked = true;
                cd->position += STEP_SIZE * 2;
            }
        }
    }
    int16_t prevX = cd->player.xComp;
    if (ESP_OK == accelGetSteeringAngleDegrees(&cd->player.xComp, &cd->player.yComp))
    {
        if (prevX > cd->player.xComp + JERK_VALUE)
        {
            cd->player.jerked = true;
            cd->position += STEP_SIZE * 2;
        }
    }

    // Logic
    cd->chicken.position++;

    // Draw
    drawChicken(elapsedUs);
}

static void initGame(void)
{
    cd->position = 0;
}

static void drawChicken(int64_t elapsedUs)
{
    clearPxTft();

    // Level
    // Tickmarks
    int16_t startPos = TFT_WIDTH + cd->position;
    for (int i = cd->position; i < startPos; i++)
    {
        int linePos = cd->position + i;
        int screenSpaceX = startPos - i;
        if (linePos % 100 == 0)
        {
            drawLineFast(screenSpaceX, TFT_HEIGHT, screenSpaceX, TFT_HEIGHT - 32, c555);
            char buffer[8];
            snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, linePos);
            drawText(getSysFont(), c555, buffer, screenSpaceX - (2 + textWidth(getSysFont(), buffer)), TFT_HEIGHT - 32);
        }
        else if (linePos % 10 == 0)
        {
            drawLineFast(screenSpaceX, TFT_HEIGHT, screenSpaceX, TFT_HEIGHT - 16, c555);
        }
    }

    // Player
    drawRect(PLAYER_X_POS, PLAYER_Y_POS, PLAYER_X_POS + 64, PLAYER_Y_POS + 128, c050);

    // Chicken
    int16_t chickenXOffset = -cd->chicken.position + CHICKEN_X_OFFSET + cd->position;
    drawRect(chickenXOffset, CHICKEN_Y_POS, chickenXOffset + 32, CHICKEN_Y_POS + 32, c500);
}