/**
 * @file jukebox.c
 * @author Brycey92
 * @brief Equivalent to a sound-test mode from older games
 * @date 2023-08-26
 */

/*==============================================================================
 * Includes
 *============================================================================*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "hdw-tft.h"
#include "mainMenu.h"
#include "portableDance.h"
#include "tunernome.h"
#include "settingsManager.h"
#include "swadge2024.h"

#include "jukebox.h"
#include "menu.h"

/*==============================================================================
 * Defines
 *============================================================================*/

#define CORNER_OFFSET           14
#define JUKEBOX_SPRITE_Y_OFFSET 3
#define LINE_BREAK_Y            8

/*==============================================================================
 * Enums
 *============================================================================*/

// The state data
typedef enum
{
    JUKEBOX_MENU,
    JUKEBOX_PLAYER
} jukeboxScreen_t;

/*==============================================================================
 * Prototypes
 *============================================================================*/

void jukeboxEnterMode(void);
void jukeboxExitMode(void);
void jukeboxButtonCallback(buttonEvt_t* evt);
void jukeboxMainLoop(int64_t elapsedUs);
void jukeboxMainMenuCb(const char* label, bool selected, uint32_t settingVal);
void jukeboxBackgroundDrawCb(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

void setJukeboxMainMenu(bool resetPos);

/*==============================================================================
 * Structs
 *============================================================================*/

typedef struct
{
    // Menu
    menu_t* menu;
    menuLogbookRenderer_t* menuLogbookRenderer;

    // Fonts
    font_t ibm_vga8;
    font_t radiostars;
    font_t logbook;

    // WSGs
    wsg_t arrow;
    wsg_t jukeboxSprite;

    // Midis
    song_t pongHit1;
    song_t pongHit2;
    song_t pongBgm;

    // Touch
    int32_t touchAngle;
    int32_t touchRadius;
    int32_t touchIntensity;

    // Light Dances
    portableDance_t* portableDances;

    // Jukebox Stuff
    uint8_t categoryIdx;
    uint8_t songIdx;
    bool inMusicSubmode;

    jukeboxScreen_t screen;
} jukebox_t;

jukebox_t* jukebox;

typedef struct
{
    char* name;
    song_t* song;
} jukeboxSong;

typedef struct
{
    char* categoryName;
    uint8_t numSongs;
    const jukeboxSong* songs;
} jukeboxCategory;

/*==============================================================================
 * Variables
 *============================================================================*/

swadgeMode_t jukeboxMode = {
    .modeName                 = "Jukebox",
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = jukeboxEnterMode,
    .fnExitMode               = jukeboxExitMode,
    .fnMainLoop               = jukeboxMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = jukeboxBackgroundDrawCb,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

/*==============================================================================
 * Const Variables
 *============================================================================*/

// Text
static const char str_jukebox[]    = "Jukebox";
static const char str_bgm_muted[]  = "Swadge music is muted!";
static const char str_sfx_muted[]  = "Swadge SFX are muted!";
static const char str_bgm[]        = "Music";
static const char str_sfx[]        = "SFX";
static const char str_exit[]       = "Exit";
static const char str_leds[]       = "Sel: LEDs:";
static const char str_back[]       = "Start: Back";
static const char str_brightness[] = "Touch: LED Brightness:";
static const char str_stop[]       = ": Stop";
static const char str_play[]       = ": Play";

// Arrays

// TODO: get these midis from Dac
// static const jukeboxSong jukeboxMusic[] =
// {
//     {.name = "Hot Rod", .song = &hotrod},
//     {.name = "Fauxrio Kart", .song = &fauxrio_kart},
//     {.name = "The Lake", .song = &the_lake},
//     {.name = "Ya like jazz?", .song = &herecomesthesun},
//     {.name = "Banana", .song = &bananaphone},
// };

static jukeboxSong pongMusic[] = {
    {.name = "BGM", .song = NULL},
};

static const jukeboxCategory musicCategories[] = {
    //{.categoryName = "Jukebox", .numSongs = ARRAY_SIZE(jukeboxMusic), .songs = jukeboxMusic},
    {.categoryName = "Pong", .numSongs = ARRAY_SIZE(pongMusic), .songs = pongMusic},
};

static jukeboxSong pongSfx[] = {
    {.name = "Block 1", .song = NULL},
    {.name = "Block 2", .song = NULL},
};

static const jukeboxSong tunernomeSfx[] = {
    {.name = "Primary", .song = &metronome_primary},
    {.name = "Secondary", .song = &metronome_secondary},
};

static const jukeboxCategory sfxCategories[] = {
    {.categoryName = "Pong", .numSongs = ARRAY_SIZE(pongSfx), .songs = pongSfx},
    {.categoryName = "Tunernome", .numSongs = ARRAY_SIZE(tunernomeSfx), .songs = tunernomeSfx},
};

/*============================================================================
 * Functions
 *==========================================================================*/

/**
 * Initializer for jukebox
 */
void jukeboxEnterMode()
{
    // Allocate zero'd memory for the mode
    jukebox = calloc(1, sizeof(jukebox_t));

    // Load fonts
    loadFont("ibm_vga8.font", &jukebox->ibm_vga8, false);
    loadFont("radiostars.font", &jukebox->radiostars, false);
    loadFont("logbook.font", &jukebox->logbook, false);

    // Load images
    loadWsg("arrow12.wsg", &jukebox->arrow, false);
    loadWsg("jukebox.wsg", &jukebox->jukeboxSprite, false);

    // Load midis
    loadSong("block1.sng", &jukebox->pongHit1, false);
    loadSong("block2.sng", &jukebox->pongHit2, false);
    loadSong("stereo.sng", &jukebox->pongBgm, false);
    jukebox->pongBgm.shouldLoop = true;
    pongMusic[0].song           = &jukebox->pongBgm;
    pongSfx[0].song             = &jukebox->pongHit1;
    pongSfx[1].song             = &jukebox->pongHit2;

    // Initialize menu
    jukebox->menu                = initMenu(str_jukebox, &jukeboxMainMenuCb);
    jukebox->menuLogbookRenderer = initMenuLogbookRenderer(&jukebox->logbook);
    addSingleItemToMenu(jukebox->menu, str_bgm);
    addSingleItemToMenu(jukebox->menu, str_sfx);
    addSingleItemToMenu(jukebox->menu, str_exit);
    jukebox->screen = JUKEBOX_MENU;

    jukebox->portableDances = initPortableDance(NULL);

    // Disable Comet {R,G,B}, Rise {R,G,B}, Pulse {R,G,B}, and Fire {G,B}
    portableDanceDisableDance(jukebox->portableDances, "Comet R");
    portableDanceDisableDance(jukebox->portableDances, "Comet G");
    portableDanceDisableDance(jukebox->portableDances, "Comet B");
    portableDanceDisableDance(jukebox->portableDances, "Rise R");
    portableDanceDisableDance(jukebox->portableDances, "Rise G");
    portableDanceDisableDance(jukebox->portableDances, "Rise B");
    portableDanceDisableDance(jukebox->portableDances, "Pulse R");
    portableDanceDisableDance(jukebox->portableDances, "Pulse G");
    portableDanceDisableDance(jukebox->portableDances, "Pulse B");
    portableDanceDisableDance(jukebox->portableDances, "Fire G");
    portableDanceDisableDance(jukebox->portableDances, "Fire B");
    portableDanceDisableDance(jukebox->portableDances, "Flashlight");

    bzrStop(true);
}

/**
 * Called when jukebox is exited
 */
void jukeboxExitMode(void)
{
    bzrStop(true);

    freeFont(&jukebox->ibm_vga8);
    freeFont(&jukebox->radiostars);
    freeFont(&jukebox->logbook);

    freeWsg(&jukebox->arrow);
    freeWsg(&jukebox->jukeboxSprite);

    freeSong(&jukebox->pongHit1);
    freeSong(&jukebox->pongHit2);
    freeSong(&jukebox->pongBgm);

    freePortableDance(jukebox->portableDances);

    deinitMenuLogbookRenderer(jukebox->menuLogbookRenderer);
    deinitMenu(jukebox->menu);

    free(jukebox);
}

/**
 * @brief Button callback function
 *
 * @param evt The button event that occurred
 */
void jukeboxButtonCallback(buttonEvt_t* evt)
{
    if (!evt->down)
    {
        return;
    }

    switch (evt->button)
    {
        case PB_A:
        {
            if (jukebox->inMusicSubmode)
            {
                bzrPlayBgm(musicCategories[jukebox->categoryIdx].songs[jukebox->songIdx].song, BZR_STEREO);
            }
            else
            {
                bzrPlaySfx(sfxCategories[jukebox->categoryIdx].songs[jukebox->songIdx].song, BZR_STEREO);
            }
            break;
        }
        case PB_B:
        {
            bzrStop(true);
            break;
        }
        case PB_SELECT:
        {
            portableDanceNext(jukebox->portableDances);
            break;
        }
        case PB_START:
        {
            bzrStop(true);
            jukebox->screen = JUKEBOX_MENU;
            break;
        }
        case PB_UP:
        {
            bzrStop(true);
            uint8_t length;
            if (jukebox->inMusicSubmode)
            {
                length = ARRAY_SIZE(musicCategories);
            }
            else
            {
                length = ARRAY_SIZE(sfxCategories);
            }

            if (jukebox->categoryIdx == 0)
            {
                jukebox->categoryIdx = length;
            }
            jukebox->categoryIdx = jukebox->categoryIdx - 1;

            jukebox->songIdx = 0;
            break;
        }
        case PB_DOWN:
        {
            bzrStop(true);
            uint8_t length;
            if (jukebox->inMusicSubmode)
            {
                length = ARRAY_SIZE(musicCategories);
            }
            else
            {
                length = ARRAY_SIZE(sfxCategories);
            }

            jukebox->categoryIdx = (jukebox->categoryIdx + 1) % length;

            jukebox->songIdx = 0;
            break;
        }
        case PB_LEFT:
        {
            bzrStop(true);
            uint8_t length;
            if (jukebox->inMusicSubmode)
            {
                length = musicCategories[jukebox->categoryIdx].numSongs;
            }
            else
            {
                length = sfxCategories[jukebox->categoryIdx].numSongs;
            }

            if (jukebox->songIdx == 0)
            {
                jukebox->songIdx = length;
            }
            jukebox->songIdx = jukebox->songIdx - 1;
            break;
        }
        case PB_RIGHT:
        {
            bzrStop(true);
            uint8_t length;
            if (jukebox->inMusicSubmode)
            {
                length = musicCategories[jukebox->categoryIdx].numSongs;
            }
            else
            {
                length = sfxCategories[jukebox->categoryIdx].numSongs;
            }

            jukebox->songIdx = (jukebox->songIdx + 1) % length;
            break;
        }
        default:
        {
            break;
        }
    }
}

/**
 * Update the display by drawing the current state of affairs
 */
void jukeboxMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};

    switch (jukebox->screen)
    {
        case JUKEBOX_MENU:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    jukebox->menu = menuButton(jukebox->menu, evt);
                }
            }
            clearPxTft();
            drawMenuLogbook(jukebox->menu, jukebox->menuLogbookRenderer, elapsedUs);
            break;
        }
        case JUKEBOX_PLAYER:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                jukeboxButtonCallback(&evt);
            }

            if (getTouchJoystick(&jukebox->touchAngle, &jukebox->touchRadius, &jukebox->touchIntensity))
            {
                setLedBrightnessSetting((jukebox->touchAngle * (MAX_LED_BRIGHTNESS + 1)) / 360);
            }

            portableDanceMainLoop(jukebox->portableDances, elapsedUs);

            // Plot jukebox sprite
            int16_t spriteWidth = jukebox->jukeboxSprite.w;
            drawWsg(&jukebox->jukeboxSprite, (TFT_WIDTH - spriteWidth) / 2,
                    TFT_HEIGHT - jukebox->jukeboxSprite.h - JUKEBOX_SPRITE_Y_OFFSET, false, false, 0);

            // Plot the button funcs
            // LEDs
            drawText(&jukebox->radiostars, c555, str_leds, CORNER_OFFSET, CORNER_OFFSET);
            // Light dance name
            drawText(&(jukebox->radiostars), c111, portableDanceGetName(jukebox->portableDances),
                     TFT_WIDTH - CORNER_OFFSET
                         - textWidth(&jukebox->radiostars, portableDanceGetName(jukebox->portableDances)),
                     CORNER_OFFSET);

            // Back
            drawText(&jukebox->radiostars, c555, str_back, CORNER_OFFSET,
                     CORNER_OFFSET + LINE_BREAK_Y + jukebox->radiostars.height);

            // LED Brightness
            drawText(&jukebox->radiostars, c555, str_brightness, CORNER_OFFSET,
                     CORNER_OFFSET + (LINE_BREAK_Y + jukebox->radiostars.height) * 2);
            char text[32];
            snprintf(text, sizeof(text), "%d", getLedBrightnessSetting());
            drawText(&jukebox->radiostars, c111, text,
                     TFT_WIDTH - textWidth(&jukebox->radiostars, text) - CORNER_OFFSET,
                     CORNER_OFFSET + (LINE_BREAK_Y + jukebox->radiostars.height) * 2);

            // Stop
            int16_t afterText = drawText(&jukebox->radiostars, c511, "B", CORNER_OFFSET,
                                         TFT_HEIGHT - jukebox->radiostars.height - CORNER_OFFSET);
            drawText(&jukebox->radiostars, c555, str_stop, afterText,
                     TFT_HEIGHT - jukebox->radiostars.height - CORNER_OFFSET);

            // Play
            afterText = drawText(&jukebox->radiostars, c151, "A",
                                 TFT_WIDTH - textWidth(&jukebox->radiostars, str_play)
                                     - textWidth(&jukebox->radiostars, "A") - CORNER_OFFSET,
                                 TFT_HEIGHT - jukebox->radiostars.height - CORNER_OFFSET);
            drawText(&jukebox->radiostars, c555, str_play, afterText,
                     TFT_HEIGHT - jukebox->radiostars.height - CORNER_OFFSET);

            char* categoryName;
            char* songName;
            char* songTypeName;
            uint8_t numSongs;
            bool drawNames = false;
            if (jukebox->inMusicSubmode)
            {
                // Warn the user that the swadge is muted, if that's the case
                if (getBgmVolumeSetting() == getBgmVolumeSettingBounds()->min)
                {
                    drawText(&jukebox->radiostars, c551, str_bgm_muted,
                             (TFT_WIDTH - textWidth(&jukebox->radiostars, str_bgm_muted)) / 2, TFT_HEIGHT / 2);
                }
                else
                {
                    categoryName = musicCategories[jukebox->categoryIdx].categoryName;
                    songName     = musicCategories[jukebox->categoryIdx].songs[jukebox->songIdx].name;
                    songTypeName = "Music";
                    numSongs     = musicCategories[jukebox->categoryIdx].numSongs;
                    drawNames    = true;
                }
            }
            else
            {
                // Warn the user that the swadge is muted, if that's the case
                if (getSfxVolumeSetting() == getSfxVolumeSettingBounds()->min)
                {
                    drawText(&jukebox->radiostars, c551, str_sfx_muted,
                             (TFT_WIDTH - textWidth(&jukebox->radiostars, str_sfx_muted)) / 2, TFT_HEIGHT / 2);
                }
                else
                {
                    categoryName = sfxCategories[jukebox->categoryIdx].categoryName;
                    songName     = sfxCategories[jukebox->categoryIdx].songs[jukebox->songIdx].name;
                    songTypeName = "SFX";
                    numSongs     = sfxCategories[jukebox->categoryIdx].numSongs;
                    drawNames    = true;
                }
            }

            if (drawNames)
            {
                // Draw the mode name
                snprintf(text, sizeof(text), "Mode: %s", categoryName);
                int16_t width = textWidth(&(jukebox->radiostars), text);
                int16_t yOff  = (TFT_HEIGHT - jukebox->radiostars.height) / 2 - jukebox->radiostars.height * 0;
                drawText(&(jukebox->radiostars), c313, text, (TFT_WIDTH - width) / 2, yOff);
                // Draw category arrows if this submode has more than 1 category
                if ((jukebox->inMusicSubmode && ARRAY_SIZE(musicCategories) > 1) || ARRAY_SIZE(sfxCategories) > 1)
                {
                    drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) - 8 - jukebox->arrow.w, yOff, false, false, 0);
                    drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) + width + 8, yOff, false, false, 180);
                }

                // Draw the song name
                snprintf(text, sizeof(text), "%s: %s", songTypeName, songName);
                yOff  = (TFT_HEIGHT - jukebox->radiostars.height) / 2 + jukebox->radiostars.height * 2.5f;
                width = textWidth(&(jukebox->radiostars), text);
                drawText(&(jukebox->radiostars), c113, text, (TFT_WIDTH - width) / 2, yOff);
                // Draw song arrows if this category has more than 1 song
                if (numSongs > 1)
                {
                    drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) - 8 - jukebox->arrow.w, yOff, false, false, 270);
                    drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) + width + 8, yOff, false, false, 90);
                }
            }
            break;
        }
    }
}

void jukeboxMainMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (!selected)
    {
        return;
    }

    if (label == str_bgm)
    {
        jukebox->screen         = JUKEBOX_PLAYER;
        jukebox->categoryIdx    = 0;
        jukebox->songIdx        = 0;
        jukebox->inMusicSubmode = true;
    }
    else if (label == str_sfx)
    {
        jukebox->screen         = JUKEBOX_PLAYER;
        jukebox->categoryIdx    = 0;
        jukebox->songIdx        = 0;
        jukebox->inMusicSubmode = false;
    }
    else if (label == str_exit)
    {
        switchToSwadgeMode(&mainMenuMode);
        return;
    }
}

/**
 * @brief Draw a portion of the background when requested
 *
 * @param disp The display to draw to
 * @param x The X offset to draw
 * @param y The Y offset to draw
 * @param w The width to draw
 * @param h The height to draw
 * @param up The current number of the update call
 * @param upNum The total number of update calls for this frame
 */
void jukeboxBackgroundDrawCb(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    switch (jukebox->screen)
    {
        case JUKEBOX_MENU:
        {
            // The menu draw function in the main loop handles this already
            break;
        }
        case JUKEBOX_PLAYER:
        {
            // Use TURBO drawing mode to draw individual pixels fast
            SETUP_FOR_TURBO();

            // Draw a grid
            for (int16_t yp = y; yp < y + h; yp++)
            {
                for (int16_t xp = x; xp < x + w; xp++)
                {
                    TURBO_SET_PIXEL(xp, yp, c234);
                }
            }
            break;
        }
    }
}
