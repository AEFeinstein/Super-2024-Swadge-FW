#include "menu.h"
#include "menuLogbookRenderer.h"
#include "marbles.h"

#include <stdint.h>
#include <stdbool.h>

static void marblesEnterMode(void);
static void marblesExitMode(void);
static void marblesMainLoop(int64_t elapsedUs);
static void marblesBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void marblesMenuCb(const char*, bool selected, uint32_t settingVal);

static void marblesHandleButton(buttonEvt_t evt);
void marblesUpdatePhysics(int64_t elapsedUs);
void marblesDrawLevel(void);

static const char marblesName[] = "Marbles";
static const char marblesPlay[] = "Play";

typedef enum
{
    MAIN_MENU,
    IN_LEVEL,
} marblesScreen_t;

swadgeMode_t marblesMode = {
    .modeName                 = marblesName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .fnEnterMode              = marblesEnterMode,
    .fnExitMode               = marblesExitMode,
    .fnMainLoop               = marblesMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = marblesBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

typedef struct
{
    font_t ibm;
    menu_t* menu;
    menuLogbookRenderer_t* renderer;

    marblesScreen_t screen;
} marblesMode_t;

marblesMode_t* marbles;

/**
 * This function is called when this mode is started. It should initialize
 * variables and start the mode.
 */
static void marblesEnterMode(void)
{
    marbles = calloc(1, sizeof(marblesMode_t));
    loadFont("ibm_vga8.font", &marbles->ibm, false);

    marbles->menu = initMenu(marblesName, marblesMenuCb);
    addSingleItemToMenu(marbles->menu, marblesPlay);

    marbles->renderer = initMenuLogbookRenderer(&marbles->ibm);

    marbles->screen = MAIN_MENU;
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void marblesExitMode(void)
{
    deinitMenuLogbookRenderer(marbles->renderer);
    deinitMenu(marbles->menu);
    freeFont(&marbles->ibm);
    free(marbles);
}

static void marblesHandleButton(buttonEvt_t evt)
{
    switch (marbles->screen)
    {
        case MAIN_MENU:
        {
            marbles->menu = menuButton(marbles->menu, evt);
            break;
        }

        case IN_LEVEL:
        {
            break;
        }
    }
}

/**
 * @brief Update the physics and perform collision checks on all entities
 *
 * @param elapsedUs Time since the last call
 */
void marblesUpdatePhysics(int64_t elapsedUs)
{
    // Loop over entities, handle movement, do collision checks
}

/**
 * @brief Draw the game level and entities
 *
 */
void marblesDrawLevel(void)
{
    // Draw the track, marbles, etc.
}

/**
 * This function is called from the main loop. It's pretty quick, but the
 * timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
static void marblesMainLoop(int64_t elapsedUs)
{
    clearPxTft();

    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        marblesHandleButton(evt);
    }

    switch (marbles->screen)
    {
        case MAIN_MENU:
        {
            drawMenuLogbook(marbles->menu, marbles->renderer, elapsedUs);
            break;
        }

        case IN_LEVEL:
        {
            marblesUpdatePhysics(elapsedUs);
            marblesDrawLevel();
            break;
        }
    }
}

/**
 * This function is called when the display driver wishes to update a
 * section of the display.
 *
 * @param disp The display to draw to
 * @param x the x coordiante that should be updated
 * @param y the x coordiante that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void marblesBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, c555);
}

/**
 * @brief Callback for when menu items are selected
 *
 * @param label The menu item that was selected or moved to
 * @param selected true if the item was selected, false if it was moved to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
static void marblesMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (label == marblesPlay)
    {
        marbles->screen = IN_LEVEL;
    }
}