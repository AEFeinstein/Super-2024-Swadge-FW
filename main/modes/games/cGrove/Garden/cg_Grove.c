/**
 * @file cg_Garden.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief The main interation area with the Chowa
 * @version 0.1
 * @date 2024-09-07
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "cg_Grove.h"
#include "cg_GroveAI.h"
#include "cg_GroveDraw.h"
#include "cg_Items.h"
#include <esp_random.h>

//==============================================================================
// Defines
//==============================================================================

#define CG_CURSOR_SPEED 16

//==============================================================================
// Consts
//==============================================================================

static const char* groveCursorSprites[] = {
    "chowa_hand1.wsg",
    "chowa_hand2.wsg",
    "chowa_hand3.wsg",
};

static const char* questionMarkSprites[] = {
    "questmk-1.wsg", "questmk-2.wsg", "questmk-3.wsg", "questmk-4.wsg", "questmk-5.wsg", "questmk-6.wsg",
};

static const char* angerParticles[] = {
    "anger-1.wsg",
    "anger-2.wsg",
};

//==============================================================================
// Function Declarations
//==============================================================================

/**
 * @brief Attempts to grab objects. Due to total amount being limited, no need to optimize
 *
 * @param cg Game Object
 */
static void cg_attemptGrab(cGrove_t* cg);

/**
 * @brief Input handling for garden
 *
 * @param cg Game Object
 */
static void cg_handleInputGarden(cGrove_t* cg);

/**
 * @brief Moves the view of the field byt eh provided x and y
 *
 * @param cg Game Object
 * @param xChange Distance to move horizontally. Negative is right, positive if left.
 * @param yChange Distance to move vertically. Negative is up, positive s down
 */
static void cg_moveCamera(cGrove_t* cg, int16_t xChange, int16_t yChange);

/**
 * @brief Initialize object boundaries
 *
 * @param cg Game Data
 */
static void cg_setupBorders(cGrove_t* cg);

static void cg_bgmCB(void);

//==============================================================================
// Variables
//==============================================================================

bool isBGMPlaying;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the Garden mode
 *
 * @param cg Game Object
 */
void cg_initGrove(cGrove_t* cg)
{
    // Load assets
    // WSGs
    loadWsg("garden_background.wsg", &cg->grove.groveBG, true);
    // Cursors
    cg->grove.cursors = calloc(ARRAY_SIZE(groveCursorSprites), sizeof(wsg_t));
    for (int32_t idx = 0; idx < ARRAY_SIZE(groveCursorSprites); idx++)
    {
        loadWsg(groveCursorSprites[idx], &cg->grove.cursors[idx], true);
    }
    // Emotes
    cg->grove.angerParticles = calloc(ARRAY_SIZE(angerParticles), sizeof(wsg_t));
    for (int32_t idx = 0; idx < ARRAY_SIZE(angerParticles); idx++)
    {
        loadWsg(angerParticles[idx], &cg->grove.angerParticles[idx], true);
    }
    cg->grove.questionMarks = calloc(ARRAY_SIZE(questionMarkSprites), sizeof(wsg_t));
    for (int32_t idx = 0; idx < ARRAY_SIZE(questionMarkSprites); idx++)
    {
        loadWsg(questionMarkSprites[idx], &cg->grove.questionMarks[idx], true);
    }
    // Audio
    loadMidiFile("Chowa_Grove_Meadow.mid", &cg->grove.bgm, true);

    // Initialize viewport
    cg->grove.camera.height = TFT_HEIGHT; // Used to check what objects should be drawn
    cg->grove.camera.width  = TFT_WIDTH;
    cg->grove.camera.pos.x  = (cg->grove.groveBG.w - TFT_HEIGHT) >> 1;
    cg->grove.camera.pos.y  = (cg->grove.groveBG.h - TFT_WIDTH) >> 1;

    // Initialize the cursor
    cg->grove.cursor.height = cg->grove.cursors[0].h;
    cg->grove.cursor.width  = cg->grove.cursors[0].w;
    cg->grove.cursor.pos.x  = (TFT_WIDTH - cg->grove.cursors[0].w) >> 1;
    cg->grove.cursor.pos.y  = (TFT_HEIGHT - cg->grove.cursors[0].h) >> 1;
    cg->grove.holdingChowa  = false;
    cg->grove.holdingItem   = false;

    // Setup boundaries
    cg_setupBorders(cg);

    // Initialize Chowa
    for (int32_t i = 0; i < CG_MAX_CHOWA; i++)
    {
        cg->grove.chowa[i].chowa = &cg->chowa[i];
        if (cg->grove.chowa[i].chowa->active)
        {
            cg->grove.chowa[i].aabb.height = 24;
            cg->grove.chowa[i].aabb.width  = 24;
            cg->grove.chowa[i].aabb.pos.x  = 32 + esp_random() % (cg->grove.groveBG.w - 64);
            cg->grove.chowa[i].aabb.pos.y  = 32 + esp_random() % (cg->grove.groveBG.h - 64);
        }
    }
    for (int32_t i = 0; i < CG_GROVE_MAX_GUEST_CHOWA; i++)
    {
        cg->grove.chowa[i + CG_MAX_CHOWA].chowa = &cg->guests[i];
    }

    // Initialize items
    // TODO:
}

