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

#include "mode_bigbug.h"
#include "mode_swadgeHero.h"
#include "pango.h"
#include "ultimateTTT.h"
#include "mode_cGrove.h"
#include "mode_2048.h"
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
#define MIN_DECORATION_SPEED_PAUSED     24 //pixels to rise per second
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

jukeboxSong_t music_bigbug[] = {
    {
        .filename = "brkBgmTitle.mid",
        .name     = "BGM Title",
    },
    {
        .filename = "brkBgmCrazy.mid",
        .name     = "BGM Crazy",
    },
    {
        .filename = "brkBgmPixel.mid",
        .name     = "BGM Pixel",
    },
    {
        .filename = "brkBgmSkill.mid",
        .name     = "BGM Skill",
    },
    {
        .filename = "brkBgmFinale.mid",
        .name     = "BGM Finale",
    },
    {
        .filename = "brkHighScore.mid",
        .name     = "High Score",
    },
    {
        .filename = "BigBug_Dr.Garbotniks Home.mid",
        .name     = "Dr. Garbotnik's Home",
    },
    {
        .filename = "BigBug_Dr.Garbotniks Home2.mid",
        .name     = "Dr. Garbotnik's Home 2",
    },
    {
        .filename = "BigBugExploration.mid",
        .name     = "Exploration",
    },
    {
        .filename = "Big Bug Hurry up.mid",
        .name     = "Hurry Up!",
    },
    {
        .filename = "BigBug_Space Travel.mid",
        .name     = "Space Travel",
    },
    {
        .filename = "BigBug_Boss.mid",
        .name     = "Revenge",
    },
};

jukeboxSong_t music_swadgehero[] = {
    {
        .filename = "sh_bleed.mid",
        .name     = "Let It Bleed",
    },
    {
        .filename = "sh_cgrove.mid",
        .name     = "Chowa Grove",
    },
    {
        .filename = "sh_crace.mid",
        .name     = "Chowa Race",
    },
    {
        .filename = "sh_credits.mid",
        .name     = "Hot Dog Credits",
    },
    {
        .filename = "sh_cremulons.mid",
        .name     = "Dance of the Cremulons",
    },
    {
        .filename = "sh_devils.mid",
        .name     = "The Devil's Lullaby",
    },
    {
        .filename = "sh_gs_credits.mid",
        .name     = "Gunship Credits",
    },
    {
        .filename = "sh_ocean.mid",
        .name     = "Swadge City Ocean 1989",
    },
    {
        .filename = "sh_pain.mid",
        .name     = "Pain",
    },
    {
        .filename = "sh_pango.mid",
        .name     = "Pango",
    },
    {
        .filename = "sh_revenge.mid",
        .name     = "Revenge",
    },
    {
        .filename = "sh_starfest.mid",
        .name     = "Starfest Magway",
    },
    {
        .filename = "sh_wakeman.mid",
        .name     = "Wake Man Stage",
    },
    
    {
        .filename = "sh_sunrise.mid",
        .name     = "Sunrise (Unused)",
    },
};

jukeboxSong_t music_pango[] = {
    {
        .filename = "Pango_Jump Start.mid",
        .name     = "Jump Start",
    },
    {
        .filename = "Pango_Main.mid",
        .name     = "Main",
    },
    {
        .filename = "Pango_Faster.mid",
        .name     = "Faster",
    },
    {
        .filename = "Pango_Speed.mid",
        .name     = "Speed",
    },
    {
        .filename = "Pango_High Score.mid",
        .name     = "High Score",
    },
    {
        .filename = "Pango_Game Over.mid",
        .name     = "Game Over",
    },
    {
        .filename = "bgmGameStart.mid",
        .name     = "Game Start (Unused)",
    },
    {
        .filename = "bgmGameOver.mid",
        .name     = "Game Over (Unused)",
    },
};

