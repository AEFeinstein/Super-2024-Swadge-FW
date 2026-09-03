//==============================================================================
// Include
//==============================================================================

#include "ci_menu.h"

#include "ci_items.h"
#include "mainMenu.h"

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
// Function Definitions
//==============================================================================

static void drawSplash(ciCampData_t* ccd, int64_t elaspsedUs);
static void drawMenu(ciCampData_t* ccd);
static void drawEncyclopedia(ciCampData_t* ccd);

//==============================================================================
// Functions
//==============================================================================

void ciInitSplash(ciCampData_t* ccd)
{
    ccd->state = CI_SPLASH;
    ccd->timer = 0;
}

void ciInitMenu(ciCampData_t* ccd)
{
    ccd->selection = CI_MENU_PLAY;
    ccd->state     = CI_MENU;
}

void ciInitEncyclopedia(ciCampData_t* ccd)
{
    ccd->selection = 0;
    ccd->state     = CI_MENU_ENCYCLOPEDIA;
}

void ciRunSplash(ciCampData_t* ccd, int64_t elaspsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && (evt.button & PB_A))
        {
            ciInitMenu(ccd);
        }
    }
    drawSplash(ccd, elaspsedUs);
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
                        break;
                    }
                    case CI_ENCYC:
                    {
                        ciInitEncyclopedia(ccd);
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
                ciInitSplash(ccd);
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
            if (evt.button & PB_A)
            {
                ccd->state = CI_ENCYC_DESC;
            }
            else if (evt.button & PB_RIGHT)
            {
                ccd->selection++;
                if (ccd->selection % ENC_COL == 0)
                {
                    ccd->selection -= ENC_COL;
                }
                ccd->selection %= ciGetItemArrayLength();
            }
            else if (evt.button & PB_LEFT)
            {
                ccd->selection--;
                if (ccd->selection == -1 || ccd->selection % ENC_COL == 5)
                {
                    ccd->selection += ENC_COL;
                }
            }
            else if (evt.button & PB_DOWN)
            {
                ccd->selection += ENC_COL;
                ccd->selection %= ciGetItemArrayLength();
            }
            else if (evt.button & PB_UP)
            {
                ccd->selection -= ENC_COL;
                if (ccd->selection < 0)
                {
                    ccd->selection += ciGetItemArrayLength();
                }
            }
            else
            {
                ciInitMenu(ccd);
            }
        }
    }

    drawEncyclopedia(ccd);
}

// Static
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
    for (int idx = start * (ENC_COL * ENC_ROW); idx < MIN((start + 1) * (ENC_COL * ENC_ROW), ciGetItemArrayLength());
         idx++)
    {
        int idxDiv = idx;
        if (start != 0)
        {
            idxDiv %= (start * (ENC_COL * ENC_ROW));
        }
        ciDrawItemIcon(ccd, idx, 20 + (idxDiv % ENC_COL) * 40, 24 + (idxDiv / ENC_COL) * 54, (ccd->selection == idx),
                       false);
    }
    int yStart = (TFT_HEIGHT - ccd->uiImages[CI_UI_ARROW].h) / 2;
    if (start != 0)
    {
        drawWsg(&ccd->uiImages[CI_UI_ARROW], 1, yStart, false, true, 0);
    }
    if (ciGetItemArrayLength() > (start + 1) * (ENC_COL * ENC_ROW))
    {
        drawWsgSimple(&ccd->uiImages[CI_UI_ARROW], TFT_WIDTH - (ccd->uiImages[CI_UI_ARROW].w + 1), yStart);
    }
}