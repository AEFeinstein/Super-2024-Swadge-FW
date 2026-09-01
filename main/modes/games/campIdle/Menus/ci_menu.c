//==============================================================================
// Include
//==============================================================================

#include "ci_menu.h"

//==============================================================================
// Defines
//==============================================================================

#define TITLE_X 32
#define TITLE_Y 32

//==============================================================================
// Function Definitions
//==============================================================================

static void drawSplash(ciCampData_t* ccd);
static void drawMenu(ciCampData_t* ccd);

//==============================================================================
// Functions
//==============================================================================

void ciInitSplash(ciCampData_t* ccd)
{
    ccd->state = CI_SPLASH;
}

void ciInitMenu(ciCampData_t* ccd)
{
    ccd->selection = CI_MENU_PLAY;
    ccd->state     = CI_MENU;
}

void ciRunSplash(ciCampData_t* ccd)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && (evt.button & PB_A))
        {
            ciInitMenu(ccd);
        }
    }
    drawSplash(ccd);
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
                //switch()
            }
            else if (evt.button & PB_B)
            {
                ciInitSplash(ccd);
            }
        }
    }
    drawMenu(ccd);
}

// Static
static void drawSplash(ciCampData_t* ccd)
{
    // Draw background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c001);
    // Draw title
    drawText(&ccd->largeText, c555, menuText[CI_MENU_SPLASH], TITLE_X, TITLE_Y);
}

static void drawMenu(ciCampData_t* ccd)
{
    // Draw background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c010);
    // Draw title
    drawText(&ccd->largeText, c555, menuText[CI_MENU_TITLE], TITLE_X, TITLE_Y);
    // Draw entries
    for (int idx = CI_MENU_PLAY; idx < ARRAY_SIZE(menuText); idx++)
    {
        drawText(&ccd->largeText, c555, menuText[idx], TITLE_X, TITLE_Y * idx);
    }
    // Draw selection box
    int xStart = TITLE_X;
    int yStart = (TITLE_Y * ccd->selection);
    drawRect(xStart - 2, yStart - 2, xStart + textWidth(&ccd->largeText, menuText[ccd->selection]) + 2,
             yStart + ccd->largeText.height + 2, c550);
}