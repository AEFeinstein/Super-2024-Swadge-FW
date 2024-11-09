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
#include "cg_GroveItems.h"
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

static const char* musicNoteSprites[] = {
    "cg_note1.wsg",
    "cg_note2.wsg",
    "cg_note3.wsg",
};

static const char* speechBubbleSprites[] = {
    "cg_text0.wsg",
    "cg_text1.wsg",
    "cg_text2.wsg",
    "cg_text3.wsg",
};

static const char* eggsIntactSprites[] = {
    "chowa_egg1.wsg", "chowa_egg2.wsg", "chowa_egg3.wsg", "chowa_egg4.wsg", "chowa_egg5.wsg", "chowa_egg6.wsg",
};

static const char* itemSprites[] = {
    "agi_book.wsg",   "cha_book.wsg", "spd_book.wsg",     "sta_book.wsg", "str_book.wsg", "cg_ball.wsg",
    "cg_crayons.wsg", "cg_knife.wsg", "cg_toy_sword.wsg", "cake.wsg",     "souffle.wsg",  "DonutRing.wsg",
};

static const char shopMenuTitle[] = "Ring Shop";

static const char* menuLabels[] = {
    "Shop for items", "View inventory", "View Chowa", "Release Chowa", "Tutorial", "Exit Menu", "Exit Grove",
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

/**
 * @brief Menu callback
 *
 * @param label Selection labels
 * @param selected If selected
 * @param settingVal Value of a changed setting
 */
static void shopMenuCb(const char* label, bool selected, uint32_t settingVal);

/**
 * @brief Callback to restart BGM
 *
 */
static void cg_bgmCB(void);

//==============================================================================
// Variables
//==============================================================================

cGrove_t* cgr;

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
    // set cgr
    cgr = cg;

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
    cg->grove.notes = calloc(ARRAY_SIZE(musicNoteSprites), sizeof(wsg_t));
    for (int32_t idx = 0; idx < ARRAY_SIZE(musicNoteSprites); idx++)
    {
        loadWsg(musicNoteSprites[idx], &cg->grove.notes[idx], true);
    }
    cg->grove.speechBubbles = calloc(ARRAY_SIZE(speechBubbleSprites), sizeof(wsg_t));
    for (int32_t idx = 0; idx < ARRAY_SIZE(speechBubbleSprites); idx++)
    {
        loadWsg(speechBubbleSprites[idx], &cg->grove.speechBubbles[idx], true);
    }
    // Items
    cg->grove.itemsWSGs = calloc(ARRAY_SIZE(itemSprites), sizeof(wsg_t));
    for (int32_t idx = 0; idx < ARRAY_SIZE(itemSprites); idx++)
    {
        loadWsg(itemSprites[idx], &cg->grove.itemsWSGs[idx], true);
    }
    // Eggs
    cg->grove.eggs = calloc(ARRAY_SIZE(eggsIntactSprites), sizeof(wsg_t));
    for (int32_t idx = 0; idx < ARRAY_SIZE(eggsIntactSprites); idx++)
    {
        loadWsg(eggsIntactSprites[idx], &cg->grove.eggs[idx], true);
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
        if (cg->grove.chowa[i + CG_MAX_CHOWA].chowa->active)
        {
            cg->grove.chowa[i + CG_MAX_CHOWA].aabb.height = 24;
            cg->grove.chowa[i + CG_MAX_CHOWA].aabb.width  = 24;
            cg->grove.chowa[i + CG_MAX_CHOWA].aabb.pos.x  = 32 + esp_random() % (cg->grove.groveBG.w - 64);
            cg->grove.chowa[i + CG_MAX_CHOWA].aabb.pos.y  = 32 + esp_random() % (cg->grove.groveBG.h - 64);
        }
    }

    // Init Ring
    cg->grove.ring.aabb.height = cg->grove.itemsWSGs[11].h;
    cg->grove.ring.aabb.width  = cg->grove.itemsWSGs[11].w;

    // Initialize shop menu
    cg->grove.menu = initMenu(shopMenuTitle, shopMenuCb);
    for (int idx = 0; idx < ARRAY_SIZE(menuLabels); idx++)
    {
        addSingleItemToMenu(cg->grove.menu, menuLabels[idx]);
    }
    // TODO: Recolor
    cg->grove.renderer = initMenuManiaRenderer(NULL, NULL, NULL);

    // TODO: Load inventory from NVS
    cg->grove.inv.money = 10000;

    // Set initial state
    // TODO: Load tutorial if first run
    cg->grove.state = CG_GROVE_FIELD;
}

