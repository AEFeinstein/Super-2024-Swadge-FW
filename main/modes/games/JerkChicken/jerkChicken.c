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
#define JERK_VALUE   1024
#define STEP_SIZE    32
#define SCROLL_SPEED 3

// Timers
#define ANIM_TIMER_PERIOD 16667

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
    CHICKEN_THINKING,      // Chicken is ready to change behavior
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
    int32_t position;       // X position of the world
    int32_t targetPosition; // Target X position
    int64_t animTimer;      // How many US since last update
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
static void jerkRod(void);
static void chickenHandleInputs(void);
static void chickenLogic(int64_t elapsedUs);

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

static void enterChicken()
{
    cd = (chickenData_t*)heap_caps_calloc(1, sizeof(chickenData_t), MALLOC_CAP_8BIT);

    initGame();
}

static void exitChicken()
{
    heap_caps_free(cd);
}

static void chickenLoop(int64_t elapsedUs)
{
    // Input
    chickenHandleInputs();

    // Logic
    chickenLogic(elapsedUs);

    // Draw
    drawChicken(elapsedUs);
}

static void initGame()
{
    cd->position = 0;
}

static void jerkRod()
{
    cd->player.jerked = true;
    cd->targetPosition += STEP_SIZE * 2;
}

static void chickenHandleInputs()
{
    accelIntegrate();
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && cd->position == cd->targetPosition) // Only eval if the player has come to a erst
        {
            if (evt.button & PB_LEFT)
            {
                // Step backward
                cd->targetPosition += STEP_SIZE;
            }
            else if (evt.button & PB_RIGHT)
            {
                // Step forward
                cd->targetPosition -= STEP_SIZE;
            }
            else if (evt.button & PB_UP)
            {
                // Jerk rod
                jerkRod();
            }
        }
    }
    int16_t prevX = cd->player.xComp;
    if (ESP_OK == accelGetSteeringAngleDegrees(&cd->player.xComp, &cd->player.yComp)
        && cd->position == cd->targetPosition)
    {
        if (prevX > cd->player.xComp + JERK_VALUE && !cd->player.jerked)
        {
            jerkRod();
        }
    }
}

static void scrollScreen()
{
    if (cd->position < cd->targetPosition)
    {
        cd->position += SCROLL_SPEED;
        if (cd->position > cd->targetPosition)
        {
            cd->position = cd->targetPosition;
        }
    }
    else if (cd->position > cd->targetPosition)
    {
        cd->position -= SCROLL_SPEED;
        if (cd->position < cd->targetPosition)
        {
            cd->position = cd->targetPosition;
        }
    }
}

static void chickenLogic(int64_t elapsedUs)
{
    // TODO: Handle chicken brain

    // Move camera
    // Timer
    RUN_TIMER_EVERY(cd->animTimer, ANIM_TIMER_PERIOD, elapsedUs, scrollScreen(););

    // TODO: evaluate lose conditions
}

static void drawChicken(int64_t elapsedUs)
{
    clearPxTft();

    // Level
    // Tickmarks
    int32_t startPos = TFT_WIDTH + cd->position;
    for (int i = cd->position; i < startPos; i++)
    {
        int screenSpaceX = startPos - i;
        if (i % 100 == 0)
        {
            drawLineFast(screenSpaceX, TFT_HEIGHT, screenSpaceX, TFT_HEIGHT - 32, c555);
            char buffer[8];
            snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, i / 100);
            drawText(getSysFont(), c555, buffer, screenSpaceX - (2 + textWidth(getSysFont(), buffer)), TFT_HEIGHT - 32);
        }
        else if (i % 10 == 0)
        {
            drawLineFast(screenSpaceX, TFT_HEIGHT, screenSpaceX, TFT_HEIGHT - 16, c555);
        }
        int g = 0;
    }

    // Player
    drawRect(PLAYER_X_POS, PLAYER_Y_POS, PLAYER_X_POS + 64, PLAYER_Y_POS + 128, c050);

    // Chicken
    int16_t chickenXOffset = -cd->chicken.position + CHICKEN_X_OFFSET + cd->position;
    drawRect(chickenXOffset, CHICKEN_Y_POS, chickenXOffset + 32, CHICKEN_Y_POS + 32, c500);

    // Test
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Curr pos: %" PRId32, cd->position);
    drawText(getSysFont(), c550, buffer, 32, 32);
}