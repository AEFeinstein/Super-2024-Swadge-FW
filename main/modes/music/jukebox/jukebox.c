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

#include "swadge2024.h"
#include "hdw-tft.h"
#include "portableDance.h"
#include "midiPlayer.h"
#include "esp_random.h"

#include "modeIncludeList.h"
#include "jukebox.h"
#include "tunernome.h"
#ifdef SW_VOL_CONTROL
    #include "mainMenu.h"
#endif
#include "mode_credits.h"
#include "factoryTest.h"

/*==============================================================================
 * Defines
 *============================================================================*/

#define CORNER_OFFSET           14
#define JUKEBOX_SPRITE_Y_OFFSET 3
#define LINE_BREAK_LONG_Y       8
#define LINE_BREAK_SHORT_Y      3
#define ARROW_OFFSET_FROM_TEXT  8

#define NUM_DECORATION_WSGS             8
#define MAX_DECORATIONS_ON_SCREEN       10
#define MIN_DECORATION_DELAY_US_PAUSED  1000000
#define MAX_DECORATION_DELAY_US_PAUSED  4000000
#define MIN_DECORATION_DELAY_US_PLAYING 200000
#define MAX_DECORATION_DELAY_US_PLAYING 600000
#define MIN_DECORATION_SPEED_PAUSED     24 // pixels to rise per second
#define MAX_DECORATION_SPEED_PAUSED     48
#define MIN_DECORATION_SPEED_PLAYING    96
#define MAX_DECORATION_SPEED_PLAYING    144
#define MAX_DECORATION_ROTATE_DEG       30

/*==============================================================================
 * Enums
 *============================================================================*/

// Nobody here but us chickens!

/*==============================================================================
 * Structs
 *============================================================================*/

typedef struct
{
    cnfsFileIdx_t fIdx;
    const char* name;
    midiFile_t song;
    bool shouldLoop;
} jukeboxSong_t;

typedef struct
{
    const char* categoryName;
    jukeboxSong_t* songs;
    const uint8_t numSongs;
    bool generalMidi;
} jukeboxCategory_t;

typedef struct
{
    wsg_t* graphic;
    int16_t x;
    float y;
    int16_t rotateDeg;
    uint8_t speed; // pixels to move up per second
} jukeboxDecoration_t;

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

    // Screen Decorations
    jukeboxDecoration_t decorations[MAX_DECORATIONS_ON_SCREEN];
    int64_t usSinceLastDecoration;
    uint32_t usBetweenDecorations;
    wsg_t quaver;
    wsg_t doubleQuaver;
    wsg_t fClef;
    wsg_t halfNote;
    wsg_t taillessQuaver;
    wsg_t trebleClef;
    wsg_t twoTailQuaver;
    wsg_t wholeNote;
    // if you add more decoration wsgs, increment NUM_DECORATION_WSGS
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

wsg_t* jukeboxGetDecorationGraphic(uint8_t i);
bool jukeboxIsDecorationUpsideDownable(uint8_t i);

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
#ifdef SW_VOL_CONTROL
static const char str_bgm_muted[] = "Swadge music is muted!";
static const char str_sfx_muted[] = "Swadge SFX are muted!";
#endif
static const char str_mode[]       = "Mode";
const char str_bgm[]               = "Music";
static const char str_sfx[]        = "SFX";
static const char str_leds[]       = "B: LEDs:";
static const char str_music_sfx[]  = "Pause: Music/SFX:";
static const char str_brightness[] = "Touch: LED Brightness:";
static const char str_stop[]       = ": Stop";
static const char str_play[]       = ": Play";

// Arrays