jukeboxSong_t music_chowagrove[] = {
    {
        .filename = "Chowa_Menu.mid",
        .name     = "Menu",
    },
    {
        .filename = "Chowa_Battle.mid",
        .name     = "Battle",
    },
    {
        .filename = "Chowa_Dancing.mid",
        .name     = "Dancing",
    },
    {
        .filename = "Chowa_Meadow.mid",
        .name     = "Meadow",
    },
    {
        .filename = "Chowa_Race.mid",
        .name     = "Race",
    },
};

jukeboxSong_t music_2048[] = {
    {
        .filename = "lullaby_in_numbers.mid",
        .name     = "Lullaby In Numbers",
    },
};

jukeboxSong_t music_jukebox[] = {
    {
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
    }
};

jukeboxSong_t music_credits[] = {
    {
        .filename = "hd_credits.mid",
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
        .categoryName = bigbugName,
        .songs        = music_bigbug,
        .numSongs     = ARRAY_SIZE(music_bigbug),
    },
    {
        .categoryName = shName,
        .songs        = music_swadgehero,
        .numSongs     = ARRAY_SIZE(music_swadgehero),
    },
    {
        .categoryName = pangoName,
        .songs        = music_pango,
        .numSongs     = ARRAY_SIZE(music_pango),
    },
    {
        .categoryName = cGroveTitle,
        .songs        = music_chowagrove,
        .numSongs     = ARRAY_SIZE(music_chowagrove),
    },
    {
        .categoryName = t48Name,
        .songs        = music_2048,
        .numSongs     = ARRAY_SIZE(music_2048),
    },
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
    },
};
// clang-format on

jukeboxSong_t sfx_bigbug[] = {
    {
        .filename = "brkGetReady.mid",
        .name     = "Get Ready",
    },
    {
        .filename = "sndBounce.mid",
        .name     = "Bounce",
    },
    {
        .filename = "sndBreak.mid",
        .name     = "Break",
    },
    {
        .filename = "sndBreak2.mid",
        .name     = "Break 2",
    },
    {
        .filename = "sndBreak3.mid",
        .name     = "Break 3",
    },
    {
        .filename = "sndBrk1up.mid",
        .name     = "1-Up",
    },
    {
        .filename = "sndDropBomb.mid",
        .name     = "Drop Bomb",
    },
    {
        .filename = "sndWaveBall.mid",
        .name     = "Wave Ball",
    },
    {
        .filename = "brkLvlClear.mid",
        .name     = "Level Clear",
    },
    {
        .filename = "sndBrkTally.mid",
        .name     = "Tally",
    },
    {
        .filename = "sndBrkDie.mid",
        .name     = "Die",
    },
    {
        .filename = "brkGameOver.mid",
        .name     = "Game Over",
    },
    {
        .filename = "BigBug - Car 1.mid",
        .name     = "Car 1",
    },
    {
        .filename = "BigBug - Car 2.mid",
        .name     = "Car 2",
    },
    {
        .filename = "BigBug - Car 3.mid",
        .name     = "Car 3",
    },
    {
        .filename = "BigBug - Collection.mid",
        .name     = "Collection",
    },
    {
        .filename = "BigBug - Egg 1.mid",
        .name     = "Egg 1",
    },
    {
        .filename = "BigBug - Egg 2.mid",
        .name     = "Egg 2",
    },
    {
        .filename = "Bump.mid",
        .name     = "Bump",
    },
    {
        .filename = "Dirt_Breaking.mid",
        .name     = "Dirt Breaking",
    },
    {
        .filename = "Harpoon.mid",
        .name     = "Harpoon",
    },
    {
        .filename = "r_health.mid",
        .name     = "Health",
    },
    {
        .filename = "r_item_get.mid",
        .name     = "Item Get",
    },
    {
        .filename = "r_p_ice.mid",
        .name     = "Ice",
    },
};

