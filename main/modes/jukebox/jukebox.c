/**
 * @file jukebox.c
 * @author Brycey92 & VanillyNeko
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
#include "mode_platformer.h"
#include "portableDance.h"
#include "mode_ray.h"
#include "tunernome.h"
#include "settingsManager.h"
#include "swadge2024.h"

#include "jukebox.h"

/*==============================================================================
 * Defines
 *============================================================================*/

#define CORNER_OFFSET           14
#define JUKEBOX_SPRITE_Y_OFFSET 3
#define LINE_BREAK_Y            8

/*==============================================================================
 * Enums
 *============================================================================*/

// Nobody here but us chickens!

/*==============================================================================
 * Structs
 *============================================================================*/

typedef struct
{
    const char* fname;
    const char* name;
    song_t song;
} jukeboxSong_t;

typedef struct
{
    const char* catName;
    const jukeboxSong_t* songs;
} jukeboxCategory_t;

typedef struct
{
    // Fonts
    font_t ibm_vga8;
    font_t radiostars;

    // WSGs
    wsg_t arrow;
    wsg_t jukeboxSprite;

    // Music Midis and Jukebox Structs
    jukeboxCategory_t* musicCategories;
    uint8_t numMusicCategories;

    // SFX Midis and Jukebox Structs
    jukeboxCategory_t* tunernomeSfxCategory;
    jukeboxCategory_t* sfxCategories;
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
void jukeboxBackgroundDrawCb(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

void jukeboxBzrDoneCb(void);
void jukeboxFreeCategories(jukeboxCategory_t** categoryArray, uint8_t numCategories);

/*==============================================================================
 * Variables
 *============================================================================*/

const char jukeboxName[] = "Jukebox";

swadgeMode_t jukeboxMode = {
    .modeName                 = jukeboxName,
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
static const char str_bgm_muted[]  = "Swadge music is muted!";
static const char str_sfx_muted[]  = "Swadge SFX are muted!";
static const char str_bgm[]        = "Music";
static const char str_sfx[]        = "SFX";
static const char str_leds[]       = "B: LEDs:";
static const char str_music_sfx[]  = "Pause: Music/SFX:";
static const char str_brightness[] = "Touch: LED Brightness:";
static const char str_stop[]       = ": Stop";
static const char str_play[]       = ": Play";

// Arrays

const jukeboxSong_t songs_galacticBrickdown[] = {
    {
        .fname = "brkBgmCrazy.sng",
        .name  = "BGM Crazy",
    },
    {
        .fname = "brkBgmFinale.sng",
        .name  = "BGM Finale",
    },
    {
        .fname = "brkBgmPixel.sng",
        .name  = "BGM Pixel",
    },        
    {
        .fname = "brkBgmSkill.sng",
        .name  = "BGM Skill",
    },        
    {
        .fname = "brkBgmTitle.sng",
        .name  = "BGM Title",
    },
};

const jukeboxSong_t songs_swadgeLand[] = {
    {
        .fname = "bgmDeMAGio.sng",
        .name  = "DeMAGio BGM",
    },
    {
        .fname = "bgmSmooth.sng",
        .name  = "Smooth BGM",
    },        
    {
        .fname = "bgmUnderground.sng",
        .name  = "Underground BGM",
    },        
    {
        .fname = "bgmCastle.sng",
        .name  = "Castle BGM",
    },        
    {
        .fname = "bgmNameEntry.sng",
        .name  = "Name Entry BGM",
    },
};

const jukeboxSong_t songs_magtroid[] = {
    {
        .fname = "base_0.sng",
        .name  = "Base 0",
    },
    {
        .fname = "base_1.sng",
        .name  = "Base 1",
    },        
    {
        .fname = "cave_0.sng",
        .name  = "Cave 0",
    },        
    {
        .fname = "cave_1.sng",
        .name  = "Cave 1",
    },        
    {
        .fname = "jungle_0.sng",
        .name  = "Jungle 0",
    },
    {
        .fname = "jungle_1.sng",
        .name  = "Jungle 1",
    },  
    {
        .fname = "ray_boss.sng",
        .name  = "Boss",
    },  
};

const jukeboxSong_t songs_jukebox[] = {
    {
        .fname = "hotrod.sng",
        .name  = "Hot Rod",
    },
    {
        .fname = "Fauxrio_Kart.sng",
        .name  = "Fauxrio Kart",
    },        
    {
        .fname = "thelake.sng",
        .name  = "The Lake",
    },        
    {
        .fname = "yalikejazz.sng",
        .name  = "Ya like jazz?",
    },        
    {
        .fname = "banana.sng",
        .name  = "Banana",
    },
};

const jukeboxCategory_t categories[] = {
    {
        .catName  = breakoutName,
        .songs = songs_galacticBrickdown
    },
    {
        .catName  = platformerName,
        .songs = songs_swadgeLand
    },
    {
        .catName  = rayName,
        .songs = songs_magtroid
    },
    {
        .catName  = jukeboxName,
        .songs = songs_jukebox
    },
};

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

    ///// Enter music submode /////
    jukebox->inMusicSubmode = true;

    ///// Load fonts /////
    loadFont("ibm_vga8.font", &jukebox->ibm_vga8, false);
    loadFont("radiostars.font", &jukebox->radiostars, false);

    ///// Load images /////
    loadWsg("arrow10.wsg", &jukebox->arrow, false);
    loadWsg("jukebox.wsg", &jukebox->jukeboxSprite, false);

    ///// Load music midis /////

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

    // Free images
    freeWsg(&jukebox->arrow);
    freeWsg(&jukebox->jukeboxSprite);

    // Free allocated music midis, song arrays, and category arrays
    jukeboxFreeCategories(&jukebox->musicCategories, jukebox->numMusicCategories);

    // Free allocated SFX midis, song arrays, and category arrays

    // Tunernome's SFX are declared in code, not loaded as .sng files, so we need to prevent them from being freed
    for (int i = 0; i < ARRAY_SIZE(jukebox->tunernomeSfxCategory); i++)
    {
        memset(&jukebox->tunernomeSfxCategory->songs[i].song, 0, sizeof(song_t));
    }

    jukeboxFreeCategories(&jukebox->sfxCategories, jukebox->numSfxCategories);

    // Free dances

    freePortableDance(jukebox->portableDances);

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
                    bzrPlayBgmCb(&jukebox->musicCategories[jukebox->categoryIdx].songs[jukebox->songIdx].song,
                                 BZR_STEREO, jukeboxBzrDoneCb);
                }
                else
                {
                    bzrPlaySfxCb(&jukebox->sfxCategories[jukebox->categoryIdx].songs[jukebox->songIdx].song, BZR_STEREO,
                                 jukeboxBzrDoneCb);
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
            jukebox->isPlaying      = false;
            jukebox->categoryIdx    = 0;
            jukebox->songIdx        = 0;
            jukebox->inMusicSubmode = !jukebox->inMusicSubmode;
            break;
        }
        case PB_UP:
        {
            uint8_t length;
            if (jukebox->inMusicSubmode)
            {
                length = jukebox->numMusicCategories;
            }
            else
            {
                length = jukebox->numSfxCategories;
            }

            uint8_t before = jukebox->categoryIdx;
            if (jukebox->categoryIdx == 0)
            {
                jukebox->categoryIdx = length;
            }
            jukebox->categoryIdx = jukebox->categoryIdx - 1;
            if (jukebox->categoryIdx != before)
            {
                bzrStop(true);
                jukebox->isPlaying = false;
                jukebox->songIdx   = 0;
            }
            break;
        }
        case PB_DOWN:
        {
            uint8_t length;
            if (jukebox->inMusicSubmode)
            {
                length = jukebox->numMusicCategories;
            }
            else
            {
                length = jukebox->numSfxCategories;
            }

            uint8_t before       = jukebox->categoryIdx;
            jukebox->categoryIdx = (jukebox->categoryIdx + 1) % length;
            if (jukebox->categoryIdx != before)
            {
                bzrStop(true);
                jukebox->isPlaying = false;
                jukebox->songIdx   = 0;
            }
            break;
        }
        case PB_LEFT:
        {
            uint8_t length;
            if (jukebox->inMusicSubmode)
            {
                length = ARRAY_SIZE(jukebox->musicCategories[jukebox->categoryIdx]);
            }
            else
            {
                length = ARRAY_SIZE(jukebox->sfxCategories[jukebox->categoryIdx]);
            }

            uint8_t before = jukebox->songIdx;
            if (jukebox->songIdx == 0)
            {
                jukebox->songIdx = length;
            }
            jukebox->songIdx = jukebox->songIdx - 1;
            if (jukebox->songIdx != before)
            {
                bzrStop(true);
                jukebox->isPlaying = false;
            }
            break;
        }
        case PB_RIGHT:
        {
            uint8_t length;
            if (jukebox->inMusicSubmode)
            {
                length = ARRAY_SIZE(jukebox->musicCategories[jukebox->categoryIdx]);
            }
            else
            {
                length = ARRAY_SIZE(jukebox->sfxCategories[jukebox->categoryIdx]);
            }

            uint8_t before   = jukebox->songIdx;
            jukebox->songIdx = (jukebox->songIdx + 1) % length;
            if (jukebox->songIdx != before)
            {
                bzrStop(true);
                jukebox->isPlaying = false;
            }
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
             TFT_WIDTH - CORNER_OFFSET - textWidth(&jukebox->radiostars, portableDanceGetName(jukebox->portableDances)),
             CORNER_OFFSET);

    // Music/SFX
    drawText(&jukebox->radiostars, c555, str_music_sfx, CORNER_OFFSET,
             CORNER_OFFSET + LINE_BREAK_Y + jukebox->radiostars.height);
    // "Music" or "SFX"
    const char* curMusicSfxStr = jukebox->inMusicSubmode ? str_bgm : str_sfx;
    drawText(&(jukebox->radiostars), c111, curMusicSfxStr,
             TFT_WIDTH - CORNER_OFFSET - textWidth(&jukebox->radiostars, curMusicSfxStr),
             CORNER_OFFSET + LINE_BREAK_Y + jukebox->radiostars.height);

    // LED Brightness
    drawText(&jukebox->radiostars, c555, str_brightness, CORNER_OFFSET,
             CORNER_OFFSET + (LINE_BREAK_Y + jukebox->radiostars.height) * 2);
    char text[32];
    snprintf(text, sizeof(text), "%d", getLedBrightnessSetting());
    drawText(&jukebox->radiostars, c111, text, TFT_WIDTH - textWidth(&jukebox->radiostars, text) - CORNER_OFFSET,
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
    drawText(&jukebox->radiostars, c555, btnText, afterText, TFT_HEIGHT - jukebox->radiostars.height - CORNER_OFFSET);

    const char* catName;
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
            catName = jukebox->musicCategories[jukebox->categoryIdx].catName;
            songName     = jukebox->musicCategories[jukebox->categoryIdx].songs[jukebox->songIdx].name;
            songTypeName = "Music";
            numSongs     = ARRAY_SIZE(jukebox->musicCategories[jukebox->categoryIdx]);
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
            catName = jukebox->sfxCategories[jukebox->categoryIdx].catName;
            songName     = jukebox->sfxCategories[jukebox->categoryIdx].songs[jukebox->songIdx].name;
            songTypeName = "SFX";
            numSongs     = ARRAY_SIZE(jukebox->sfxCategories[jukebox->categoryIdx]);
            drawNames    = true;
        }
    }

    if (drawNames)
    {
        // Draw the mode name
        const font_t* nameFont      = &(jukebox->radiostars);
        uint8_t arrowOffsetFromText = 8;
        if (catName == breakoutMode.modeName)
        {
            arrowOffsetFromText = 1;
        }
        snprintf(text, sizeof(text), "Mode: %s", catName);
        int16_t width = textWidth(nameFont, text);
        int16_t yOff  = (TFT_HEIGHT - nameFont->height) / 2 - nameFont->height * 0;
        drawText(nameFont, c311, text, (TFT_WIDTH - width) / 2, yOff);
        // Draw category arrows if this submode has more than 1 category
        if ((jukebox->inMusicSubmode && jukebox->numMusicCategories > 1) || jukebox->numSfxCategories > 1)
        {
            drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) - arrowOffsetFromText - jukebox->arrow.w, yOff, false,
                    false, 0);
            drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) + width + arrowOffsetFromText, yOff, false, false, 180);
        }

        // Draw the song name
        snprintf(text, sizeof(text), "%s: %s", songTypeName, songName);
        yOff  = (TFT_HEIGHT - nameFont->height) / 2 + nameFont->height * 2.5f;
        width = textWidth(nameFont, text);
        drawText(nameFont, c113, text, (TFT_WIDTH - width) / 2, yOff);
        // Draw song arrows if this category has more than 1 song
        if (numSongs > 1)
        {
            drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) - 8 - jukebox->arrow.w, yOff, false, false, 270);
            drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) + width + 8, yOff, false, false, 90);
        }
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
    fillDisplayArea(x, y, x + w, y + h, c235);
}

void jukeboxBzrDoneCb(void)
{
    jukeboxCategory_t* category;
    if (jukebox->inMusicSubmode)
    {
        category = jukebox->musicCategories;
    }
    else
    {
        category = jukebox->sfxCategories;
    }

    if (!category[jukebox->categoryIdx].songs[jukebox->songIdx].song.shouldLoop)
    {
        jukebox->isPlaying = false;
    }
}

void jukeboxFreeCategories(jukeboxCategory_t** categoryArray, uint8_t numCategories)
{
    for (uint8_t catIdx = 0; catIdx < numCategories; catIdx++)
    {
        for (uint8_t songIdx = 0; songIdx < ARRAY_SIZE(categoryArray[catIdx]); songIdx++)
        {
            // Avoid freeing songs we never loaded
            if (categoryArray[catIdx]->songs[songIdx].song.tracks != NULL)
            {
                freeSong(&categoryArray[catIdx]->songs[songIdx].song);
            }
        }

        free(categoryArray[catIdx]->songs);
        free(categoryArray[catIdx]);
    }

    free(categoryArray);
}