jukeboxSong_t music_jukebox[] = {
    {
        .fIdx = FAUXRIO_KART_MID,
        .name = "Fauxrio Kart",
    },
    {
        .fIdx = HOTROD_MID,
        .name = "Hot Rod",
    },
    {
        .fIdx = FAIRY_FOUNTAIN_MID,
        .name = "The Lake",
    },
    {
        .fIdx = YALIKEJAZZ_MID,
        .name = "Ya like jazz?",
    },
    {
        .fIdx = BANANA_MID,
        .name = "Banana",
    },
    {
        .fIdx = ATRIUM_VIBE_MID,
        .name = "Atrium TEST", // Only here for ease of testing! TODO remove
    },
};

jukeboxSong_t music_credits[] = {
    {
        .fIdx = HD_CREDITS_MID,
        .name = creditsName,
    },
};

jukeboxSong_t music_unused[] = {
    {
        .fIdx = GMCC_MID,
        .name = "Pong BGM",
    },
    {
        .fIdx = ODE_MID,
        .name = "Ode to Joy",
    },
    {
        .fIdx = STEREO_MID,
        .name = "Stereo",
    },
    {
        .fIdx = FOLLINESQUE_MID,
        .name = "Follinesque",
    },
};

const jukeboxCategory_t musicCategories[] = {
    {
        .categoryName = jukeboxName,
        .songs        = music_jukebox,
        .numSongs     = ARRAY_SIZE(music_jukebox),
        .generalMidi  = false,
    },
    {
        .categoryName = creditsName,
        .songs        = music_credits,
        .numSongs     = ARRAY_SIZE(music_credits),
        .generalMidi  = true,
    },
    {
        .categoryName = "(Unused)",
        .songs        = music_unused,
        .numSongs     = ARRAY_SIZE(music_unused),
        .generalMidi  = false,
    },
};

#ifdef SW_VOL_CONTROL
jukeboxSong_t sfx_mainMenu[] = {
    {
        .fIdx = "jingle.mid",
        .name = "Jingle",
    },
};
#endif

jukeboxSong_t sfx_factoryTest[] = {
    {
        .fIdx = STEREO_TEST_MID,
        .name = "Stereo Check",
    },
};

jukeboxSong_t sfx_unused[] = {
    {
        .fIdx = BLOCK_1_MID,
        .name = "Pong Block 1",
    },
    {
        .fIdx = BLOCK_2_MID,
        .name = "Pong Block 2",
    },
    {
        .fIdx = GAMECUBE_MID,
        .name = "GameCube",
    },
};

