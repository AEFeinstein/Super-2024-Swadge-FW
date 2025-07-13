/**
 * @file jerkChicken.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A mode about jerking the chicken. No, not like the food. Not that way either.
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
#define JERK_VALUE           1024
#define STEP_SIZE            32
#define PLAYER_SPEED         3
#define CHICKEN_SPEED        2
#define BOX_MAX_DISTANCE     40
#define CHICKEN_ATTACK_RANGE 80
#define CHICKEN_OFFSCREEN    150
// Timers
#define ANIM_TIMER_PERIOD   16667  // Assumes 60Hz
#define CHICKEN_STATIC_BASE 500000 // 1/2 sec
// Pixel offsets
#define PLAYER_X_POS      32
#define PLAYER_Y_POS      64
#define CHICKEN_X_OFFSET  200
#define CHICKEN_Y_POS     160
#define BAIT_BOX_X_OFFSET 108
#define BAIT_BOX_Y_POS    168
// Strip data
#define STRIP_COUNT  8
#define STRIP_VCOUNT 12
#define STRIP_WIDTH  40
#define STRIP_HEIGHT 16

//==============================================================================
// Consts
//==============================================================================

const char chickenModeName[] = "Jerk Chicken";

static const char* const strings[] = {
    "Press any button to play!",
};

static const cnfsFileIdx_t tiles[] = {
    LEAVES_WSG,
    TREE_TOP_WSG,
    TREE_TRUNK_WSG,
    TREE_BOTTOM_WSG,
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CHICKEN_SPLASH,
    CHICKEN_PREP,
    CHICKEN_GAME,
    CHICKEN_LOSE,
    CHICKEN_SCORE,
} gameState_t;

typedef enum
{
    CHK_EASY,
    CHK_MED,
    CHK_HARD,
} chickenDifficulty_t;

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

typedef enum
{
    FOREST,
    CITY,
    FARM,
    MOUNTAIN,
} stripTypes_t;

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
    int diffMod;           // The current difficulty modifier (50 is easiest, 0 is hardest)

    // Movement
    int8_t moveSpeed;  // Current move speed
    int32_t position;  // X position in the game
    int32_t targetPos; // Target position for the chicken
    int64_t animTimer; // Chicken animation timer
    int8_t animFrame;  // Current animation frame
} chicken_t;

typedef struct
{
    // Box
    int8_t HP;

    // Movement
    int8_t speed;
    int32_t position;
    int32_t targetPos;
} baitBox_t;

typedef struct
{
    int8_t imgIdxs[STRIP_VCOUNT];
    int16_t position;
    stripTypes_t strip;
} bgStrip_t;

typedef struct
{
    stripTypes_t newStrip;
    int32_t startPos;
} bgStripOrder_t;

typedef struct
{
    // Player
    player_t player;

    // Chicken
    chicken_t chicken;
    wsg_t chck;

    // Bait Box
    baitBox_t box;

    // Mode
    int score;                // Score
    gameState_t state;        // Splash/Game/End
    chickenDifficulty_t diff; // Selected difficulty
    bool accessible;          // If button Jerk is enabled or not

    // Background scroll
    bgStrip_t strips[STRIP_COUNT];
    wsg_t block;
    wsg_t* bgTiles;

    // Splash
    int64_t animTimer;
    bool textToggle;

    // Debug
    bool debug; // Displays debug info
} chickenData_t;

//==============================================================================
// Map Order
//==============================================================================

static const bgStripOrder_t order[] = {
    {.newStrip = FOREST, .startPos = 0},
    {.newStrip = MOUNTAIN, .startPos = 300},
};

//==============================================================================
// Function Declarations
//==============================================================================

// SwadgeMode functions
static void enterChicken(void);
static void exitChicken(void);
static void chickenLoop(int64_t elapsedUs);

// Game logic
static void initGame(chickenDifficulty_t diff);
static void jerkRod(void);
static void chickenHandleInputs(void);
static bool movePos(int32_t* position, int32_t* target, int8_t speed);
static void increaseDifficulty(void);
static void chickenLogic(int64_t elapsedUs);

// Draw
// Main
static void drawChickenSplash(int64_t elapsedUs);
static void drawChickenPrep(void);
static void drawChicken(int64_t elapsedUs);
static void drawChickenLose(int64_t elapsedUs);
static void drawChickenScore(void);
// Sub
static void drawBG(void);
static void setStripIdxs(bgStrip_t* strip);
static void drawTape(int xPos);

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

    cd->state = CHICKEN_SPLASH;

    // Test code
    loadWsg(CHK_RUN_1_WSG, &cd->chck, true);
    cd->bgTiles = heap_caps_calloc(ARRAY_SIZE(tiles), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int i = 0; i < ARRAY_SIZE(tiles); i++)
    {
        loadWsg(tiles[i], &cd->bgTiles[i], true);
    }
    loadWsg(BLOCK_WSG, &cd->block, true);
    cd->debug = true;
}

static void exitChicken()
{
    freeWsg(&cd->block);
    for (int i = 0; i < ARRAY_SIZE(tiles); i++)
    {
        freeWsg(&cd->bgTiles[i]);
    }
    free(cd->bgTiles);
    freeWsg(&cd->chck);
    heap_caps_free(cd);
}

static void chickenLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    switch (cd->state)
    {
        case CHICKEN_SPLASH:
        {
            // Accept any input to change state
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    cd->state = CHICKEN_PREP;
                }
            }
            // Draw splash screen
            drawChickenSplash(elapsedUs);
            break;
        }
        case CHICKEN_PREP:
        {
            // Handle menu for preparing for game
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    // TODO: Provide player with options
                    // - Bait image (unlocked by trophy)
                    // - Difficulty
                    //  - Easy: Chicken waits longer, Score is slower, Difficulty mod starts higher
                    //  - Medium: Default
                    //  - Hard: Chicken stops being patient, Score increments faster, Difficulty mod goes away faster
                    // - Accessibility mode: Up key jerks the rod
                    cd->state = CHICKEN_GAME;
                    initGame(cd->diff);
                }
            }
            // Draw menu
            drawChickenPrep();
            break;
        }
        case CHICKEN_GAME:
        default:
        {
            // Input
            chickenHandleInputs();
            // Logic
            chickenLogic(elapsedUs);
            // Draw
            drawChicken(elapsedUs);
            break;
        }
        case CHICKEN_LOSE:
        {
            // Accept any input to change state
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    cd->state = CHICKEN_SCORE;
                }
            }
            // Draw losing state
            drawChickenLose(elapsedUs);
            break;
        }
        case CHICKEN_SCORE:
        {
            // Accept any input to change state
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    cd->state = CHICKEN_PREP;
                }
            }
            // Draw High scores
            drawChickenScore();
            break;
        }
    }
}

static void initGame(chickenDifficulty_t diff)
{
    cd->player.position  = 0;
    cd->chicken.position = 0;
    cd->box.position     = 0;
    cd->score            = 0;

    switch (diff)
    {
        case CHK_EASY:
        {
            cd->box.HP          = 5;
            cd->chicken.diffMod = 100;
            break;
        }
        case CHK_MED:
        {
            cd->box.HP          = 3;
            cd->chicken.diffMod = 50;
            break;
        }
        case CHK_HARD:
        {
            cd->box.HP          = 1;
            cd->chicken.diffMod = 25;
            break;
        }
    }

    for (int idx = 0; idx < STRIP_COUNT; idx++)
    {
        cd->strips[idx].position = idx * STRIP_WIDTH;
        cd->strips[idx].strip    = FOREST;
    }
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

static void increaseDifficulty()
{
    if (cd->chicken.diffMod > 0)
    {
        cd->chicken.diffMod--;
    }
}

static void chickenLogic(int64_t elapsedUs)
{
    // Handle chicken brain
    switch (cd->chicken.state)
    {
        case CHICKEN_LUNGE_PREPARE:
        {
            cd->chicken.animTimer -= elapsedUs;
            if (cd->chicken.animTimer <= 0)
            {
                cd->chicken.state = CHICKEN_LUNGE;
            }
            break;
        }
        case CHICKEN_WALKING:
        case CHICKEN_LUNGE:
        {
            RUN_TIMER_EVERY(
                cd->chicken.animTimer, ANIM_TIMER_PERIOD, elapsedUs,
                if (movePos(&cd->chicken.position, &cd->chicken.targetPos, cd->chicken.moveSpeed)) {
                    cd->chicken.state = CHICKEN_IDLE;
                });
            if (cd->chicken.state == CHICKEN_IDLE)
            {
                cd->chicken.animTimer = CHICKEN_STATIC_BASE;
            }
            break;
        }
        case CHICKEN_IDLE:
        case CHICKEN_PECK:
        {
            cd->chicken.animTimer -= elapsedUs;
            if (cd->chicken.animTimer <= 0)
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

            // If close enough to the box, peck at it and damage box
            if (cd->chicken.position > cd->box.position + CHICKEN_ATTACK_RANGE
                && cd->chicken.position < cd->box.position + CHICKEN_ATTACK_RANGE + 24)
            {
                cd->chicken.state     = CHICKEN_PECK;
                cd->chicken.animTimer = CHICKEN_STATIC_BASE * 2;
                cd->box.HP--;
                break;
            }
            // If past box, always go backwards
            else if (cd->chicken.position > cd->box.position + CHICKEN_ATTACK_RANGE + 24)
            {
                cd->chicken.moveSpeed = CHICKEN_SPEED;
                cd->chicken.targetPos = cd->box.position + CHICKEN_ATTACK_RANGE + 12;
                cd->chicken.state     = CHICKEN_WALKING;
                break;
            }
            // Chance to lunge if in range
            int choiceIdx = esp_random() % (50 + cd->chicken.diffMod);
            if (choiceIdx < 25 && cd->chicken.position > cd->box.position + CHICKEN_ATTACK_RANGE - 48)
            {
                cd->chicken.moveSpeed = CHICKEN_SPEED * 2;
                cd->chicken.targetPos = cd->chicken.position + 32 + (esp_random() % 48);
                cd->chicken.state     = CHICKEN_LUNGE_PREPARE;
                cd->chicken.animTimer = CHICKEN_STATIC_BASE * 2;
                increaseDifficulty();
                break;
            }
            // More likely to walk forward if box is closer
            choiceIdx = esp_random() % (50 + cd->chicken.diffMod);
            if (choiceIdx < 10 + (cd->chicken.position - cd->box.position))
            {
                cd->chicken.moveSpeed = CHICKEN_SPEED;
                cd->chicken.targetPos = cd->chicken.position + 16 + (esp_random() % 32);
                cd->chicken.state     = CHICKEN_WALKING;
                break;
            }

            // Move backwards if too far away
            choiceIdx = esp_random() % (50 + cd->chicken.diffMod);
            if (choiceIdx < 10 + (cd->box.position - cd->chicken.position))
            {
                cd->chicken.moveSpeed = CHICKEN_SPEED;
                cd->chicken.targetPos = cd->chicken.position - (16 + (esp_random() % 32));
                cd->chicken.state     = CHICKEN_WALKING;
                break;
            }

            // Randomly peck or stand still
            choiceIdx = esp_random() % 100;
            if (choiceIdx < 50)
            {
                cd->chicken.state     = CHICKEN_PECK;
                cd->chicken.animTimer = CHICKEN_STATIC_BASE * 2;
                break;
            }
            else
            {
                cd->chicken.state     = CHICKEN_IDLE;
                cd->chicken.animTimer = CHICKEN_STATIC_BASE * 2 + (esp_random() % CHICKEN_STATIC_BASE);
            }
            break;
        }
    }

    // Player
    // Move camera
    RUN_TIMER_EVERY(cd->player.animTimer, ANIM_TIMER_PERIOD, elapsedUs,
                    movePos(&cd->player.position, &cd->player.targetPosition, cd->player.moveSpeed););

    // Move box
    if (cd->box.position + BOX_MAX_DISTANCE < cd->player.position)
    {
        cd->box.position = cd->player.position - BOX_MAX_DISTANCE;
    }
    else if (cd->box.position - BOX_MAX_DISTANCE > cd->player.position)
    {
        cd->box.position = cd->player.position + BOX_MAX_DISTANCE;
    }

    // Update score
    if (cd->chicken.position > cd->score)
    {
        cd->score = cd->chicken.position;
    }

    // Check if game is lost
    if (cd->box.HP <= 0 || cd->chicken.position - cd->player.position <= -CHICKEN_OFFSCREEN)
    {
        cd->state = CHICKEN_LOSE;
    }
}

// Draw
// Main routines
static void drawChickenSplash(int64_t elapsedUs)
{
    clearPxTft();
    // TODO: Draw a background
    // Draw text
    RUN_TIMER_EVERY(cd->animTimer, 500000, elapsedUs, cd->textToggle = !cd->textToggle;);
    if (cd->textToggle)
    {
        drawText(getSysFont(), c555, strings[0], (TFT_WIDTH - textWidth(getSysFont(), strings[0])) >> 1, 200);
    }
}

static void drawChickenPrep()
{
}

static void drawChicken(int64_t elapsedUs)
{
    clearPxTft();

    // Level
    drawBG();
    // Tickmarks
    drawTape(cd->player.position);

    // Player

    // Box

    // Chicken

    // UI

    // Test
    if (cd->debug)
    {
        // Player
        drawRect(PLAYER_X_POS, PLAYER_Y_POS, PLAYER_X_POS + 64, PLAYER_Y_POS + 128, c050);

        // Box
        int16_t boxXOffset = -cd->box.position + BAIT_BOX_X_OFFSET + cd->player.position;
        drawRect(boxXOffset, BAIT_BOX_Y_POS, boxXOffset + 24, BAIT_BOX_Y_POS + 24, c055);

        // Chicken
        int16_t chickenXOffset = -cd->chicken.position + CHICKEN_X_OFFSET + cd->player.position;
        drawRect(chickenXOffset, CHICKEN_Y_POS, chickenXOffset + 32, CHICKEN_Y_POS + 32, c500);
        drawWsg(&cd->chck, chickenXOffset, CHICKEN_Y_POS, false, false, 0);

        char buffer[32];
        snprintf(buffer, sizeof(buffer) - 1, "State: %" PRId16, cd->chicken.state);
        drawText(getSysFont(), c550, buffer, chickenXOffset, CHICKEN_Y_POS - 16);

        // UI
        snprintf(buffer, sizeof(buffer) - 1, "Score: %" PRId8, cd->score);
        drawText(getSysFont(), c550, buffer, 32, 16);
        snprintf(buffer, sizeof(buffer) - 1, "HP: %" PRId8, cd->box.HP);
        drawText(getSysFont(), c550, buffer, 32, 32);
        snprintf(buffer, sizeof(buffer) - 1, "Difficulty Mod: %" PRId8, cd->chicken.diffMod);
        drawText(getSysFont(), c550, buffer, 32, 48);
    }
}

static void drawChickenLose(int64_t elapsedUs)
{
    // Draw chicken wandering around and pecking, player sitting down, defeated
    drawText(getSysFont(), c555, "Game over", 100, 100);
}

static void drawChickenScore()
{
    clearPxTft();
    // Draw scoreboard
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Score: %" PRId8, cd->score);
    drawText(getSysFont(), c550, buffer, 32, 16);
}

// Subroutines
static void drawBG()
{
    int startPos = cd->player.position + TFT_WIDTH;
    //  Draw
    for (int i = 0; i < STRIP_COUNT; i++)
    {
        // Reposition loops
        if (cd->strips[i].position - cd->player.position < -40)
        {
            int newPos = cd->strips[i].position + STRIP_WIDTH * STRIP_COUNT;
            for (int idx = 0; idx < ARRAY_SIZE(order); idx++)
            {
                if (newPos < order[idx].startPos)
                {
                    break;
                }
                else if (newPos >= order[idx].startPos && cd->strips[i].position < order[idx].startPos)
                {
                    if (cd->strips[i].strip != order[idx].newStrip)
                    {
                        // TODO: Assign a transition
                    }
                }
                else
                {
                    // Assign the main strip
                    // FIXME: Assigns over and over again.
                    cd->strips[i].strip = order[idx].newStrip;
                }
            }

            cd->strips[i].position += STRIP_WIDTH * STRIP_COUNT;

            // TODO: Set the stripindex
        }
        else if (cd->strips[i].position > startPos)
        {
            cd->strips[i].position -= STRIP_WIDTH * STRIP_COUNT;
            // TODO: Set the stripindex
        }

        setStripIdxs(&cd->strips[i]);

        // Draw

        for (int j = 0; j < STRIP_VCOUNT; j++)
        {
            if (cd->strips[i].strip == FOREST)
            {
                drawWsgSimple(&cd->bgTiles[cd->strips[i].imgIdxs[j]],
                              cd->player.position + (TFT_WIDTH - STRIP_WIDTH) - cd->strips[i].position,
                              j * STRIP_HEIGHT);
            }
            else
            {
                // FIXME: Only here until all tile sets are in.
                drawWsgSimple(&cd->block, cd->player.position + (TFT_WIDTH - STRIP_WIDTH) - cd->strips[i].position,
                              j * STRIP_HEIGHT);
            }
        }
    }
}

static void setStripIdxs(bgStrip_t* strip)
{
    switch (strip->strip)
    {
        case FOREST:
        {
            strip->imgIdxs[0]  = 0;
            strip->imgIdxs[1]  = 0;
            strip->imgIdxs[2]  = 0;
            strip->imgIdxs[3]  = 0;
            strip->imgIdxs[4]  = 0;
            strip->imgIdxs[5]  = 0;
            strip->imgIdxs[6]  = 1;
            strip->imgIdxs[7]  = 2;
            strip->imgIdxs[8]  = 2;
            strip->imgIdxs[9]  = 2;
            strip->imgIdxs[10] = 2;
            strip->imgIdxs[11] = 3;
            break;
        }
        default:
        {
            break;
        }
    }
}

static void drawTape(int xPos)
{
    fillDisplayArea(0, TFT_HEIGHT - 32, TFT_WIDTH, TFT_HEIGHT, c550);
    drawLineFast(0, TFT_HEIGHT - 1, TFT_WIDTH, TFT_HEIGHT - 1, c330);
    drawLineFast(0, TFT_HEIGHT - 32, TFT_WIDTH, TFT_HEIGHT - 32, c555);
    int32_t startPos = TFT_WIDTH + xPos;
    for (int i = xPos; i < startPos; i++)
    {
        int screenSpaceX = startPos - i;
        if (i % 100 == 0)
        {
            drawLineFast(screenSpaceX, TFT_HEIGHT, screenSpaceX, TFT_HEIGHT - 32, c000);
            char buffer[8];
            snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, i / 100);
            drawText(getSysFont(), c000, buffer, screenSpaceX - (2 + textWidth(getSysFont(), buffer)), TFT_HEIGHT - 30);
        }
        else if (i % 10 == 0)
        {
            drawLineFast(screenSpaceX, TFT_HEIGHT, screenSpaceX, TFT_HEIGHT - 16, c000);
        }
    }
}