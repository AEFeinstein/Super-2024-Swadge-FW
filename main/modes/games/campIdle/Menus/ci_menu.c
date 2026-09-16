//==============================================================================
// Include
//==============================================================================

// Main
#include "ci_menu.h"

// Swadge
#include "mainMenu.h"

// Camp
#include "ci_items.h"
#include "ci_crafting.h"
#include "ci_helpers.h"
#include "ci_recipeData.h"

//==============================================================================
// Defines
//==============================================================================

// Times
#define SPLASH_TIMER 500000
// Title offset
#define TITLE_X 32
#define TITLE_Y 32
// Encyclopedia
#define ENC_ROW 4
#define ENC_COL 6

//==============================================================================
// Consts
//==============================================================================

const char* const menuText[] = {
    "Main Menu", "Play!", "Encyclopedia", "Tutorial", "Quit", "Cozy Camper", "Press 'A' to play!",
};

//==============================================================================
// Function Definitions
//==============================================================================

/**
 * @brief Draws the splash screen
 *
 * @param ccd Game Data
 * @param elapsedUs Time since last frame
 */
static void drawSplash(ciCampData_t* ccd, int64_t elapsedUs);

/**
 * @brief Draws the menus
 *
 * @param ccd Game Data
 */
static void drawMenu(ciCampData_t* ccd);

/**
 * @brief Draws the encyclopedia
 *
 * @param ccd Game Data
 */
static void drawEncyclopedia(ciCampData_t* ccd);

//==============================================================================
// Functions
//==============================================================================

void ciInitState(ciCampData_t* ccd, ciState_t state)
{
    ccd->state = state;
    switch (state)
    {
        case CI_SPLASH:
        {
            ccd->timer = 0;
            break;
        }
        case CI_MENU:
        {
            ccd->selection = CI_MENU_PLAY;
            break;
        }
        case CI_ENCYC:
        {
            ccd->selection = 0;
            break;
        }
        case CI_CRAFTING_PREP:
        {
            ccd->selection = 0;
            break;
        }
        default:
        {
            break;
        }
    }
}

void ciRunSplash(ciCampData_t* ccd, int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && (evt.button & PB_A))
        {
            ciInitState(ccd, CI_MENU);
        }
    }
    drawSplash(ccd, elapsedUs);
}

void ciRunMenu(ciCampData_t* ccd)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button & PB_DOWN)
            {
                ccd->selection++;
                if (ccd->selection > CI_MENU_QUIT)
                {
                    ccd->selection = CI_MENU_PLAY;
                }
            }
            else if (evt.button & PB_UP)
            {
                ccd->selection--;
                if (ccd->selection < CI_MENU_PLAY)
                {
                    ccd->selection = CI_MENU_QUIT;
                }
            }
            else if (evt.button & PB_A)
            {
                switch (ccd->selection)
                {
                    case CI_MENU_PLAY:
                    {
                        ciInitState(ccd, CI_CRAFTING); // FIXME: Need to add the rest of the menu substructure
                        break;
                    }
                    case CI_ENCYC:
                    {
                        ciInitState(ccd, CI_ENCYC);
                        break;
                    }
                    case CI_MENU_TUTORIAL:
                    {
                        break;
                    }
                    case CI_MENU_QUIT:
                    {
                        switchToSwadgeMode(&mainMenuMode);
                        break;
                    }
                    default:
                    {
                        ESP_LOGE("CC", "Invalid selection");
                        break;
                    }
                }
            }
            else if (evt.button & PB_B)
            {
                ciInitState(ccd, CI_SPLASH);
            }
        }
    }
    drawMenu(ccd);
}

void ciRunEncyclopedia(ciCampData_t* ccd)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            ccd->selection = ciMenu2DNavigate(&evt, ccd->selection, ENC_COL, ciGetItemCount());
            if (evt.button & PB_A)
            {
                ccd->state = CI_ENCYC_DESC;
            }
            else if (evt.button & PB_B)
            {
                ciInitState(ccd, CI_MENU);
            }
        }
    }
    drawEncyclopedia(ccd);
}

bool ciRunCraft(ciCampData_t* ccd)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button & PB_A || evt.button & PB_LEFT)
            {
                ciInitState(ccd, CI_CRAFTING_PREP);
                // TODO: Add positive beep sound
            }
            else if (evt.button & PB_B)
            {
                // TODO: Add positive beep sound
                return true;
            }
        }
    }
    drawCraft(&ccd->cft, &ccd->inv, &ccd->largeText, &ccd->smallFont, ccd->timerUnits, ccd->timerUs);
    return false;
}

