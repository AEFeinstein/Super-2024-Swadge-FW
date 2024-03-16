#include "introMode.h"

#include <esp_log.h>

#include "autoLoader.h"
#include "hashMap.h"
#include "macros.h"
#include "menu.h"

/*

    iv->ibm = autoLoadFont(iv->loader, "ibm_vga8.font");
    autoLoadFont(iv->loader, "radiostars.font");
    autoLoadFont(iv->loader, "eightbit_atari_grube2.font");
    autoLoadFont(iv->loader, "seven_segment.font");
    autoLoadFont(iv->loader, "dylwhich_sans.font");
*/

static const char* font_names[] = {
    "ibm_vga8.font",
    "seven_segment.font",
    "eightbit_atari_grube2.font",
    "seven_segment.font",
    "dylwhich_sans.font",
    "logbook.font",
};

static void introEnterMode(void);
static void introExitMode(void);
static void introMainLoop(int64_t elapsedUs);
static void introAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void introBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void introMenuCb(const char*, bool selected, uint32_t settingVal);

static const char introName[]  = "Intro";

swadgeMode_t introMode = {
    .modeName                 = introName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = introEnterMode,
    .fnExitMode               = introExitMode,
    .fnMainLoop               = introMainLoop,
    .fnAudioCallback          = introAudioCallback,
    .fnBackgroundDrawCallback = introBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

// A struct to use for the key
typedef struct
{
    int32_t x, y;
} keyStruct_t;

typedef struct
{
    bool inMenu;
    int fontIndex;

    font_t* ibm;
    wsg_t king_donut;
    song_t ode_to_joy;

    hashMap_t map;
    keyStruct_t keys[5];

    autoLoader_t* loader;

    menu_t* menu;
    menuLogbookRenderer_t* menuLogbookRenderer;

} introVars_t;

static introVars_t* iv;

int32_t hashStruct(const void* data)
{
    // Helper function provided with hash map
    return hashBytes((const uint8_t*)data, sizeof(keyStruct_t));
}

bool structEq(const void* a, const void* b)
{
    // Another hash map helper function
    return bytesEq((const uint8_t*)a, sizeof(keyStruct_t), (const uint8_t*)b, sizeof(keyStruct_t));
}

/**
 * This function is called when this mode is started. It should initialize
 * variables and start the mode.
 */
static void introEnterMode(void)
{
    iv = calloc(1, sizeof(introVars_t));

    iv->loader = initAutoLoader(false);

    for (const char** font = font_names; font < (font_names + ARRAY_SIZE(font_names)); font++)
    {
        autoLoadFont(iv->loader, *font);
    }

    hashInitBin(&iv->map, 10, hashStruct, structEq);

    iv->keys[0].x = 10;
    iv->keys[0].y = 16;

    iv->keys[1].x = 5;
    iv->keys[1].y = 5;

    iv->keys[2].x = 7;
    iv->keys[2].y = 12;

    iv->keys[3].x = 1;
    iv->keys[3].y = 3;

    iv->keys[4].x = 16;
    iv->keys[4].y = 4;

    hashPutBin(&iv->map, &iv->keys[0], "Star");
    hashPutBin(&iv->map, &iv->keys[1], "Square");
    hashPutBin(&iv->map, &iv->keys[2], "Circle");
    hashPutBin(&iv->map, &iv->keys[3], "Diamond");
    hashPutBin(&iv->map, &iv->keys[4], "Triangle");

    /*autoLoadFont(iv->loader, "radiostars.font");
    autoLoadFont(iv->loader, "eightbit_atari_grube2.font");
    autoLoadFont(iv->loader, "seven_segment.font");
    autoLoadFont(iv->loader, "dylwhich_sans.font");*/

    iv->ibm = autoLoadFont(iv->loader, "ibm_vga8.font");
    //loadFont("ibm_vga8.font", &iv->ibm, false);

    //loadWsg("kid0.wsg", &iv->king_donut, true);
    //loadSong("ode.sng", &iv->ode_to_joy, true);

    //bzrPlayBgm(&iv->ode_to_joy, BZR_STEREO);
    //bzrStop(true);

    iv->menu = initMenu(introName, introMenuCb);
    iv->menuLogbookRenderer = initMenuLogbookRenderer(autoLoadFont(iv->loader, "logbook.font"));
    iv->inMenu = true;
}

void runStructKeyExample(void)
{
    static int x = 0;
    static int y = 0;

    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button == PB_UP)
            {
                y++;
            }
            else if (evt.button == PB_DOWN)
            {
                y--;
            }
            else if (evt.button == PB_LEFT)
            {
                x--;
            }
            else if (evt.button == PB_RIGHT)
            {
                x++;
            }
            else if (evt.button == PB_A)
            {
                keyStruct_t key;
                key.x = x;
                key.y = y;
                const char* found = hashGetBin(&iv->map, &key);
                if (found)
                {
                    printf("Found a %s at %d, %d!\n", found, x, y);
                }
                else
                {
                    printf("Didn't find anything at %d, %d :(\n", x, y);
                }
            }
        }
    }
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void introExitMode(void)
{
    //freeFont(&iv->ibm);
    deinitAutoLoader(iv->loader);
    deinitMenu(iv->menu);
    deinitMenuLogbookRenderer(iv->menuLogbookRenderer);
    free(iv);
}

/**
 * This function is called from the main loop. It's pretty quick, but the
 * timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
static void introMainLoop(int64_t elapsedUs)
{
    clearPxTft();

    runStructKeyExample();
    return;

    // Process button events
    buttonEvt_t evt              = {0};
    static uint32_t lastBtnState = 0;
    while (checkButtonQueueWrapper(&evt))
    {
        lastBtnState = evt.state;

        if (iv->inMenu)
        {
            if (iv->menu->currentItem)
            {
                iv->menu = menuButton(iv->menu, evt);
            }

            if (evt.button == PB_A && evt.down)
            {
                iv->inMenu = false;
            }
            else if (evt.button == PB_B && evt.down)
            {
                iv->fontIndex = (iv->fontIndex + 1) % ARRAY_SIZE(font_names);
                iv->menuLogbookRenderer->font = autoLoadFont(iv->loader, font_names[iv->fontIndex]);
            }
        }
        else if (evt.button == PB_A && evt.down)
        {
            iv->inMenu = true;
        }
    }

    if (iv->inMenu)
    {
        drawMenuLogbook(iv->menu, iv->menuLogbookRenderer, elapsedUs);
        return;
    }


    // Fill the display area with a dark cyan
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c123);
}

/**
 * This function is called whenever audio samples are read from the
 * microphone (ADC) and are ready for processing. Samples are read at 8KHz
 * This cannot be used at the same time as fnBatteryCallback
 *
 * @param samples A pointer to 12 bit audio samples
 * @param sampleCnt The number of samples read
 */
static void introAudioCallback(uint16_t* samples, uint32_t sampleCnt)
{
    ;
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
static void introBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
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
static void introMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    printf("%s %s\n", label, selected ? "selected" : "scrolled to");

    if (selected)
    {
    }
}
