#include "menu.h"
#include "sequencerMode.h"

static void sequencerEnterMode(void);
static void sequencerExitMode(void);
static void sequencerMainLoop(int64_t elapsedUs);
static void sequencerBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void sequencerMenuCb(const char*, bool selected, uint32_t settingVal);

static const char sequencerName[]  = "Sequencer";
static const char sequencerMenu1[] = "Menu 1";
static const char sequencerMenu2[] = "Menu 2";
static const char sequencerMenu3[] = "Menu 3";
static const char sequencerMenu4[] = "Menu 4";
static const char sequencerMenu5[] = "Menu 5";
static const char sequencerMenu6[] = "Menu 6";
static const char sequencerMenu7[] = "Menu 7";
static const char sequencerMenu8[] = "Menu 8";

static const char opt1[]                 = "opt1";
static const char opt2[]                 = "opt2";
static const char opt3[]                 = "opt3";
static const char opt4[]                 = "opt4";
static const char* const sequencerOpts[] = {opt1, opt2, opt3, opt4};

static const char sequencerSubMenu1[] = "SubMenu1";
static const char sequencerSubMenu2[] = "SubMenu2";

static const char* keys[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

swadgeMode_t sequencerMode = {
    .modeName                 = sequencerName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = sequencerEnterMode,
    .fnExitMode               = sequencerExitMode,
    .fnMainLoop               = sequencerMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = sequencerBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

typedef struct
{
    font_t ibm;
    menu_t* menu;
    menuManiaRenderer_t* renderer;
} sequencerVars_t;

sequencerVars_t* dv;

/**
 * This function is called when this mode is started. It should initialize
 * variables and start the mode.
 */
static void sequencerEnterMode(void)
{
    dv = calloc(1, sizeof(sequencerVars_t));
    loadFont("ibm_vga8.font", &dv->ibm, false);

    dv->menu = initMenu(sequencerName, sequencerMenuCb);
    addSingleItemToMenu(dv->menu, sequencerMenu1);
    addSingleItemToMenu(dv->menu, sequencerMenu2);

    dv->menu = startSubMenu(dv->menu, sequencerSubMenu1);
    addSingleItemToMenu(dv->menu, sequencerMenu7);

    dv->menu = startSubMenu(dv->menu, sequencerSubMenu2);
    addSingleItemToMenu(dv->menu, sequencerMenu8);
    dv->menu = endSubMenu(dv->menu);

    dv->menu = endSubMenu(dv->menu);

    addSingleItemToMenu(dv->menu, sequencerMenu3);
    addSingleItemToMenu(dv->menu, sequencerMenu4);
    addMultiItemToMenu(dv->menu, sequencerOpts, ARRAY_SIZE(sequencerOpts), 0);
    addSingleItemToMenu(dv->menu, sequencerMenu5);
    addSingleItemToMenu(dv->menu, sequencerMenu6);

    dv->renderer = initMenuManiaRenderer(NULL, NULL, NULL);
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void sequencerExitMode(void)
{
    freeFont(&dv->ibm);
    deinitMenuManiaRenderer(dv->renderer);
    deinitMenu(dv->menu);
    free(dv);
}

/**
 * This function is called from the main loop. It's pretty quick, but the
 * timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
static void sequencerMainLoop(int64_t elapsedUs)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c111);

    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // dv->menu = menuButton(dv->menu, evt);
    }

    // drawMenuMania(dv->menu, dv->renderer, elapsedUs);

#define KEY_MARGIN 2

    int32_t keyWidth = textWidth(&dv->ibm, "C#7") + (2 * KEY_MARGIN);

    int32_t kIdx = 0;
    int32_t kNum = 8;
    int32_t yOff = 3;
    while (yOff < TFT_HEIGHT)
    {
        char tmp[8];
        snprintf(tmp, sizeof(tmp) - 1, "%s%d", keys[kIdx], kNum);
        if (0 == kIdx)
        {
            kIdx = ARRAY_SIZE(keys) - 1;
            kNum--;
        }
        else
        {
            kIdx--;
        }

        paletteColor_t bgColor   = c555;
        paletteColor_t textColor = c000;
        if ('#' == tmp[1])
        {
            bgColor   = c000;
            textColor = c555;
        }

        fillDisplayArea(0, yOff - KEY_MARGIN, keyWidth, yOff + dv->ibm.height + KEY_MARGIN, bgColor);

        drawText(&dv->ibm, textColor, tmp, KEY_MARGIN, yOff);
        yOff += dv->ibm.height;
        yOff += KEY_MARGIN;
        drawLineFast(0, yOff, TFT_WIDTH, yOff, c222);
        yOff += KEY_MARGIN + 1;
    }

    int32_t xOff = keyWidth;
    int32_t lIdx = 0;
    while (xOff < TFT_WIDTH)
    {
        paletteColor_t lineColor = c222;
        if (0 == lIdx % 4)
        {
            lineColor = c333;
        }
        drawLineFast(xOff, 0, xOff, TFT_HEIGHT, lineColor);
        xOff += keyWidth;
        lIdx++;
    }
}

/**
 * This function is called when the display driver wishes to update a
 * section of the display.
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void sequencerBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
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
static void sequencerMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    printf("%s %s\n", label, selected ? "selected" : "scrolled to");

    if (selected)
    {
    }
}