jukeboxSong_t sfx_pango[] = {
    {
        .filename = "sndMenuSelect.mid",
        .name     = "Menu Select",
    },
    {
        .filename = "sndMenuConfirm.mid",
        .name     = "Menu Confirm",
    },
    {
        .filename = "sndMenuDeny.mid",
        .name     = "Menu Deny",
    },
    {
        .filename = "sndSpawn.mid",
        .name     = "Spawn",
    },
    {
        .filename = "sndSlide.mid",
        .name     = "Slide",
    },
    {
        .filename = "sndBlockStop.mid",
        .name     = "Block Stop",
    },
    {
        .filename = "sndBlockCombo.mid",
        .name     = "Block Combo",
    },
    {
        .filename = "sndSquish.mid",
        .name     = "Squish",
    },
    {
        .filename = "sndPause.mid",
        .name     = "Pause",
    },
    {
        .filename = "snd1up.mid",
        .name     = "1-Up",
    },
    {
        .filename = "Pango_Level Clear.mid",
        .name     = "Level Clear",
    },
    {
        .filename = "sndTally.mid",
        .name     = "Tally",
    },
    {
        .filename = "sndDie.mid",
        .name     = "Die",
    },
};

jukeboxSong_t sfx_ultimatettt[] = {
    {
        .filename = "uttt_cursor.mid",
        .name     = "Cursor",
    },
    {
        .filename = "uttt_marker.mid",
        .name     = "Marker",
    },
    {
        .filename = "`.mid",
        .name     = "Win Subgame",
    },
    {
        .filename = "uttt_win_g.mid",
        .name     = "Win Game",
    },
};

jukeboxSong_t sfx_2048[] = {
    {
        .filename = "sndBounce.mid",
        .name     = "Bounce",
    },
};

#ifdef SW_VOL_CONTROL
jukeboxSong_t sfx_mainMenu[] = {
    {
        .filename = "jingle.mid",
        .name     = "Jingle",
    },
};
#endif

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
        .categoryName = bigbugName,
        .songs        = sfx_bigbug,
        .numSongs     = ARRAY_SIZE(sfx_bigbug),
    },
    {
        .categoryName = pangoName,
        .songs        = sfx_pango,
        .numSongs     = ARRAY_SIZE(sfx_pango),
    },
    {
        .categoryName = tttName,
        .songs        = sfx_ultimatettt,
        .numSongs     = ARRAY_SIZE(sfx_ultimatettt),
    },
    {
        .categoryName = factoryTestName,
        .songs        = sfx_factoryTest,
        .numSongs     = ARRAY_SIZE(sfx_factoryTest),
    },
#ifdef SW_VOL_CONTROL
    {
        .categoryName = mainMenuName,
        .songs        = sfx_mainMenu,
        .numSongs     = ARRAY_SIZE(sfx_mainMenu),
    },
