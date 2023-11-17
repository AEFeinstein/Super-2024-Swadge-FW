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
#include "breakout.h"
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
 * Structs
 *============================================================================*/

typedef struct
{
    char* name;
    song_t song;
} jukeboxSong;

typedef struct
{
    const char* categoryName;
    uint8_t numSongs;
    jukeboxSong* songs;
} jukeboxCategory;

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

    // Music Midis and Jukebox Structs
    jukeboxCategory* musicCategories;
    uint8_t numMusicCategories;

    // SFX Midis and Jukebox Structs
    jukeboxCategory* tunernomeSfxCategory;
    jukeboxCategory* sfxCategories;
    uint8_t numSfxCategories;

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
    bool isPlaying;
} jukebox_t;

jukebox_t* jukebox;

/*==============================================================================
 * Prototypes
 *============================================================================*/

void jukeboxEnterMode(void);
void jukeboxExitMode(void);
void jukeboxButtonCallback(buttonEvt_t* evt);
void jukeboxMainLoop(int64_t elapsedUs);
void jukeboxMainMenuCb(const char* label, bool selected, uint32_t settingVal);
void jukeboxBackgroundDrawCb(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

void jukeboxFreeCategories(jukeboxCategory** categoryArray, uint8_t numCategories);

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
static const char str_leds[]       = "B: LEDs:";
static const char str_back[]       = "Pause: Back";
static const char str_brightness[] = "Touch: LED Brightness:";
static const char str_stop[]       = ": Stop";
static const char str_play[]       = ": Play";

// Arrays

/*============================================================================
 * Functions
 *==========================================================================*/

/**
 * Initializer for jukebox
 */
void jukeboxEnterMode()
{
    ///// Allocate zero'd memory for the mode /////
    jukebox = calloc(1, sizeof(jukebox_t));

    ///// Load fonts /////
    loadFont("ibm_vga8.font", &jukebox->ibm_vga8, false);
    loadFont("radiostars.font", &jukebox->radiostars, false);
    loadFont("logbook.font", &jukebox->logbook, false);

    ///// Load images /////
    loadWsg("arrow12.wsg", &jukebox->arrow, false);
    loadWsg("jukebox.wsg", &jukebox->jukeboxSprite, false);

    ///// Load music midis /////

    // Initialize array of music categories
    int catIdx = 0;
    jukebox->numMusicCategories = 2;
    // TODO: change this number   ^ as categories (modes) with music are added
    jukebox->musicCategories = calloc(jukebox->numMusicCategories, sizeof(jukeboxCategory));

    // Breakout category
    int songIdx = 0;
    jukebox->musicCategories[catIdx].categoryName = breakoutMode.modeName;
    jukebox->musicCategories[catIdx].numSongs = 1;
    jukebox->musicCategories[catIdx].songs = calloc(jukebox->musicCategories[catIdx].numSongs, sizeof(jukeboxSong));

    // Breakout songs

    jukebox->musicCategories[catIdx].songs[songIdx].name = "BGM";
    loadSong("gmcc.sng", &jukebox->musicCategories[catIdx].songs[songIdx].song, false); // TODO: use correct sng once it's done being composed
    jukebox->musicCategories[catIdx].songs[songIdx].song.shouldLoop = true;
    songIdx++;

    catIdx++;

    // Jukebox category
    songIdx = 0;
    jukebox->musicCategories[catIdx].categoryName = jukeboxMode.modeName;
    jukebox->musicCategories[catIdx].numSongs = 5;
    jukebox->musicCategories[catIdx].songs = calloc(jukebox->musicCategories[catIdx].numSongs, sizeof(jukeboxSong));

    // Jukebox songs

    // TODO: get these midis from Dac
    // jukebox->musicCategories[catIdx].songs[songIdx].name = "Hot Rod";
    // jukebox->musicCategories[catIdx].songs[songIdx].song = &jukebox->hotrod;
    // songIdx++;
    
    // jukebox->musicCategories[catIdx].songs[songIdx].name = "Fauxrio Kart";
    // jukebox->musicCategories[catIdx].songs[songIdx].song = &jukebox->fauxrio_kart;
    // songIdx++;

    // jukebox->musicCategories[catIdx].songs[songIdx].name = "The Lake";
    // jukebox->musicCategories[catIdx].songs[songIdx].song = &jukebox->the_lake;
    // songIdx++;

    // jukebox->musicCategories[catIdx].songs[songIdx].name = "Ya like jazz?";
    // jukebox->musicCategories[catIdx].songs[songIdx].song = &jukebox->herecomesthesun;
    // songIdx++;

    // jukebox->musicCategories[catIdx].songs[songIdx].name = "Banana";
    // jukebox->musicCategories[catIdx].songs[songIdx].song = &jukebox->bananaphone;
    // songIdx++;

    catIdx++;

    // TODO: remainder of categories (modes) with music

    ///// Load SFX midis /////

    // Initialize array of music categories
    catIdx = 0;
    jukebox->numSfxCategories = 2;
    // TODO: change this number ^ as categories (modes) with SFX are added
    jukebox->sfxCategories = calloc(2, sizeof(jukeboxCategory));

    // Breakout category
    songIdx = 0;
    jukebox->sfxCategories[catIdx].categoryName = breakoutMode.modeName;
    jukebox->sfxCategories[catIdx].numSongs = 10;
    jukebox->sfxCategories[catIdx].songs = calloc(jukebox->sfxCategories[catIdx].numSongs, sizeof(jukeboxSong));

    // Breakout SFX

    jukebox->sfxCategories[catIdx].songs[songIdx].name = "Bounce";
    loadSong("sndBounce.sng", &jukebox->sfxCategories[catIdx].songs[songIdx].song, false);
    jukebox->sfxCategories[catIdx].songs[songIdx].song.shouldLoop = false;
    songIdx++;

    jukebox->sfxCategories[catIdx].songs[songIdx].name = "Break";
    loadSong("sndBreak.sng", &jukebox->sfxCategories[catIdx].songs[songIdx].song, false);
    jukebox->sfxCategories[catIdx].songs[songIdx].song.shouldLoop = false;
    songIdx++;

    // TODO: remainder of breakout SFX

    catIdx++;
    
    // Tunernome category
    songIdx = 0;
    jukebox->sfxCategories[catIdx].categoryName = tunernomeMode.modeName;
    jukebox->sfxCategories[catIdx].numSongs = 2;
    jukebox->sfxCategories[catIdx].songs = calloc(jukebox->sfxCategories[catIdx].numSongs, sizeof(jukeboxSong));
    // Special handling so we can easily clean this up later
    jukebox->tunernomeSfxCategory = &jukebox->sfxCategories[catIdx];

    // Tunernome SFX

    // Special handling since these SFX are defined in code, not loaded from .sng files
    jukebox->sfxCategories[catIdx].songs[songIdx].name = "Primary";
    memcpy(&jukebox->sfxCategories[catIdx].songs[songIdx].song, &metronome_primary, sizeof(song_t));
    songIdx++;

    // Special handling since these SFX are defined in code, not loaded from .sng files
    jukebox->sfxCategories[catIdx].songs[songIdx].name = "Secondary";
    memcpy(&jukebox->sfxCategories[catIdx].songs[songIdx].song, &metronome_secondary, sizeof(song_t));
    songIdx++;

    catIdx++;

    // TODO: remainder of categories (modes) with SFX

    ///// Initialize menu /////
    jukebox->menu                = initMenu(str_jukebox, &jukeboxMainMenuCb);
    jukebox->menuLogbookRenderer = initMenuLogbookRenderer(&jukebox->logbook);
    addSingleItemToMenu(jukebox->menu, str_bgm);
    addSingleItemToMenu(jukebox->menu, str_sfx);
    addSingleItemToMenu(jukebox->menu, str_exit);
    jukebox->screen = JUKEBOX_MENU;

    ///// Initialize portable dances /////

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

    // Free fonts
    freeFont(&jukebox->ibm_vga8);
    freeFont(&jukebox->radiostars);
    freeFont(&jukebox->logbook);

    // Free images
    freeWsg(&jukebox->arrow);
    freeWsg(&jukebox->jukeboxSprite);

    // Free allocated music midis, song arrays, and category arrays
    jukeboxFreeCategories(&jukebox->musicCategories, jukebox->numMusicCategories);

    // Free allocated SFX midis, song arrays, and category arrays

    // Tunernome's SFX are declared in code, not loaded as .sng files, so we need to prevent them from being freed
    for(int i = 0; i < jukebox->tunernomeSfxCategory->numSongs; i++)
    {
        memset(&jukebox->tunernomeSfxCategory->songs[i].song, 0, sizeof(song_t));
    }

    jukeboxFreeCategories(&jukebox->sfxCategories, jukebox->numSfxCategories);

    // Free dances and menu

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
            if (jukebox->isPlaying)
            {
                bzrStop(true);
                jukebox->isPlaying = false;
            }
            else
            {
                if (jukebox->inMusicSubmode)
                {
                    bzrPlayBgm(&jukebox->musicCategories[jukebox->categoryIdx].songs[jukebox->songIdx].song, BZR_STEREO);
                }
                else
                {
                    bzrPlaySfx(&jukebox->sfxCategories[jukebox->categoryIdx].songs[jukebox->songIdx].song, BZR_STEREO);
                }
                jukebox->isPlaying = true;
            }
            break;
        }
        case PB_B:
        {
            portableDanceNext(jukebox->portableDances);
            break;
        }
        case PB_SELECT:
        {
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
                length = jukebox->numMusicCategories;
            }
            else
            {
                length = jukebox->numSfxCategories;
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
                length = jukebox->numMusicCategories;
            }
            else
            {
                length = jukebox->numSfxCategories;
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
                length = jukebox->musicCategories[jukebox->categoryIdx].numSongs;
            }
            else
            {
                length = jukebox->sfxCategories[jukebox->categoryIdx].numSongs;
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
                length = jukebox->musicCategories[jukebox->categoryIdx].numSongs;
            }
            else
            {
                length = jukebox->sfxCategories[jukebox->categoryIdx].numSongs;
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

            // Assume not playing
            paletteColor_t color = c141;
            const char* btnText  = str_play;
            if (jukebox->isPlaying)
            {
                color   = c511;
                btnText = str_stop;
            }

            // Draw A text (play or stop)
            int16_t afterText = drawText(&jukebox->radiostars, color, "A",
                                         TFT_WIDTH - textWidth(&jukebox->radiostars, btnText)
                                             - textWidth(&jukebox->radiostars, "A") - CORNER_OFFSET,
                                         TFT_HEIGHT - jukebox->radiostars.height - CORNER_OFFSET);
            drawText(&jukebox->radiostars, c555, btnText, afterText,
                     TFT_HEIGHT - jukebox->radiostars.height - CORNER_OFFSET);

            const char* categoryName;
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
                    categoryName = jukebox->musicCategories[jukebox->categoryIdx].categoryName;
                    songName     = jukebox->musicCategories[jukebox->categoryIdx].songs[jukebox->songIdx].name;
                    songTypeName = "Music";
                    numSongs     = jukebox->musicCategories[jukebox->categoryIdx].numSongs;
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
                    categoryName = jukebox->sfxCategories[jukebox->categoryIdx].categoryName;
                    songName     = jukebox->sfxCategories[jukebox->categoryIdx].songs[jukebox->songIdx].name;
                    songTypeName = "SFX";
                    numSongs     = jukebox->sfxCategories[jukebox->categoryIdx].numSongs;
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
                if ((jukebox->inMusicSubmode && jukebox->numMusicCategories > 1) || jukebox->numSfxCategories > 1)
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

void jukeboxFreeCategories(jukeboxCategory** categoryArray, uint8_t numCategories)
{
    for(int categoryIdx = 0; categoryIdx < numCategories; categoryIdx++)
    {
        for(int songIdx = 0; songIdx < categoryArray[categoryIdx]->numSongs; songIdx++)
        {
            // Avoid freeing songs we never loaded
            if(categoryArray[categoryIdx]->songs[songIdx].song.tracks != NULL)
            {
                freeSong(&categoryArray[categoryIdx]->songs[songIdx].song);
            }
        }

        free(categoryArray[categoryIdx]->songs);
        free(categoryArray[categoryIdx]);
    }

    free(categoryArray);
}