void ciRunCraftSelection(ciCampData_t* ccd)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            ccd->selection = ciMenu2DNavigate(&evt, ccd->selection, MAX_COLS, ciGetRecipeCount());
            if (evt.button & PB_A)
            {
                const ciRecipeProto_t* r = &recipeList[ccd->selection];
                bool ableToCraft
                    = (ccd->inv.qtys[r->items[0].item] >= r->items[0].qty)
                      && (r->items[1].item == CI_NO_ITEM || (ccd->inv.qtys[r->items[1].item] >= r->items[1].qty));
                ableToCraft = ableToCraft && CHECK_BIT(ccd->wbd.benches, r->craftingStation);
                if (ableToCraft)
                {
                    ciRemoveFromInv(&ccd->inv, r->items[0].item, r->items[0].qty);
                    ciRemoveFromInv(&ccd->inv, r->items[1].item, r->items[1].qty);
                    push(&ccd->cft.craftQueue, (intptr_t*)ccd->selection);
                    // TODO: Add positive beep sound
                }
                else
                {
                    // TODO: Add negative beep sound
                }
            }
            else if (evt.button & PB_B)
            {
                ciInitState(ccd, CI_CRAFTING);
                // TODO: Add positive beep sound
            }
        }
    }
    drawCraftSelection(&ccd->cft, &ccd->inv, &ccd->wbd, &ccd->largeText, &ccd->smallFont, ccd->selection);
}

//==============================================================================
// Static Functions
//==============================================================================

static void drawSplash(ciCampData_t* ccd, int64_t elapsedUs)
{
    // Draw background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c001);
    // Draw title
    drawText(&ccd->largeText, c555, menuText[CI_MENU_SPLASH], TITLE_X, TITLE_Y);
    ccd->timer += elapsedUs;
    if (ccd->timer >= SPLASH_TIMER)
    {
        ccd->timer = 0;
    }
}

static void drawMenu(ciCampData_t* ccd)
{
    // Draw background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c010);
    // Draw title
    drawText(&ccd->largeText, c555, menuText[CI_MENU_TITLE], TITLE_X, TITLE_Y);
    // Draw entries
    for (int idx = CI_MENU_PLAY; idx < CI_MENU_QUIT + 1; idx++)
    {
        drawText(&ccd->largeText, c555, menuText[idx], TITLE_X, TITLE_Y * idx + 2 * TITLE_Y);
    }
    // Draw selection box
    int xStart = TITLE_X;
    int yStart = (TITLE_Y * ccd->selection) + 2 * TITLE_Y;
    drawRect(xStart - 2, yStart - 2, xStart + textWidth(&ccd->largeText, menuText[ccd->selection]) + 2,
             yStart + ccd->largeText.height + 2, c550);
}

static void drawEncyclopedia(ciCampData_t* ccd)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c100);
    // Draw title
    drawText(&ccd->largeText, c555, menuText[CI_MENU_ENCYCLOPEDIA],
             (TFT_WIDTH - textWidth(&ccd->largeText, menuText[CI_MENU_ENCYCLOPEDIA])) / 2, 2);
    // Groups
    int start = (ccd->selection / (ENC_COL * ENC_ROW));
    for (int idx = start * (ENC_COL * ENC_ROW); idx < MIN((start + 1) * (ENC_COL * ENC_ROW), ciGetItemCount()); idx++)
    {
        int idxDiv = idx;
        if (start != 0)
        {
            idxDiv %= (start * (ENC_COL * ENC_ROW));
        }
        ciDrawItemIcon(&ccd->inv, &ccd->smallFont, idx, 20 + (idxDiv % ENC_COL) * 40, 24 + (idxDiv / ENC_COL) * 54,
                       ccd->inv.qtys[idx], (ccd->selection == idx), false);
    }
    int yStart = (TFT_HEIGHT - ccd->uiImages[CI_UI_ARROW].h) / 2;
    if (start != 0)
    {
        drawWsg(&ccd->uiImages[CI_UI_ARROW], 1, yStart, false, true, 0);
    }
    if (ciGetItemCount() > (start + 1) * (ENC_COL * ENC_ROW))
    {
        drawWsgSimple(&ccd->uiImages[CI_UI_ARROW], TFT_WIDTH - (ccd->uiImages[CI_UI_ARROW].w + 1), yStart);
    }
}