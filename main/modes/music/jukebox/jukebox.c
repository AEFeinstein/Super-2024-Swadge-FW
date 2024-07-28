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
#include <esp_log.h>

#include "hdw-tft.h"

#include "swadge2024.h"
#include "settingsManager.h"
#include "portableDance.h"

#include "accelTest.h"
#include "colorchord.h"
#include "dance.h"
#include "factoryTest.h"
#include "gamepad.h"
#include "jukebox.h"
#include "mainMenu.h"
#include "modeTimer.h"
#include "mode_credits.h"
#include "mode_pinball.h"
#include "touchTest.h"
#include "tunernome.h"
#include "midiPlayer.h"

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
    const char* filename;
    const char* name;
    midiFile_t song;
    bool shouldLoop;
} jukeboxSong_t;

typedef struct
{
    const char* categoryName;
    jukeboxSong_t* songs;
    const uint8_t numSongs;
} jukeboxCategory_t;

typedef struct
{
    // Fonts
    font_t ibm_vga8;
    font_t radiostars;

    // WSGs
    wsg_t arrow;
    wsg_t jukeboxSprite;

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
void jukeboxLoadCategories(const jukeboxCategory_t* categoryArray, uint8_t numCategories, bool shouldLoop);
void jukeboxFreeCategories(const jukeboxCategory_t* categoryArray, uint8_t numCategories);

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

const char* JK_TAG = "JK";

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

jukeboxSong_t music_jukebox[] = {{
                                     .filename = "Fauxrio_Kart.mid",
                                     .name     = "Fauxrio Kart",
                                 },
                                 {
                                     .filename = "hotrod.mid",
                                     .name     = "Hot Rod",
                                 },
                                 {
                                     .filename = "Fairy_Fountain.mid",
                                     .name     = "The Lake",
                                 },
                                 {
                                     .filename = "yalikejazz.mid",
                                     .name     = "Ya like jazz?",
                                 },
                                 {
                                     .filename = "banana.mid",
                                     .name     = "Banana",
                                 }};

jukeboxSong_t music_credits[] = {
    {
        .filename = "credits.mid",
        .name     = creditsName,
    },
};

jukeboxSong_t music_unused[] = {
    {
        .filename = "gmcc.mid",
        .name     = "Pong BGM",
    },
    {
        .filename = "ode.mid",
        .name     = "Ode to Joy",
    },
    {
        .filename = "stereo.mid",
        .name     = "Stereo",
    },
    {
        .filename = "Follinesque.mid",
        .name     = "Follinesque",
    },
};

// clang-format off
const jukeboxCategory_t musicCategories[] = {
    {
        .categoryName = jukeboxName,
        .songs        = music_jukebox,
        .numSongs     = ARRAY_SIZE(music_jukebox),
    },
    {
        .categoryName = creditsName,
        .songs        = music_credits,
        .numSongs     = ARRAY_SIZE(music_credits),
    },
    {
        .categoryName = "(Unused)",
        .songs        = music_unused,
        .numSongs     = ARRAY_SIZE(music_unused),
    }
};
// clang-format on

jukeboxSong_t sfx_mainMenu[] = {
    {
        .filename = "item.mid",
        .name     = "Item",
    },
    {
        .filename = "jingle.mid",
        .name     = "Jingle",
    },
};

jukeboxSong_t sfx_factoryTest[] = {
    {
        .filename = "stereo_test.mid",
        .name     = "Stereo Check",
    },
};

jukeboxSong_t sfx_unused[] = {
    {
        .filename = "block1.mid",
        .name     = "Pong Block 1",
    },
    {
        .filename = "block1.mid",
        .name     = "Pong Block 2",
    },
    {
        .filename = "gamecube.mid",
        .name     = "GameCube",
    },
};

// clang-format off
const jukeboxCategory_t sfxCategories[] = {
    {
        .categoryName = factoryTestName,
        .songs        = sfx_factoryTest,
        .numSongs     = ARRAY_SIZE(sfx_factoryTest),
    },
    {
        .categoryName = mainMenuName,
        .songs        = sfx_mainMenu,
        .numSongs     = ARRAY_SIZE(sfx_mainMenu),
    },
    {
        .categoryName = "(Unused)",
        .songs        = sfx_unused,
        .numSongs     = ARRAY_SIZE(sfx_unused),
    }
};
// clang-format on

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

    // Enter music submode
    jukebox->inMusicSubmode = true;

    // Load fonts
    loadFont("ibm_vga8.font", &jukebox->ibm_vga8, false);
    loadFont("radiostars.font", &jukebox->radiostars, false);

    // Load images
    loadWsg("arrow10.wsg", &jukebox->arrow, false);
    loadWsg("jukebox.wsg", &jukebox->jukeboxSprite, false);

    // Load midis
    jukeboxLoadCategories(musicCategories, ARRAY_SIZE(musicCategories), true);
    jukeboxLoadCategories(sfxCategories, ARRAY_SIZE(sfxCategories), false);

    // Initialize portable dances

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

    soundStop(true);
}

/**
 * Called when jukebox is exited
 */
