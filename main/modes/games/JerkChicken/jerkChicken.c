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
#include <esp_random.h>

//==============================================================================
// Defines
//==============================================================================

// Settings
#define JERK_VALUE    1024
#define STEP_SIZE     32
#define PLAYER_SPEED  3
#define CHICKEN_SPEED 2

// Chicken brain chances
#define CHANCE_WALK      10
#define CHANCE_WALK_BACK 5
#define CHANCE_LUNGE     10
#define CHANCE_PECK      35

// Timers
#define ANIM_TIMER_PERIOD   16667  // Assumes 60Hz
#define CHICKEN_STATIC_BASE 250000 // 1/4 sec

// Pixel offsets
#define PLAYER_X_POS     32
#define PLAYER_Y_POS     64
#define CHICKEN_X_OFFSET 200
#define CHICKEN_Y_POS    160
// #define BAIT_BOX_Y_POS   168

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
    CHICKEN_IDLE,          // Chicken standing still
    CHICKEN_WALKING,       // Chicken walking toward player
    CHICKEN_LUNGE_PREPARE, // Chicken preparing to launch itself forward
    CHICKEN_LUNGE,         // Chicken charging forward
    CHICKEN_PECK,          // Chicken pecking at the ground
    CHICKEN_NUM_STATES     // Number of states the chicken may exist in. No quantum super-positioning.
} chickenStates_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Input
    int16_t xComp, yComp; // Tilt components
    bool jerked;          // If jerk was caused

    // Movement
    int8_t moveSpeed;       // How fast player moves
    int32_t position;       // X position of the world
    int32_t targetPosition; // Target X position
    int64_t animTimer;      // How many US since last update
} player_t;

typedef struct
{
    // Chicken brain
    chickenStates_t state; // Current chicken brain

    // Movement
    int8_t moveSpeed;  // Current move speed
    int32_t position;  // X position in the game
    int32_t targetPos; // Target position for the chicken
    int64_t animTimer; // Chicken animation timer
    int8_t animFrame;  // Current animation frame
} chicken_t;

typedef struct
{
    // Movement
    int8_t speed;
    int32_t position;
    int32_t targetPos;
} baitBox_t;

typedef struct
{
    // Player
    player_t player;

    // Chicken
    chicken_t chicken;

    // Debug
    bool debug; // Displays debug info
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
static bool movePos(int32_t* position, int32_t* target, int8_t speed);
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

    cd->debug = true;

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
    cd->player.position  = 0;
    cd->chicken.position = 0;
}

static void jerkRod()
{
    cd->player.jerked = true;
    cd->player.targetPosition += STEP_SIZE * 2;
    cd->player.moveSpeed = PLAYER_SPEED * 2;
}

static void chickenHandleInputs()
{
    accelIntegrate();
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && cd->player.position == cd->player.targetPosition) // Only eval if the player has come to a erst
        {
            if (evt.button & PB_LEFT)
            {
                // Step backward
                cd->player.targetPosition += STEP_SIZE;
                cd->player.moveSpeed = PLAYER_SPEED;
            }
            else if (evt.button & PB_RIGHT)
            {
                // Step forward
                cd->player.targetPosition -= STEP_SIZE;
                cd->player.moveSpeed = PLAYER_SPEED;
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
        && cd->player.position == cd->player.targetPosition)
    {
        if (prevX > cd->player.xComp + JERK_VALUE && !cd->player.jerked)
        {
            jerkRod();
        }
    }
}

static bool movePos(int32_t* position, int32_t* target, int8_t speed)
{
    if (*position == *target)
    {
        return true;
    }
    if (*position < *target)
    {
        *position += speed;
        if (*position > *target)
        {
            *position = *target;
            return true;
        }
    }
    else if (*position > *target)
    {
        *position -= speed;
        if (*position < *target)
        {
            *position = *target;
            return true;
        }
    }
    return false;
}

