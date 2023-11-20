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
#include "mainMenu.h"
#include "breakout.h"
#include "factoryTest.h"
#include "mode_platformer.h"
#include "mode_credits.h"
#include "portableDance.h"
#include "mode_ray.h"
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
    const char* filename;
    const char* name;
    song_t song;
} jukeboxSong_t;

typedef struct
{
    const char* categoryName;
    const jukeboxSong_t* songs;
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

    // Music Midis and Jukebox Structs
    uint8_t numMusicCategories;

    // SFX Midis and Jukebox Structs
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
void jukeboxLoadCategories(const jukeboxCategory_t* categoryArray, uint8_t numCategories);
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

const jukeboxSong_t songs_galacticBrickdown[] = {
    // {
    //     .filename = "brkBgmCrazy.sng",
    //     .name  = "BGM Crazy",
    // },
    // {
    //     .filename = "brkBgmFinale.sng",
    //     .name  = "BGM Finale",
    // },
    // {
    //     .filename = "brkBgmPixel.sng",
    //     .name  = "BGM Pixel",
    // },        
    // {
    //     .filename = "brkBgmSkill.sng",
    //     .name  = "BGM Skill",
    // },        
    // {
    //     .filename = "brkBgmTitle.sng",
    //     .name  = "BGM Title",
    // },
};

const jukeboxSong_t songs_swadgeLand[] = {
    {
        .filename = "bgmDeMAGio.sng",
        .name  = "DeMAGio BGM",
    },
    {
        .filename = "bgmSmooth.sng",
        .name  = "Smooth BGM",
    },        
    {
        .filename = "bgmUnderground.sng",
        .name  = "Underground BGM",
    },        
    {
        .filename = "bgmCastle.sng",
        .name  = "Castle BGM",
    },        
    {
        .filename = "bgmNameEntry.sng",
        .name  = "Name Entry BGM",
    },
};

const jukeboxSong_t songs_magtroid[] = {
    {
        .filename = "base_0.sng",
        .name  = "Base 0",
    },
    {
        .filename = "base_1.sng",
        .name  = "Base 1",
    },        
    {
        .filename = "cave_0.sng",
        .name  = "Cave 0",
    },        
    {
        .filename = "cave_1.sng",
        .name  = "Cave 1",
    },        
    {
        .filename = "jungle_0.sng",
        .name  = "Jungle 0",
    },
    {
        .filename = "jungle_1.sng",
        .name  = "Jungle 1",
    },  
    {
        .filename = "ray_boss.sng",
        .name  = "Boss",
    },  
};

const jukeboxSong_t songs_jukebox[] = {
    {
        .filename = "hotrod.sng",
        .name  = "Hot Rod",
    },
    {
        .filename = "Fauxrio_Kart.sng",
        .name  = "Fauxrio Kart",
    },        
    {
        .filename = "thelake.sng",
        .name  = "The Lake",
    },        
    {
        .filename = "yalikejazz.sng",
        .name  = "Ya like jazz?",
    },        
    {
        .filename = "banana.sng",
        .name  = "Banana",
    },
};

const jukeboxSong_t songs_credits[] = {
    {
        .filename = "credits.sng",
        .name  = creditsName,
    },
};

const jukeboxSong_t songs_unused[] = {
    {
        .filename = "Pong BGM",
        .name  = "gmcc.sng",
    },
    {
        .filename = "Ode to Joy",
        .name  = "ode.sng",
    },
    {
        .filename = "Stereo",
        .name  = "stereo.sng",
    },
};

const jukeboxCategory_t musicCategories[] = {
    {
        .categoryName  = breakoutName,
        .songs = songs_galacticBrickdown,
        .numSongs = ARRAY_SIZE(songs_galacticBrickdown),
    },
    {
        .categoryName  = platformerName,
        .songs = songs_swadgeLand,
        .numSongs = ARRAY_SIZE(songs_swadgeLand),
    },
    {
        .categoryName  = rayName,
        .songs = songs_magtroid,
        .numSongs = ARRAY_SIZE(songs_magtroid),
    },
    {
        .categoryName  = jukeboxName,
        .songs = songs_jukebox,
        .numSongs = ARRAY_SIZE(songs_jukebox),
    },
    {
        .categoryName = "(Unused)",
        .songs = songs_unused,
        .numSongs = ARRAY_SIZE(songs_unused),
    }
};

const jukeboxSong_t sounds_galacticBrickdown[] = {
    {
        .filename = "sndBounce.sng",
        .name  = "Bounce",
    },
    {
        .filename = "sndBreak.sng",
        .name  = "Break",
    },        
    {
        .filename = "sndBreak2.sng",
        .name  = "Break 2",
    },        
    {
        .filename = "sndBreak3.sng",
        .name  = "Break 3",
    },        
    {
        .filename = "sndBrk1up.sng",
        .name  = "Break 1-Up",
    },        
    {
        .filename = "sndBrkDie.sng",
        .name  = "Break Die",
    },        
    {
        .filename = "sndDetonate.sng",
        .name  = "Detonate",
    },        
    {
        .filename = "sndDropBomb.sng",
        .name  = "Drop Bomb",
    },        
    {
        .filename = "sndTally.sng",
        .name  = "Tally",
    },        
    {
        .filename = "sndWaveBall.sng",
        .name  = "Wave Ball",
    },        
    {
        .filename = "brkGameOver.sng",
        .name  = "Game Over",
    },        
    {
        .filename = "brkGetReady.sng",
        .name  = "Get Ready",
    },        
    {
        .filename = "brkHighScore.sng",
        .name  = "High Score",
    },        
    {
        .filename = "brkLvlClear.sng",
        .name  = "Level Clear",
    },
};

const jukeboxSong_t sounds_swadgeland[] = {
    {
        .filename = "bgmIntro.sng",
        .name  = "Into",
    },
    {
        .filename = "bgmGameStart.sng",
        .name  = "Game Start",
    },        
    {
        .filename = "bgmGameOver.sng",
        .name  = "Game Over",
    },        
    {
        .filename = "snd1up.sng",
        .name  = "1-Up",
    },        
    {
        .filename = "sndCheckpoint.sng",
        .name  = "Checkpoint",
    },        
    {
        .filename = "sndCoin.sng",
        .name  = "Coin",
    },        
    {
        .filename = "sndDie.sng",
        .name  = "Die",
    },        
    {
        .filename = "sndHit.sng",
        .name  = "Hit",
    },        
    {
        .filename = "sndHurt.sng",
        .name  = "Hurt",
    },        
    {
        .filename = "sndJump1.sng",
        .name  = "Jump 1",
    },        
    {
        .filename = "sndJump2.sng",
        .name  = "Jump 2",
    },        
    {
        .filename = "sndJump3.sng",
        .name  = "Jump 3",
    },        
    {
        .filename = "sndLevelClearA.sng",
        .name  = "Level Clear A",
    },        
    {
        .filename = "sndLevelClearB.sng",
        .name  = "Level Clear B",
    },        
    {
        .filename = "sndLevelClearC.sng",
        .name  = "Level Clear C",
    },        
    {
        .filename = "sndLevelClearD.sng",
        .name  = "Level Clear D",
    },        
    {
        .filename = "sndLevelClearS.sng",
        .name  = "Level Clear S",
    },        
    {
        .filename = "sndMenuConfirm.sng",
        .name  = "Menu Confirm",
    },        
    {
        .filename = "sndMenuDeny.sng",
        .name  = "Menu Deny",
    },        
    {
        .filename = "sndMenuSelect.sng",
        .name  = "Menu Select",
    },        
    {
        .filename = "sndOuttaTime.sng",
        .name  = "Outta Time",
    },        
    {
        .filename = "sndPause.sng",
        .name  = "Pause",
    },        
    {
        .filename = "sndPowerUp.sng",
        .name  = "Power Up",
    },        
    {
        .filename = "sndSquish.sng",
        .name  = "Squish",
    },        
    {
        .filename = "sndWarp.sng",
        .name  = "Warp",
    },        
    {
        .filename = "sndWaveBall.sng",
        .name  = "Wave Ball",
    },
};

const jukeboxSong_t sounds_magtroid[] = {
    {
        .filename = "r_door_open.sng",
        .name  = "Door Open",
    },
    {
        .filename = "r_e_block.sng",
        .name  = "Block",
    },
    {
        .filename = "r_e_damage.sng",
        .name  = "Damage",
    },
    {
        .filename = "r_e_dead.sng",
        .name  = "Dead",
    },
    {
        .filename = "r_e_freeze.sng",
        .name  = "Freeze",
    },
    {
        .filename = "r_game_over.sng",
        .name  = "Game Over",
    },
    {
        .filename = "r_health.sng",
        .name  = "Health",
    },
    {
        .filename = "r_item_get.sng",
        .name  = "Item Get",
    },
    {
        .filename = "r_lava_dmg.sng",
        .name  = "Lava Damage",
    },
    {
        .filename = "r_p_charge.sng",
        .name  = "Charge",
    },
    {
        .filename = "r_p_charge_start.sng",
        .name  = "Charge Start",
    },
    {
        .filename = "r_p_damage.sng",
        .name  = "Damage",
    },
    {
        .filename = "r_p_ice.sng",
        .name  = "Ice",
    },
    {
        .filename = "r_p_missile.sng",
        .name  = "Missile",
    },
    {
        .filename = "r_p_shoot.sng",
        .name  = "Shoot",
    },
    {
        .filename = "r_p_xray.sng",
        .name  = "X-Ray",
    },
    {
        .filename = "r_warp.sng",
        .name  = "Warp",
    },
};

const jukeboxSong_t sounds_mainMenu[] = {
    {
        .filename = "item.sng",
        .name  = "Item",
    },
    {
        .filename = "jingle.sng",
        .name  = "Jingle",
    },
};

const jukeboxSong_t sounds_unused[] = {
    {
        .filename = "Pong Block 1",
        .name  = "block1.sng",
    },
    {
        .filename = "Pong Block 2",
        .name  = "block2.sng",
    },
};

const jukeboxSong_t sounds_FactoryTest[] = {
    {
        .filename = "stereo_test.sng",
        .name  = "Stereo Check",
    },
};

const jukeboxCategory_t sfxCategories[] = {
    {
        .categoryName  = breakoutName,
        .songs = sounds_galacticBrickdown,
        .numSongs = ARRAY_SIZE(sounds_galacticBrickdown),
    },
    {
        .categoryName  = platformerName,
        .songs = sounds_swadgeland,
        .numSongs = ARRAY_SIZE(sounds_swadgeland),
    },
    {
        .categoryName  = rayName,
        .songs = sounds_magtroid,
        .numSongs = ARRAY_SIZE(sounds_magtroid),
    },
    {
        .categoryName  = factoryTestName,
        .songs = sounds_FactoryTest,
        .numSongs = ARRAY_SIZE(sounds_FactoryTest),
    },
    {
        .categoryName  = mainMenuName,
        .songs = sounds_mainMenu,
        .numSongs = ARRAY_SIZE(sounds_mainMenu),
    },
    {
        .categoryName = "(Unused)",
        .songs = sounds_unused,
        .numSongs = ARRAY_SIZE(sounds_unused),
    }
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

    jukeboxLoadCategories(musicCategories, ARRAY_SIZE(musicCategories));
    jukeboxLoadCategories(sfxCategories, ARRAY_SIZE(sfxCategories));

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
    jukeboxFreeCategories(musicCategories, jukebox->numMusicCategories);

    // Free allocated SFX midis, song arrays, and category arrays

    jukeboxFreeCategories(sfxCategories, jukebox->numSfxCategories);

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
                    bzrPlayBgmCb(&musicCategories[jukebox->categoryIdx].songs[jukebox->songIdx].song,
                                 BZR_STEREO, jukeboxBzrDoneCb);
                }
                else
                {
                    bzrPlaySfxCb(&sfxCategories[jukebox->categoryIdx].songs[jukebox->songIdx].song, BZR_STEREO,
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
        if (categoryName == breakoutMode.modeName)
        {
            arrowOffsetFromText = 1;
        }
        snprintf(text, sizeof(text), "Mode: %s", categoryName);
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
    const jukeboxCategory_t* category;
    if (jukebox->inMusicSubmode)
    {
        category = musicCategories;
    }
    else
    {
        category = sfxCategories;
    }

    if (!category[jukebox->categoryIdx].songs[jukebox->songIdx].song.shouldLoop)
    {
        jukebox->isPlaying = false;
    }
}

void jukeboxLoadCategories(const jukeboxCategory_t* categoryArray, uint8_t numCategories)
{
    for (int categoryIdx = 0; categoryIdx < numCategories; categoryIdx++)
    {
        for (int songIdx = 0; songIdx < categoryArray[categoryIdx].numSongs; songIdx++)
        {
            loadSong(categoryArray[categoryIdx].songs[songIdx].filename, &categoryArray[categoryIdx].songs[songIdx].song, true);
            
            if (categoryArray[categoryIdx].songs[songIdx].song.numTracks == 0)
            {
                ESP_LOGE(JK_TAG, "Category %d \"%s\" song %d \"%s\" numTracks is 0\n", categoryIdx, categoryArray[categoryIdx].categoryName, songIdx, categoryArray[categoryIdx].songs[songIdx].filename);
            }
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
                freeSong(&categoryArray[categoryIdx].songs[songIdx].song);
            }
        }

        free(categoryArray[categoryIdx].songs);
        free(&categoryArray[categoryIdx]);
    }
}