void jukeboxExitMode(void)
{
    soundStop(true);

    // Free fonts
    freeFont(&jukebox->ibm_vga8);
    freeFont(&jukebox->radiostars);

    // Free images
    freeWsg(&jukebox->arrow);
    freeWsg(&jukebox->jukeboxSprite);

    // Free allocated midis
    jukeboxFreeCategories(musicCategories, ARRAY_SIZE(musicCategories));
    jukeboxFreeCategories(sfxCategories, ARRAY_SIZE(sfxCategories));

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
                soundStop(true);
                jukebox->isPlaying = false;
            }
            else
            {
                if (jukebox->inMusicSubmode)
                {
                    if (soundGetPlayerBgm() != NULL)
                    {
                        soundGetPlayerBgm()->loop
                            = musicCategories[jukebox->categoryIdx].songs[jukebox->songIdx].shouldLoop;
                    }
                    soundPlayBgmCb(&musicCategories[jukebox->categoryIdx].songs[jukebox->songIdx].song, BZR_STEREO,
                                   jukeboxBzrDoneCb);
                }
                else
                {
                    if (soundGetPlayerSfx() != NULL)
                    {
                        soundGetPlayerSfx()->loop
                            = sfxCategories[jukebox->categoryIdx].songs[jukebox->songIdx].shouldLoop;
                    }
                    soundPlaySfxCb(&sfxCategories[jukebox->categoryIdx].songs[jukebox->songIdx].song, BZR_STEREO,
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
            soundStop(true);
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
                length = ARRAY_SIZE(musicCategories);
            }
            else
            {
                length = ARRAY_SIZE(sfxCategories);
            }

            uint8_t before = jukebox->categoryIdx;
            if (jukebox->categoryIdx == 0)
            {
                jukebox->categoryIdx = length;
            }
            jukebox->categoryIdx = jukebox->categoryIdx - 1;
            if (jukebox->categoryIdx != before)
            {
                soundStop(true);
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
                length = ARRAY_SIZE(musicCategories);
            }
            else
            {
                length = ARRAY_SIZE(sfxCategories);
            }

            uint8_t before       = jukebox->categoryIdx;
            jukebox->categoryIdx = (jukebox->categoryIdx + 1) % length;
            if (jukebox->categoryIdx != before)
            {
                soundStop(true);
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
                length = musicCategories[jukebox->categoryIdx].numSongs;
            }
            else
            {
                length = sfxCategories[jukebox->categoryIdx].numSongs;
            }

            uint8_t before = jukebox->songIdx;
            if (jukebox->songIdx == 0)
            {
                jukebox->songIdx = length;
            }
            jukebox->songIdx = jukebox->songIdx - 1;
            if (jukebox->songIdx != before)
            {
                soundStop(true);
                jukebox->isPlaying = false;
            }
            break;
        }
        case PB_RIGHT:
        {
            uint8_t length;
            if (jukebox->inMusicSubmode)
            {
                length = musicCategories[jukebox->categoryIdx].numSongs;
            }
            else
            {
                length = sfxCategories[jukebox->categoryIdx].numSongs;
            }

            uint8_t before   = jukebox->songIdx;
            jukebox->songIdx = (jukebox->songIdx + 1) % length;
            if (jukebox->songIdx != before)
            {
                soundStop(true);
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

    const char* categoryName;
    const char* songName;
    const char* songTypeName;
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
        const font_t* nameFont      = &(jukebox->radiostars);
        uint8_t arrowOffsetFromText = 8;
        snprintf(text, sizeof(text), "Mode: %s", categoryName);
        int16_t width = textWidth(nameFont, text);
        int16_t yOff  = (TFT_HEIGHT - nameFont->height) / 2 - nameFont->height * 0;
        drawText(nameFont, c311, text, (TFT_WIDTH - width) / 2, yOff);
        // Draw category arrows if this submode has more than 1 category
        if ((jukebox->inMusicSubmode && ARRAY_SIZE(musicCategories) > 1) || ARRAY_SIZE(sfxCategories) > 1)
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
    const jukeboxCategory_t* category;
    if (jukebox->inMusicSubmode)
    {
        category = musicCategories;
    }
    else
    {
        category = sfxCategories;
    }

    if (!category[jukebox->categoryIdx].songs[jukebox->songIdx].shouldLoop)
    {
        jukebox->isPlaying = false;
    }
}

void jukeboxLoadCategories(const jukeboxCategory_t* categoryArray, uint8_t numCategories, bool shouldLoop)
{
    for (int categoryIdx = 0; categoryIdx < numCategories; categoryIdx++)
    {
        for (int songIdx = 0; songIdx < categoryArray[categoryIdx].numSongs; songIdx++)
        {
            loadMidiFile(categoryArray[categoryIdx].songs[songIdx].filename,
                         &categoryArray[categoryIdx].songs[songIdx].song, true);
            categoryArray[categoryIdx].songs[songIdx].shouldLoop = shouldLoop;
        }
    }
}

void jukeboxFreeCategories(const jukeboxCategory_t* categoryArray, uint8_t numCategories)
{
    for (uint8_t categoryIdx = 0; categoryIdx < numCategories; categoryIdx++)
    {
        for (uint8_t songIdx = 0; songIdx < categoryArray[categoryIdx].numSongs; songIdx++)
        {
            // Avoid freeing songs we never loaded
            if (categoryArray[categoryIdx].songs[songIdx].song.tracks != NULL)
            {
                unloadMidiFile(&categoryArray[categoryIdx].songs[songIdx].song);
            }
        }
    }
}