static void chickenLogic(int64_t elapsedUs)
{
    // Handle chicken brain
    switch (cd->chicken.state)
    {
        case CHICKEN_LUNGE_PREPARE:
        {
            cd->chicken.animTimer -= elapsedUs;
            if(cd->chicken.animTimer <= 0)
            {
                cd->chicken.state = CHICKEN_LUNGE;
            }
            break;
        }
        case CHICKEN_WALKING:
        case CHICKEN_LUNGE:
        {
            RUN_TIMER_EVERY(cd->chicken.animTimer, ANIM_TIMER_PERIOD, elapsedUs,
                            cd->chicken.state
                            = movePos(&cd->chicken.position, &cd->chicken.targetPos, cd->chicken.moveSpeed)
                                  ? CHICKEN_THINKING
                                  : CHICKEN_WALKING;);
            break;
        }
        case CHICKEN_IDLE:
        case CHICKEN_PECK:
        {
            cd->chicken.animTimer -= elapsedUs;
            if(cd->chicken.animTimer <= 0)
            {
                cd->chicken.state = CHICKEN_THINKING;
            }
            break;
        }
        case CHICKEN_THINKING:
        default:
        {
            // Decide next action in a sane manner
            cd->chicken.animFrame = 0; // Reset regardless to avoid bugs
            int choiceIdx         = esp_random() % 100;
            if (choiceIdx < CHANCE_WALK)
            {
                cd->chicken.moveSpeed = CHICKEN_SPEED;
                cd->chicken.targetPos = cd->chicken.position + 16 + (esp_random() % 32);
                cd->chicken.state     = CHICKEN_WALKING;
            }
            else if (choiceIdx < CHANCE_WALK + CHANCE_WALK_BACK)
            {
                cd->chicken.moveSpeed = CHICKEN_SPEED;
                cd->chicken.targetPos = cd->chicken.position - (16 + (esp_random() % 32));
                cd->chicken.state     = CHICKEN_WALKING;
            }
            else if (choiceIdx < CHANCE_WALK + CHANCE_WALK_BACK + CHANCE_LUNGE)
            {
                cd->chicken.moveSpeed = CHICKEN_SPEED * 2;
                cd->chicken.targetPos = cd->chicken.position + 32 + (esp_random() % 48);
                cd->chicken.state     = CHICKEN_LUNGE_PREPARE;
                cd->chicken.animTimer = CHICKEN_STATIC_BASE;
            }
            else if (choiceIdx < CHANCE_WALK + CHANCE_WALK_BACK + CHANCE_LUNGE + CHANCE_PECK)
            {
                cd->chicken.state = CHICKEN_PECK;
                cd->chicken.animTimer = CHICKEN_STATIC_BASE * 2; // 1/2 sec
            }
            else
            {
                cd->chicken.state = CHICKEN_IDLE;
                cd->chicken.animTimer = CHICKEN_STATIC_BASE * 2 + (esp_random() % CHICKEN_STATIC_BASE); // 1/2 - 3/4 sec
            }
            break;
        }
    }

    // Player
    // Move camera
    RUN_TIMER_EVERY(cd->player.animTimer, ANIM_TIMER_PERIOD, elapsedUs,
                    movePos(&cd->player.position, &cd->player.targetPosition, cd->player.moveSpeed););

    // TODO: evaluate lose conditions
}

static void drawChicken(int64_t elapsedUs)
{
    clearPxTft();

    // Level
    // Tickmarks
    int32_t startPos = TFT_WIDTH + cd->player.position;
    for (int i = cd->player.position; i < startPos; i++)
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
    }

    // Player
    drawRect(PLAYER_X_POS, PLAYER_Y_POS, PLAYER_X_POS + 64, PLAYER_Y_POS + 128, c050);

    // Chicken
    int16_t chickenXOffset = -cd->chicken.position + CHICKEN_X_OFFSET + cd->player.position;
    drawRect(chickenXOffset, CHICKEN_Y_POS, chickenXOffset + 32, CHICKEN_Y_POS + 32, c500);

    // UI

    // Test
    if (cd->debug)
    {
        char buffer[32];
        snprintf(buffer, sizeof(buffer) - 1, "State: %" PRId16, cd->chicken.state);
        drawText(getSysFont(), c550, buffer, chickenXOffset, CHICKEN_Y_POS - 16);
    }
}