/**
 * @brief Destroys the grove data
 *
 * @param cg Game Data
 */
void cg_deInitGrove(cGrove_t* cg)
{
    deinitMenuManiaRenderer(cg->grove.renderer);
    deinitMenu(cg->grove.menu);

    // Unload assets
    // Audio
    unloadMidiFile(&cg->grove.bgm);
    // WSGs
    for (uint8_t i = 0; i < ARRAY_SIZE(eggsIntactSprites); i++)
    {
        freeWsg(&cg->grove.eggs[i]);
    }
    free(cg->grove.eggs);
    for (uint8_t i = 0; i < ARRAY_SIZE(itemSprites); i++)
    {
        freeWsg(&cg->grove.itemsWSGs[i]);
    }
    free(cg->grove.itemsWSGs);
    for (uint8_t i = 0; i < ARRAY_SIZE(speechBubbleSprites); i++)
    {
        freeWsg(&cg->grove.speechBubbles[i]);
    }
    free(cg->grove.speechBubbles);
    for (uint8_t i = 0; i < ARRAY_SIZE(musicNoteSprites); i++)
    {
        freeWsg(&cg->grove.notes[i]);
    }
    free(cg->grove.notes);
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
    if (!cg->grove.isBGMPlaying)
    {
        // soundPlayBgmCb(&cg->grove.bgm, MIDI_BGM, cg_bgmCB);
        cg->grove.isBGMPlaying = true;
    }
    switch (cg->grove.state)
    {
        case CG_GROVE_MENU:
        {
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                cg->grove.menu = menuButton(cg->grove.menu, evt);
            }
            drawMenuMania(cg->grove.menu, cg->grove.renderer, elapsedUS);
            break;
        }
        case CG_GROVE_FIELD:
        {
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

            // Resurface items dropped in the water
            vec_t temp;
            for (int idx = 0; idx < CG_GROVE_MAX_ITEMS; idx++)
            {
                if (&cg->grove.items[idx] != cg->grove.heldItem
                    && rectRectIntersection(cg->grove.items[idx].aabb, cg->grove.boundaries[CG_WATER], &temp))
                {
                    cg->grove.items[idx].aabb.pos.x
                        = 32 + (esp_random() % (cg->grove.groveBG.w - (64 + cg->grove.itemsWSGs[idx].w)));
                    cg->grove.items[idx].aabb.pos.y
                        = 32 + (esp_random() % (cg->grove.groveBG.h - (64 + cg->grove.itemsWSGs[idx].h)));
                }
            }

            // Spawn Rings
            if (esp_random() % 512 == 0)
            {
                cg->grove.ring.aabb.pos.x
                    = 32 + (esp_random() % (cg->grove.groveBG.w - (64 + cg->grove.itemsWSGs[11].w)));
                cg->grove.ring.aabb.pos.y
                    = 32 + (esp_random() % (cg->grove.groveBG.h - (64 + cg->grove.itemsWSGs[11].h)));
                cg->grove.ring.active = true;
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
            cg_groveDrawField(cg, elapsedUS);
            break;
        }
        case CG_GROVE_SHOP:
        {
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && evt.button & PB_DOWN)
                {
                    cg->grove.shopSelection += 1;
                    if (cg->grove.shopSelection >= CG_MAX_TYPE_ITEMS)
                    {
                        cg->grove.shopSelection = 0;
                    }
                }
                else if (evt.down && evt.button & PB_UP)
                {
                    cg->grove.shopSelection -= 1;
                    if (cg->grove.shopSelection < 0)
                    {
                        cg->grove.shopSelection = CG_MAX_TYPE_ITEMS - 1;
                    }
                }
                else if (evt.down && evt.button & PB_A)
                {
                    // Attempt to buy an item
                    if (cg->grove.inv.money >= itemPrices[cg->grove.shopSelection])
                    {
                        cg->grove.inv.money -= itemPrices[cg->grove.shopSelection];
                        cg->grove.inv.quantities[cg->grove.shopSelection] += 1;
                    }
                }
                else if (evt.down && evt.button & PB_B)
                {
                    // Attempt to sell an item
                    if (cg->grove.inv.quantities[cg->grove.shopSelection] > 0)
                    {
                        cg->grove.inv.money += itemPrices[cg->grove.shopSelection];
                        cg->grove.inv.quantities[cg->grove.shopSelection] -= 1;
                    }
                }
                else if (evt.down)
                {
                    cg->grove.state = CG_GROVE_MENU;
                }
            }
            // Draw shop
            cg_groveDrawShop(cg);
            break;
        }
        case CG_GROVE_INVENTORY:
        {
            // View items and quantities
            // Handle input
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && evt.button & PB_DOWN)
                {
                    cg->grove.shopSelection += 1;
                    if (cg->grove.shopSelection >= CG_GROVE_MAX_ITEMS)
                    {
                        cg->grove.shopSelection = 0;
                    }
                }
                else if (evt.down && evt.button & PB_UP)
                {
                    cg->grove.shopSelection -= 1;
                    if (cg->grove.shopSelection < 0)
                    {
                        cg->grove.shopSelection = CG_GROVE_MAX_ITEMS - 1;
                    }
                }
                else if (evt.down && evt.button & PB_A)
                {
                    // Attempt to add item to field
                    if (cg->grove.inv.quantities[cg->grove.shopSelection] > 0)
                    {
                        cg->grove.items[cg->grove.groveActiveItemIdx].active = true;
                        cg->grove.items[cg->grove.groveActiveItemIdx].spr
                            = cg->grove.itemsWSGs[cg->grove.shopSelection];
                        cg->grove.items[cg->grove.groveActiveItemIdx].aabb.pos.x
                            = 32
                              + (esp_random()
                                 % (cg->grove.groveBG.w - (64 + cg->grove.itemsWSGs[cg->grove.shopSelection].w)));
                        cg->grove.items[cg->grove.groveActiveItemIdx].aabb.pos.y
                            = 32
                              + (esp_random()
                                 % (cg->grove.groveBG.h - (64 + cg->grove.itemsWSGs[cg->grove.shopSelection].h)));
                        cg->grove.items[cg->grove.groveActiveItemIdx].aabb.height
                            = cg->grove.itemsWSGs[cg->grove.shopSelection].h;
                        cg->grove.items[cg->grove.groveActiveItemIdx].aabb.width
                            = cg->grove.itemsWSGs[cg->grove.shopSelection].w;
                        strcpy(cg->grove.items[cg->grove.groveActiveItemIdx].name,
                               shopMenuItems[cg->grove.shopSelection]);
                        cg->grove.groveActiveItemIdx++;
                        cg->grove.inv.quantities[cg->grove.shopSelection] -= 1;
                        if (cg->grove.groveActiveItemIdx >= CG_GROVE_MAX_ITEMS)
                        {
                            cg->grove.groveActiveItemIdx = 0;
                        }
                    }
                }
                else if (evt.down && evt.button & PB_B)
                {
                    // Clear all items from field
                    for (int idx = 0; idx < CG_GROVE_MAX_ITEMS; idx++)
                    {
                        for (int idx2 = 0; idx2 < CG_MAX_TYPE_ITEMS; idx2++)
                        {
                            if (strcmp(cg->grove.items[idx].name, shopMenuItems[idx2]) == 0)
                            {
                                cg->grove.inv.quantities[idx2] += 1;
                            }
                        }
                        cg->grove.items[idx].active = false;
                        strcpy(cg->grove.items[idx].name, "");
                    }
                    cg->grove.groveActiveItemIdx = 0;
                }
                else if (evt.down)
                {
                    cg->grove.state = CG_GROVE_MENU;
                }
            }
            // Draw
            cg_groveDrawInv(cg);
            break;
        }
        case CG_GROVE_VIEW_STATS:
        {
            // Display stats
            // Handle input, up/down, back, A prompts user to kick guest (only if guest)
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && evt.button & PB_DOWN)
                {
                    cg->grove.shopSelection += 1;
                    if (cg->grove.shopSelection >= CG_MAX_CHOWA + CG_GROVE_MAX_GUEST_CHOWA)
                    {
                        cg->grove.shopSelection = 0;
                    }
                }
                else if (evt.down && evt.button & PB_UP)
                {
                    cg->grove.shopSelection -= 1;
                    if (cg->grove.shopSelection < 0)
                    {
                        cg->grove.shopSelection = CG_MAX_CHOWA + CG_GROVE_MAX_GUEST_CHOWA - 1;
                    }
                }
                else if (evt.down && evt.button & PB_A && cg->grove.shopSelection >= CG_MAX_CHOWA)
                {
                    cg->grove.confirm = !cg->grove.confirm;
                }
                else if (evt.down && evt.button & PB_B && cg->grove.confirm)
                {
                    cg->guests[cg->grove.shopSelection - CG_MAX_CHOWA].active = false;
                    cg->grove.confirm                                         = false;
                }
                else if (evt.down)
                {
                    cg->grove.state = CG_GROVE_MENU;
                }
            }
            // Draw Stats
            cg_groveDrawStats(cg);
            break;
        }
        /* case CG_GROVE_ABANDON:
        {
            // Take Chowa behind the shed
            // Handle input, up/down, back, A for buying plus prompt
            // Draw kill screen
            // break;
        }
        case CG_GROVE_TUTORIAL:
        {
            // Handle input
            // Draw Tutorial
            // break;
        } */
        default:
        {
            // Something isn't implemented, so kick back to menu
            cg->grove.state = CG_GROVE_MENU;
            break;
        }
    }
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
                    cg->grove.heldItem    = NULL;
                }
                else if (cg->grove.holdingChowa)
                {
                    cg->grove.holdingChowa      = false;
                    cg->grove.heldChowa->gState = CHOWA_IDLE;
                }
                else
                {
                    cg_attemptGrab(cg);
                }
            }
            if (evt.button & PB_B && evt.down)
            {
                for (int idx = 0; idx < CG_MAX_CHOWA + CG_GROVE_MAX_GUEST_CHOWA; idx++)
                {
                    vec_t temp;
                    rectangle_t rect = {.pos    = addVec2d(cg->grove.cursor.pos, cg->grove.camera.pos),
                                        .height = cg->grove.cursor.height,
                                        .width  = cg->grove.cursor.width};
                    // Chowa
                    if (rectRectIntersection(rect, cg->grove.chowa[idx].aabb, &temp))
                    {
                        cg->grove.chowa[idx].gState   = CHOWA_PET;
                        cg->grove.chowa[idx].timeLeft = 1000000;
                    }
                }
            }
            if (evt.button & PB_START && evt.down)
            {
                cg->grove.state = CG_GROVE_MENU;
            }
        }
    }
    else
    {
        vec_t temp;
        rectangle_t rect = {.pos    = addVec2d(cg->grove.cursor.pos, cg->grove.camera.pos),
                            .height = cg->grove.cursor.height,
                            .width  = cg->grove.cursor.width};
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
                    cg->grove.heldItem    = NULL;
                }
                else if (cg->grove.holdingChowa)
                {
                    cg->grove.holdingChowa      = false;
                    cg->grove.heldChowa->gState = CHOWA_IDLE;
                }
                else if (cg->grove.ring.active && rectRectIntersection(rect, cg->grove.ring.aabb, &temp))
                {
                    cg->grove.ring.active = false;
                    cg->grove.inv.money += 1;
                }
                else
                {
                    cg_attemptGrab(cg);
                }
            }
            if (evt.button & PB_B && evt.down)
            {
                for (int idx = 0; idx < CG_MAX_CHOWA + CG_GROVE_MAX_GUEST_CHOWA; idx++)
                {
                    // Chowa
                    if (rectRectIntersection(rect, cg->grove.chowa[idx].aabb, &temp))
                    {
                        cg->grove.chowa[idx].gState   = CHOWA_PET;
                        cg->grove.chowa[idx].timeLeft = 1000000;
                    }
                }
            }
            if (evt.button & PB_START && evt.down)
            {
                cg->grove.state = CG_GROVE_MENU;
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

static void shopMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == menuLabels[0])
        {
            // Enter Shop
            cgr->grove.state = CG_GROVE_SHOP;
        }
        if (label == menuLabels[1])
        {
            // View inventory, spawn in world
            // Settings menu, use scrolls to pick item
            cgr->grove.state = CG_GROVE_INVENTORY;
        }
        if (label == menuLabels[2])
        {
            // View Stats and Guests, kick guests
            cgr->grove.state         = CG_GROVE_VIEW_STATS;
            cgr->grove.shopSelection = 0; // To avoid out of bounds errors
        }
        else if (label == menuLabels[3])
        {
            // Release Chowa
            cgr->grove.state = CG_GROVE_ABANDON;
        }
        else if (label == menuLabels[3])
        {
            // Tutorial
            cgr->grove.state = CG_GROVE_TUTORIAL;
        }
        else if (label == menuLabels[5])
        {
            // Back
            cgr->grove.state = CG_GROVE_FIELD;
        }
        else if (label == menuLabels[6])
        {
            // Exit Mode
            cgr->state  = CG_MAIN_MENU;
            cgr->unload = true;
            globalMidiPlayerStop(true);
        }
    }
}

static void cg_bgmCB()
{
    cgr->grove.isBGMPlaying = false;
}