#endif
    {
        .categoryName = "(Unused)",
        .songs        = sfx_unused,
        .numSongs     = ARRAY_SIZE(sfx_unused),
    },
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
    jukebox = heap_caps_calloc(1, sizeof(jukebox_t), MALLOC_CAP_8BIT);

    // Enter music submode
    jukebox->inMusicSubmode = true;

    // Load fonts
    loadFont("ibm_vga8.font", &jukebox->ibm_vga8, false);
    loadFont("radiostars.font", &jukebox->radiostars, false);

    // Load images
    loadWsg("arrow10.wsg", &jukebox->arrow, false);
    loadWsg("jukebox.wsg", &jukebox->jukeboxSprite, false);

    loadWsg("quaver.wsg", &jukebox->quaver, false);
    loadWsg("doubleQuaver.wsg", &jukebox->doubleQuaver, false);
    loadWsg("fClef.wsg", &jukebox->fClef, false);
    loadWsg("halfNote.wsg", &jukebox->halfNote, false);
    loadWsg("taillessQuaver.wsg", &jukebox->taillessQuaver, false);
    loadWsg("trebleClef.wsg", &jukebox->trebleClef, false);
    loadWsg("twoTailQuaver.wsg", &jukebox->twoTailQuaver, false);
    loadWsg("wholeNote.wsg", &jukebox->wholeNote, false);

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

    if(jukebox->usSinceLastDecoration >= jukebox->usBetweenDecorations)
    {
        spawnDecoration = true;

        if(jukebox->isPlaying)
        {
            jukebox->usBetweenDecorations = MIN_DECORATION_DELAY_US_PLAYING + (esp_random() % (MAX_DECORATION_DELAY_US_PLAYING - MIN_DECORATION_DELAY_US_PLAYING));
        }
        else
        {
            jukebox->usBetweenDecorations = MIN_DECORATION_DELAY_US_PAUSED + (esp_random() % (MAX_DECORATION_DELAY_US_PAUSED - MIN_DECORATION_DELAY_US_PAUSED));
        }
        
    }

    for(uint8_t i = 0; i < MAX_DECORATIONS_ON_SCREEN; i++)
    {
        jukeboxDecoration_t* decoration = &jukebox->decorations[i];
        bool spawnedDecoration = false;
        if(spawnDecoration && decoration->graphic == 0)
        {
            uint8_t index = esp_random() % NUM_DECORATION_WSGS;
            decoration->graphic = jukeboxGetDecorationGraphic(index);
            decoration->x = (esp_random() % TFT_WIDTH) - (decoration->graphic->w / 2);
            decoration->y = TFT_HEIGHT - 1;
            decoration->rotateDeg = (esp_random() % (MAX_DECORATION_ROTATE_DEG * 2 + 1)) - MAX_DECORATION_ROTATE_DEG;

            if(jukeboxIsDecorationUpsideDownable(index) && (esp_random() % 4) == 0)
            {
                decoration->rotateDeg += 180;
            }

            if(decoration->rotateDeg < 0)
            {
                decoration->rotateDeg += 360;
            }
            
            if(jukebox->isPlaying)
            {
                decoration->speed = MIN_DECORATION_SPEED_PLAYING + (esp_random() % (MAX_DECORATION_SPEED_PLAYING - MIN_DECORATION_SPEED_PLAYING + 1));
            }
            else
            {
                decoration->speed = MIN_DECORATION_SPEED_PAUSED + (esp_random() % (MAX_DECORATION_SPEED_PAUSED - MIN_DECORATION_SPEED_PAUSED + 1));
            }
            jukebox->usSinceLastDecoration = 0;
            spawnDecoration = false;
            spawnedDecoration = true;
        }
        
        if(decoration->graphic != 0)
        {
            if(!spawnedDecoration)
            {
                decoration->y -= decoration->speed * (elapsedUs / 1000000.0);
            }

            drawWsg(decoration->graphic, decoration->x, round(decoration->y), false, false, decoration->rotateDeg);

            if(decoration->y < -decoration->graphic->h)
            {
                decoration->graphic = 0;
                decoration->x = 0;
                decoration->y = 0;
                decoration->rotateDeg = 0;
                decoration->speed = 0;
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
        const font_t* nameFont      = &(jukebox->radiostars);
        int16_t width = textWidth(nameFont, str_mode);
        int16_t yOff  = (TFT_HEIGHT - nameFont->height) / 2 - (nameFont->height + 2) * 2;
        drawText(nameFont, c533, str_mode, (TFT_WIDTH - width) / 2, yOff);
        // Draw category arrows if this submode has more than 1 category
        if ((jukebox->inMusicSubmode && ARRAY_SIZE(musicCategories) > 1) || ARRAY_SIZE(sfxCategories) > 1)
        {
            drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) - ARROW_OFFSET_FROM_TEXT - jukebox->arrow.w, yOff, false,
                    false, 0);
            drawWsg(&jukebox->arrow, ((TFT_WIDTH - width) / 2) + width + ARROW_OFFSET_FROM_TEXT, yOff, false, false, 180);
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
    //printf("bzr done\n");
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

wsg_t* jukeboxGetDecorationGraphic(uint8_t i)
{
    switch(i)
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
    switch(i)
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