/**
 * @brief Destroys the grove data
 *
 * @param cg Game Data
 */
void cg_deInitGrove(cGrove_t* cg)
{
    // Unload assets
    // Audio
    unloadMidiFile(&cg->grove.bgm);
    // WSGs
    for (uint8_t i = 0; i < ARRAY_SIZE(questionMarkSprites); i++)
    {
        freeWsg(&cg->grove.questionMarks[i]);
    }
    free(cg->grove.questionMarks);
    for (uint8_t i = 0; i < ARRAY_SIZE(angerParticles); i++)
    {
        freeWsg(&cg->grove.angerParticles[i]);
    }
    free(cg->grove.angerParticles);
    for (uint8_t i = 0; i < ARRAY_SIZE(groveCursorSprites); i++)
    {
        freeWsg(&cg->grove.cursors[i]);
    }
    free(cg->grove.cursors);
    freeWsg(&cg->grove.groveBG);
}

/**
 * @brief Main loop for Garden mode
 *
 * @param cg Game Object
 */
void cg_runGrove(cGrove_t* cg, int64_t elapsedUS)
{
    // Play BGM if it's not playing
    if (!isBGMPlaying)
    {
        soundPlayBgmCb(&cg->grove.bgm, MIDI_BGM, cg_bgmCB);
        isBGMPlaying = true;
    }

    // TODO:
    // Ability to sit on stump and do things
    // Change cursor if over item or Chowa

    // Input
    cg_handleInputGarden(cg);

    // Garden Logic
    if (cg->grove.holdingItem)
    {
        cg->grove.heldItem->aabb.pos = addVec2d(cg->grove.cursor.pos, cg->grove.camera.pos);
    }
    if (cg->grove.holdingChowa)
    {
        cg->grove.heldChowa->aabb.pos = addVec2d(cg->grove.cursor.pos, cg->grove.camera.pos);
    }

    // Chowa AI
    for (int32_t idx = 0; idx < CG_MAX_CHOWA + CG_GROVE_MAX_GUEST_CHOWA; idx++)
    {
        if (cg->grove.chowa[idx].chowa->active)
        {
            cg_GroveAI(cg, &cg->grove.chowa[idx], elapsedUS);
        }
    }

    // Draw
    cg_groveDraw(cg, elapsedUS);
}

//==============================================================================
// Static functions
//==============================================================================

static void cg_attemptGrab(cGrove_t* cg)
{
    vec_t collVec;
    cg->grove.heldItem = NULL;
    // Check if over a Chowa
    for (int8_t c = 0; c < CG_MAX_CHOWA; c++)
    {
        if (cg->chowa[c].active)
        {
            rectangle_t translated = {.pos    = subVec2d(cg->grove.chowa[c].aabb.pos, cg->grove.camera.pos),
                                      .height = cg->grove.chowa[c].aabb.height,
                                      .width  = cg->grove.chowa[c].aabb.width};
            if (rectRectIntersection(cg->grove.cursor, translated, &collVec))
            {
                cg->grove.holdingChowa      = true;
                cg->grove.heldChowa         = &cg->grove.chowa[c];
                cg->grove.heldChowa->gState = CHOWA_HELD;
            }
        }
    }
    // Check if over an item
    for (int8_t item = 0; item < CG_GROVE_MAX_ITEMS; item++)
    {
        if (cg->grove.items[item].active)
        {
            rectangle_t translated = {.pos    = subVec2d(cg->grove.items[item].aabb.pos, cg->grove.camera.pos),
                                      .height = cg->grove.items[item].aabb.height,
                                      .width  = cg->grove.items[item].aabb.width};
            if (rectRectIntersection(cg->grove.cursor, translated, &collVec))
            {
                cg->grove.holdingItem = true;
                cg->grove.heldItem    = &cg->grove.items[item];
            }
        }
    }
}

