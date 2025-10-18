//==============================================================================
// Includes
//==============================================================================

// Swadge
#include "miniMiner.h"
#include "mainMenu.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Consts
//==============================================================================

static const char modeName[] = "miniminer";

static const char* const menuText[] = {
    "Overworld",    "Play the game",      "Tutorial", "Learn to play",
    "Encyclopedia", "Read about systems", "Exit",     "Return to Main Menu",
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SPLASH,
    MENU,
    OVERWORLD,
    MAP, // Main, prototypes, tutorials
    ENCYCLOPEDIA,
} mmStates_t;

typedef enum
{
    MO_OVERWORLD,
    MO_TUTORIAL,
    MO_ENCYCLOPEDIA,
    MO_EXIT,
    MO_COUNT
} mmMenuOptions_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // State
    mmStates_t state;

    // Main menu
    int8_t selectIdx;
} mmData_t;

//==============================================================================
// Function definitions
//==============================================================================

// Mode requirements
void mmEnter(void);
void mmExit(void);
void mmLoop(int64_t elapsedUS);

// Main screens
void mmDrawMenu(void);
void mmDrawSplash(int64_t elapsedUS);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t miniMinerMode = {
    .modeName    = modeName,
    .wifiMode    = NO_WIFI,
    .fnEnterMode = mmEnter,
    .fnExitMode  = mmExit,
    .fnMainLoop  = mmLoop,
};

mmData_t* mm;

//==============================================================================
// Functions
//==============================================================================

void mmEnter(void)
{
    mm = (mmData_t*)heap_caps_calloc(1, sizeof(mmData_t), MALLOC_CAP_8BIT);

    mm->state = SPLASH;
}

void mmExit(void)
{
    free(mm);
}

void mmLoop(int64_t elapsedUS)
{
    buttonEvt_t evt;
    switch (mm->state)
    {
        case SPLASH:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    mm->state = MENU;
                }
            }
            mmDrawSplash(elapsedUS);
            break;
        }
        case MENU:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_UP)
                    {
                        mm->selectIdx--;
                        if (mm->selectIdx < 0)
                        {
                            mm->selectIdx = MO_COUNT - 1;
                        }
                    }
                    else if (evt.button & PB_DOWN)
                    {
                        mm->selectIdx++;
                        if (mm->selectIdx >= MO_COUNT)
                        {
                            mm->selectIdx = 0;
                        }
                    }
                    if (evt.button & PB_A)
                    {
                        switch (mm->selectIdx)
                        {
                            case (MO_EXIT):
                            {
                                switchToSwadgeMode(&mainMenuMode);
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                    }
                }
            }
            mmDrawMenu();
            break;
        }
        default:
        {
            while (checkButtonQueueWrapper(&evt))
            {
            }
            break;
        }
    }
}

void mmDrawMenu(void)
{
    clearPxTft();
    drawText(getSysFont(), c555, modeName, 32, 32);
    for (int idx = 0; idx < MO_COUNT; idx++)
    {
        drawText(getSysFont(), (idx == mm->selectIdx) ? c550 : c444, menuText[idx * 2], 32, 60 + (idx * 24));
        if (idx == mm->selectIdx)
        {
            int16_t x = 32;
            int16_t y = TFT_HEIGHT - 60;
            drawTextWordWrap(getSysFont(), c444, menuText[(idx * 2) + 1], &x, &y, TFT_WIDTH - 32, TFT_HEIGHT);
        }
    }
}

void mmDrawSplash(int64_t elapsedUS)
{
    drawText(getSysFont(), c555, modeName, 32, 32);
}