const jukeboxCategory_t sfxCategories[] = {
    {
        .categoryName = factoryTestName,
        .songs        = sfx_factoryTest,
        .numSongs     = ARRAY_SIZE(sfx_factoryTest),
        .generalMidi  = true,
    },
#ifdef SW_VOL_CONTROL
    {
        .categoryName = mainMenuName,
        .songs        = sfx_mainMenu,
        .numSongs     = ARRAY_SIZE(sfx_mainMenu),
        .generalMidi  = true,
    },
#endif
    {
        .categoryName = "(Unused)",
        .songs        = sfx_unused,
        .numSongs     = ARRAY_SIZE(sfx_unused),
        .generalMidi  = false,
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
    // Allocate zero'd memory for the mode
    jukebox = heap_caps_calloc(1, sizeof(jukebox_t), MALLOC_CAP_8BIT);

    // Enter music submode
    jukebox->inMusicSubmode = true;

    // Load fonts
    loadFont(IBM_VGA_8_FONT, &jukebox->ibm_vga8, false);
    loadFont(RADIOSTARS_FONT, &jukebox->radiostars, false);

    // Load images
    loadWsg(ARROW_10_WSG, &jukebox->arrow, false);
    loadWsg(JUKEBOX_WSG, &jukebox->jukeboxSprite, false);

    loadWsg(QUAVER_WSG, &jukebox->quaver, false);
    loadWsg(DOUBLE_QUAVER_WSG, &jukebox->doubleQuaver, false);
    loadWsg(F_CLEF_WSG, &jukebox->fClef, false);
    loadWsg(HALF_NOTE_WSG, &jukebox->halfNote, false);
    loadWsg(TAILLESS_QUAVER_WSG, &jukebox->taillessQuaver, false);
    loadWsg(TREBLE_CLEF_WSG, &jukebox->trebleClef, false);
    loadWsg(TWO_TAIL_QUAVER_WSG, &jukebox->twoTailQuaver, false);
    loadWsg(WHOLE_NOTE_WSG, &jukebox->wholeNote, false);

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

    freeWsg(&jukebox->quaver);
    freeWsg(&jukebox->doubleQuaver);
    freeWsg(&jukebox->fClef);
    freeWsg(&jukebox->halfNote);
    freeWsg(&jukebox->taillessQuaver);
    freeWsg(&jukebox->trebleClef);
    freeWsg(&jukebox->twoTailQuaver);
    freeWsg(&jukebox->wholeNote);

    // Free allocated midis
    jukeboxFreeCategories(musicCategories, ARRAY_SIZE(musicCategories));
    jukeboxFreeCategories(sfxCategories, ARRAY_SIZE(sfxCategories));

    // Free dances
    freePortableDance(jukebox->portableDances);

    heap_caps_free(jukebox);
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

                    if (musicCategories[jukebox->categoryIdx].generalMidi)
                    {
                        midiGmOn(soundGetPlayerBgm());
                    }
                    else
                    {
                        midiGmOff(soundGetPlayerBgm());
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

                    if (sfxCategories[jukebox->categoryIdx].generalMidi)
                    {
                        midiGmOn(soundGetPlayerSfx());
                    }
                    else
                    {
                        midiGmOff(soundGetPlayerSfx());
                    }

                    soundPlaySfxCb(&sfxCategories[jukebox->categoryIdx].songs[jukebox->songIdx].song, BZR_STEREO,
                                   jukeboxBzrDoneCb);
                }
                jukebox->isPlaying            = true;
                jukebox->usBetweenDecorations = 0;
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

    // Plot background decorations (music notes)
    bool spawnDecoration = false;
    jukebox->usSinceLastDecoration += elapsedUs;

    if (jukebox->usSinceLastDecoration >= jukebox->usBetweenDecorations)
    {
        spawnDecoration = true;

        if (jukebox->isPlaying)
        {
            jukebox->usBetweenDecorations
                = MIN_DECORATION_DELAY_US_PLAYING
                  + (esp_random() % (MAX_DECORATION_DELAY_US_PLAYING - MIN_DECORATION_DELAY_US_PLAYING));
        }
        else
        {
            jukebox->usBetweenDecorations
                = MIN_DECORATION_DELAY_US_PAUSED
                  + (esp_random() % (MAX_DECORATION_DELAY_US_PAUSED - MIN_DECORATION_DELAY_US_PAUSED));
        }
    }

    for (uint8_t i = 0; i < MAX_DECORATIONS_ON_SCREEN; i++)
    {
        jukeboxDecoration_t* decoration = &jukebox->decorations[i];
        bool spawnedDecoration          = false;
        if (spawnDecoration && decoration->graphic == 0)
        {
            uint8_t index         = esp_random() % NUM_DECORATION_WSGS;
            decoration->graphic   = jukeboxGetDecorationGraphic(index);
            decoration->x         = (esp_random() % TFT_WIDTH) - (decoration->graphic->w / 2);
            decoration->y         = TFT_HEIGHT - 1;
            decoration->rotateDeg = (esp_random() % (MAX_DECORATION_ROTATE_DEG * 2 + 1)) - MAX_DECORATION_ROTATE_DEG;

            if (jukeboxIsDecorationUpsideDownable(index) && (esp_random() % 4) == 0)
            {
                decoration->rotateDeg += 180;
            }

            if (decoration->rotateDeg < 0)
            {
                decoration->rotateDeg += 360;
            }

            if (jukebox->isPlaying)
            {
                decoration->speed
                    = MIN_DECORATION_SPEED_PLAYING
                      + (esp_random() % (MAX_DECORATION_SPEED_PLAYING - MIN_DECORATION_SPEED_PLAYING + 1));
            }
            else
            {
                decoration->speed = MIN_DECORATION_SPEED_PAUSED
                                    + (esp_random() % (MAX_DECORATION_SPEED_PAUSED - MIN_DECORATION_SPEED_PAUSED + 1));
            }
            jukebox->usSinceLastDecoration = 0;
            spawnDecoration                = false;
            spawnedDecoration              = true;
        }

        if (decoration->graphic != 0)
        {
            if (!spawnedDecoration)
            {
                decoration->y -= decoration->speed * (elapsedUs / 1000000.0);
            }

            drawWsg(decoration->graphic, decoration->x, round(decoration->y), false, false, decoration->rotateDeg);

            if (decoration->y < -decoration->graphic->h)
            {
                decoration->graphic   = 0;
                decoration->x         = 0;
                decoration->y         = 0;
                decoration->rotateDeg = 0;
                decoration->speed     = 0;
            }
        }
    }

    // Plot jukebox sprite
    int16_t spriteWidth = jukebox->jukeboxSprite.w;
    drawWsg(&jukebox->jukeboxSprite, (TFT_WIDTH - spriteWidth) / 2,
            TFT_HEIGHT - jukebox->jukeboxSprite.h - JUKEBOX_SPRITE_Y_OFFSET, false, false, 0);

    // Plot the button funcs
    // LEDs
    drawText(&jukebox->radiostars, c555, str_leds, CORNER_OFFSET, CORNER_OFFSET);
    // Light dance name
    drawText(&(jukebox->radiostars), c444, portableDanceGetName(jukebox->portableDances),
             TFT_WIDTH - CORNER_OFFSET - textWidth(&jukebox->radiostars, portableDanceGetName(jukebox->portableDances)),
             CORNER_OFFSET);

    // Music/SFX
    drawText(&jukebox->radiostars, c555, str_music_sfx, CORNER_OFFSET,
             CORNER_OFFSET + LINE_BREAK_LONG_Y + jukebox->radiostars.height);
    // "Music" or "SFX"
    const char* curMusicSfxStr = jukebox->inMusicSubmode ? str_bgm : str_sfx;
    drawText(&(jukebox->radiostars), c444, curMusicSfxStr,
             TFT_WIDTH - CORNER_OFFSET - textWidth(&jukebox->radiostars, curMusicSfxStr),
             CORNER_OFFSET + LINE_BREAK_LONG_Y + jukebox->radiostars.height);

    // LED Brightness
    drawText(&jukebox->radiostars, c555, str_brightness, CORNER_OFFSET,
             CORNER_OFFSET + (LINE_BREAK_LONG_Y + jukebox->radiostars.height) * 2);
    char text[32];
    snprintf(text, sizeof(text), "%d", getLedBrightnessSetting());
    drawText(&jukebox->radiostars, c444, text, TFT_WIDTH - textWidth(&jukebox->radiostars, text) - CORNER_OFFSET,
             CORNER_OFFSET + (LINE_BREAK_LONG_Y + jukebox->radiostars.height) * 2);

    // Assume not playing
    paletteColor_t color = c252;
    const char* btnText  = str_play;
    if (jukebox->isPlaying)
    {
        color   = c533;
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
#ifdef SW_VOL_CONTROL
        // Warn the user that the swadge is muted, if that's the case
        if (getBgmVolumeSetting() == getBgmVolumeSettingBounds()->min)
        {
            drawText(&jukebox->radiostars, c551, str_bgm_muted,
                     (TFT_WIDTH - textWidth(&jukebox->radiostars, str_bgm_muted)) / 2, TFT_HEIGHT / 2);
        }
        else
#endif
        {
            categoryName = musicCategories[jukebox->categoryIdx].categoryName;
            songName     = musicCategories[jukebox->categoryIdx].songs[jukebox->songIdx].name;
            songTypeName = str_bgm;
            numSongs     = musicCategories[jukebox->categoryIdx].numSongs;
            drawNames    = true;
        }
    }
    else
    {
#ifdef SW_VOL_CONTROL
        // Warn the user that the swadge is muted, if that's the case
        if (getSfxVolumeSetting() == getSfxVolumeSettingBounds()->min)
        {
            drawText(&jukebox->radiostars, c551, str_sfx_muted,
                     (TFT_WIDTH - textWidth(&jukebox->radiostars, str_sfx_muted)) / 2, TFT_HEIGHT / 2);
        }
        else
#endif
        {
            categoryName = sfxCategories[jukebox->categoryIdx].categoryName;
            songName     = sfxCategories[jukebox->categoryIdx].songs[jukebox->songIdx].name;
            songTypeName = str_sfx;
            numSongs     = sfxCategories[jukebox->categoryIdx].numSongs;
            drawNames    = true;
        }
    }

    if (drawNames)
    {
        // Draw the word "Mode"
        const font_t* nameFont = &(jukebox->radiostars);
        int16_t width          = textWidth(nameFont, str_mode);
        int16_t yOff           = (TFT_HEIGHT - nameFont->height) / 2 - (nameFont->height + 2) * 2;
        drawText(nameFont, c533, str_mode, (TFT_WIDTH - width) / 2, yOff);
        // Draw category arrows if this submode has more than 1 category
        if ((jukebox->inMusicSubmode && ARRAY_SIZE(musicCategories) > 1) || ARRAY_SIZE(sfxCategories) > 1)
        {
            drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) - ARROW_OFFSET_FROM_TEXT - jukebox->arrow.w, yOff, false,
                    false, 0);
            drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) + width + ARROW_OFFSET_FROM_TEXT, yOff, false, false,
                    180);
        }

        // Draw the mode name
        yOff += nameFont->height + LINE_BREAK_SHORT_Y;
        drawText(nameFont, c533, categoryName, (TFT_WIDTH - textWidth(nameFont, categoryName)) / 2, yOff);

        // Draw either the word "Music" or "SFX"
        yOff += (nameFont->height + LINE_BREAK_SHORT_Y) * 2;
        width = textWidth(nameFont, songTypeName);
        drawText(nameFont, c335, songTypeName, (TFT_WIDTH - width) / 2, yOff);
        // Draw song arrows if this category has more than 1 song
        if (numSongs > 1)
        {
            drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) - 8 - jukebox->arrow.w, yOff, false, false, 270);
            drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) + width + 8, yOff, false, false, 90);
        }

        // Draw the song name
        yOff += nameFont->height + LINE_BREAK_SHORT_Y;
        drawText(nameFont, c335, songName, (TFT_WIDTH - textWidth(nameFont, songName)) / 2, yOff);
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
    fillDisplayArea(x, y, x + w, y + h, c303);
}

void jukeboxBzrDoneCb(void)
{
    // printf("bzr done\n");
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
            loadMidiFile(categoryArray[categoryIdx].songs[songIdx].fIdx,
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

wsg_t* jukeboxGetDecorationGraphic(uint8_t i)
{
    switch (i)
    {
        case 1:
            return &jukebox->doubleQuaver;
        case 2:
            return &jukebox->fClef;
        case 3:
            return &jukebox->halfNote;
        case 4:
            return &jukebox->taillessQuaver;
        case 5:
            return &jukebox->trebleClef;
        case 6:
            return &jukebox->twoTailQuaver;
        case 7:
            return &jukebox->wholeNote;
        case 0:
        default:
            return &jukebox->quaver;
    }
}

bool jukeboxIsDecorationUpsideDownable(uint8_t i)
{
    switch (i)
    {
        case 3:
        case 4:
            return true;
        case 0:
        case 1:
        case 2:
        case 5:
        case 6:
        case 7:
        default:
            return false;
    }
}