static void cg_handleInputGarden(cGrove_t* cg)
{
    buttonEvt_t evt;
    // Touch pad for the hands
    if (cg->touch)
    {
        int32_t phi, r, intensity;
        if (getTouchJoystick(&phi, &r, &intensity))
        {
            int16_t speed = phi >> 5;
            if (!(speed <= 5))
            {
                printf("touch center: %" PRIu32 ", intensity: %" PRIu32 ", intensity %" PRIu32 "\n", phi, r, intensity);
                // Move hand
                cg->grove.cursor.pos.x += (getCos1024(phi) * speed) / 1024;
                cg->grove.cursor.pos.y -= (getSin1024(phi) * speed) / 1024;
            }
        }
        while (checkButtonQueueWrapper(&evt))
        {
            if (evt.button & PB_A && evt.down)
            {
                if (cg->grove.holdingItem)
                {
                    cg->grove.holdingItem = false;
                }
                else if (cg->grove.holdingChowa)
                {
                    cg->grove.holdingChowa = false;
                    cg->grove.heldChowa->gState = CHOWA_IDLE;
                }
                else
                {
                    cg_attemptGrab(cg);
                }
            }
            if (evt.button & PB_B && evt.down)
            {
                // TODO: Pet Chowa
            }
        }
    }
    else
    {
        while (checkButtonQueueWrapper(&evt))
        {
            if (evt.button & PB_RIGHT)
            {
                cg->grove.cursor.pos.x += CG_CURSOR_SPEED;
            }
            else if (evt.button & PB_LEFT)
            {
                cg->grove.cursor.pos.x -= CG_CURSOR_SPEED;
            }
            if (evt.button & PB_UP)
            {
                cg->grove.cursor.pos.y -= CG_CURSOR_SPEED;
            }
            else if (evt.button & PB_DOWN)
            {
                cg->grove.cursor.pos.y += CG_CURSOR_SPEED;
            }
            if (evt.button & PB_A && evt.down)
            {
                if (cg->grove.holdingItem)
                {
                    cg->grove.holdingItem = false;
                }
                else if (cg->grove.holdingChowa)
                {
                    cg->grove.holdingChowa = false;
                    cg->grove.heldChowa->gState = CHOWA_IDLE;
                }
                else
                {
                    cg_attemptGrab(cg);
                }
            }
            if (evt.button & PB_B && evt.down)
            {
                // TODO: Pet Chowa
            }
        }
    }
    // Check if out of bounds
    if (cg->grove.cursor.pos.x < CG_GROVE_SCREEN_BOUNDARY)
    {
        cg_moveCamera(cg, -CG_CURSOR_SPEED, 0);
        cg->grove.cursor.pos.x = CG_GROVE_SCREEN_BOUNDARY;
    }
    else if (cg->grove.cursor.pos.x > TFT_WIDTH - (CG_GROVE_SCREEN_BOUNDARY + cg->grove.cursor.width))
    {
        cg_moveCamera(cg, CG_CURSOR_SPEED, 0);
        cg->grove.cursor.pos.x = TFT_WIDTH - (CG_GROVE_SCREEN_BOUNDARY + cg->grove.cursor.width);
    }
    if (cg->grove.cursor.pos.y < CG_GROVE_SCREEN_BOUNDARY)
    {
        cg_moveCamera(cg, 0, -CG_CURSOR_SPEED);
        cg->grove.cursor.pos.y = CG_GROVE_SCREEN_BOUNDARY;
    }
    else if (cg->grove.cursor.pos.y > TFT_HEIGHT - (CG_GROVE_SCREEN_BOUNDARY + cg->grove.cursor.height))
    {
        cg_moveCamera(cg, 0, CG_CURSOR_SPEED);
        cg->grove.cursor.pos.y = TFT_HEIGHT - (CG_GROVE_SCREEN_BOUNDARY + cg->grove.cursor.height);
    }
}

static void cg_moveCamera(cGrove_t* cg, int16_t xChange, int16_t yChange)
{
    cg->grove.camera.pos.x += xChange;
    cg->grove.camera.pos.y += yChange;
    if (cg->grove.camera.pos.x < 0)
    {
        cg->grove.camera.pos.x = 0;
    }
    else if (cg->grove.camera.pos.x > (cg->grove.groveBG.w - TFT_WIDTH))
    {
        cg->grove.camera.pos.x = (cg->grove.groveBG.w - TFT_WIDTH);
    }
    if (cg->grove.camera.pos.y < 0)
    {
        cg->grove.camera.pos.y = 0;
    }
    else if (cg->grove.camera.pos.y > (cg->grove.groveBG.h - TFT_HEIGHT))
    {
        cg->grove.camera.pos.y = (cg->grove.groveBG.h - TFT_HEIGHT);
    }
}

static void cg_setupBorders(cGrove_t* cg)
{
    // Tree
    cg->grove.boundaries[CG_TREE].pos.x  = 0;
    cg->grove.boundaries[CG_TREE].pos.y  = 10;
    cg->grove.boundaries[CG_TREE].width  = 106;
    cg->grove.boundaries[CG_TREE].height = 82;

    // Stump
    cg->grove.boundaries[CG_STUMP].pos.x  = 492;
    cg->grove.boundaries[CG_STUMP].pos.y  = 66;
    cg->grove.boundaries[CG_STUMP].width  = 48;
    cg->grove.boundaries[CG_STUMP].height = 32;

    // Water
    cg->grove.boundaries[CG_WATER].pos.x  = 32;
    cg->grove.boundaries[CG_WATER].pos.y  = 350;
    cg->grove.boundaries[CG_WATER].width  = 280;
    cg->grove.boundaries[CG_WATER].height = 108;
}

static void cg_bgmCB()
{
    isBGMPlaying = false;
}