/**
 * @file jukebox.c
 * @author Brycey92 & VanillyNeko
 * @brief Equivalent to a sound-test mode from older games
 * @date 2023-08-26
 */

/*==============================================================================
 * Includes
 *============================================================================*/

#include "modeIncludeList.h"
#include "portableDance.h"
#include "jukebox.h"

/*==============================================================================
 * Defines
 *============================================================================*/

#define CORNER_OFFSET           14
#define JUKEBOX_SPRITE_Y_OFFSET 3
#define LINE_BREAK_LONG_Y       8
#define LINE_BREAK_SHORT_Y      3
#define ARROW_OFFSET_FROM_TEXT  8

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
 * Const variable, before structs
 *============================================================================*/

// Note, the length of this array is used to define jukebox_t.noteGraphics
// Also update jukeboxIsDecorationUpsideDownable() if something is added here
static const cnfsFileIdx_t noteGraphics[] = {
    QUAVER_WSG,          DOUBLE_QUAVER_WSG, F_CLEF_WSG,          HALF_NOTE_WSG,
    TAILLESS_QUAVER_WSG, TREBLE_CLEF_WSG,   TWO_TAIL_QUAVER_WSG, WHOLE_NOTE_WSG,
};

/*==============================================================================
 * Structs
 *============================================================================*/

typedef struct
{
    const cnfsFileIdx_t fIdx;
    const char* name;
} jukeboxSong_t;

typedef struct
{
    const swadgeMode_t* category;
    const jukeboxSong_t* songs;
    const uint8_t numSongs;
    const bool generalMidi;
    const bool shouldLoop;
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

    // Loaded MIDIs
    midiFile_t** bgmMidis;
    midiFile_t** sfxMidis;

    // Screen Decorations
    jukeboxDecoration_t decorations[MAX_DECORATIONS_ON_SCREEN];
    int64_t usSinceLastDecoration;
    uint32_t usBetweenDecorations;
    wsg_t noteGraphics[ARRAY_SIZE(noteGraphics)];
} jukebox_t;

/*==============================================================================
 * Prototypes
 *============================================================================*/

void jukeboxEnterMode(void);
void jukeboxExitMode(void);
void jukeboxButtonCallback(buttonEvt_t* evt);
void jukeboxMainLoop(int64_t elapsedUs);
void jukeboxBackgroundDrawCb(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

void jukeboxBzrDoneCb(void);
midiFile_t** jukeboxLoadCategories(const jukeboxCategory_t* categoryArray, uint8_t numCategories);
void jukeboxFreeCategories(const jukeboxCategory_t* categoryArray, uint8_t numCategories, midiFile_t** midis);

bool jukeboxIsDecorationUpsideDownable(cnfsFileIdx_t i);

/*==============================================================================
 * Const Variables
 *============================================================================*/

static const char jukeboxName[] = "Jukebox";

// Text
#ifdef SW_VOL_CONTROL
static const char str_bgm_muted[] = "Swadge music is muted!";
static const char str_sfx_muted[] = "Swadge SFX are muted!";
#endif
static const char str_mode[]       = "Mode";
static const char str_bgm[]        = "Music";
static const char str_sfx[]        = "SFX";
static const char str_leds[]       = "B: LEDs:";
static const char str_music_sfx[]  = "Pause: Music/SFX:";
static const char str_brightness[] = "Touch: LED Brightness:";
static const char str_stop[]       = ": Stop";
static const char str_play[]       = ": Play";

static const jukeboxSong_t bgm_roboRunner[] = {
    {
        .fIdx = ROBO_RUNNER_BGM_MID,
        .name = "Robo Runner BGM",
    },
};

static const jukeboxSong_t bgm_vectorTanks[] = {
    {
        .fIdx = VT_FIGHT_ON_MID,
        .name = "Fight On!",
    },
    {
        .fIdx = VT_FUNK_MID,
        .name = "Tanks Funk",
    },
    {
        .fIdx = VT_POP_MID,
        .name = "Tanks Pop",
    },
    {
        .fIdx = VT_READY_AIM_FIRE_MID,
        .name = "Ready, Aim, Fire!",
    },
    {
        .fIdx = VT_RISK_MID,
        .name = "Risk",
    },
    {
        .fIdx = VT_VECTOR_TANK_MID,
        .name = "Vector Tank",
    },
};

static const jukeboxSong_t bgm_cosplayCrunch[] = {
    {
        .fIdx = COSPLAY_CRUNCH_BGM_MID,
        .name = "Cosplay Crunch BGM",
    },
    {
        .fIdx = FAIRY_FOUNTAIN_MID,
        .name = "Fairy Fountain",
    },
    {
        .fIdx = HD_CREDITS_MID,
        .name = "Hot Dog Credits",
    },
};

static const jukeboxSong_t sfx_cosplayCrunch[] = {
    {
        .fIdx = CC_PACKAGE_GET_MID,
        .name = "Package Get",
    },
};

static const jukeboxSong_t bgm_picross[] = {
    {
        .fIdx = LULLABY_IN_NUMBERS_MID,
        .name = "Lullaby in Numbers",
    },
};

static const jukeboxSong_t bgm_megaPulse[] = {
    {
        .name = "INTRO STAGE",
        .fIdx = BGM_INTRO_STAGE_MID,
    },
    {
        .name = "PRE FIGHT",
        .fIdx = BGM_PRE_FIGHT_MID,
    },
    {
        .name = "POST FIGHT",
        .fIdx = BGM_POST_FIGHT_MID,
    },
    {
        .name = "DEADEYE CHIRPZI",
        .fIdx = BGM_DEADEYE_CHIRPZI_MID,
    },
    {
        .name = "BOSS DEADEYE CHIRPZI",
        .fIdx = BGM_BOSS_DEADEYE_CHIRPZI_MID,
    },
    {
        .name = "DRAIN BAT",
        .fIdx = BGM_DRAIN_BAT_MID,
    },
    {
        .name = "BOSS DRAIN BAT",
        .fIdx = BGM_BOSS_DRAIN_BAT_MID,
    },
    {
        .name = "FLARE GRYFFYN",
        .fIdx = BGM_FLARE_GRYFFYN_MID,
    },
    {
        .name = "BOSS FLARE GRIFFIN",
        .fIdx = BGM_BOSS_FLARE_GRIFFIN_MID,
    },
    {
        .name = "GRIND PANGOLIN",
        .fIdx = BGM_GRIND_PANGOLIN_MID,
    },
    {
        .name = "BOSS GRIND PANGOLIN",
        .fIdx = BGM_BOSS_GRIND_PANGOLIN_MID,
    },
    {
        .name = "KINETIC DONUT",
        .fIdx = BGM_KINETIC_DONUT_MID,
    },
    {
        .name = "BOSS KINETIC DONUT",
        .fIdx = BGM_BOSS_KINETIC_DONUT_MID,
    },
    {
        .name = "SEVER YAGATA",
        .fIdx = BGM_SEVER_YAGATA_MID,
    },
    {
        .name = "BOSS SEVER YAGATA",
        .fIdx = BGM_BOSS_SEVER_YAGATA_MID,
    },
    {
        .name = "SMASH GORILLA",
        .fIdx = BGM_SMASH_GORILLA_MID,
    },
    {
        .name = "BOSS SMASH GORILLA",
        .fIdx = BGM_BOSS_SMASH_GORILLA_MID,
    },
    {
        .name = "BOSS TRASH MAN",
        .fIdx = BGM_BOSS_TRASH_MAN_MID,
    },
    {
        .name = "BIGMA",
        .fIdx = BGM_BIGMA_MID,
    },
    {
        .name = "BOSS BIGMA",
        .fIdx = BGM_BOSS_BIGMA_MID,
    },
    {
        .name = "BOSS HANK WADDLE",
        .fIdx = BGM_BOSS_HANK_WADDLE_MID,
    },
    {
        .name = "RIP BARONESS",
        .fIdx = BGM_RIP_BARONESS_MID,
    },
    {
        .name = "STAGE SELECT",
        .fIdx = BGM_STAGE_SELECT_MID,
    },
    {
        .name = "NAME ENTRY",
        .fIdx = BGM_NAME_ENTRY_MID,
    },
};

static const jukeboxSong_t sfx_megaPulse[] = {
    {
        .name = "INTRO",
        .fIdx = BGM_INTRO_MID,
    },
    {
        .name = "GAME OVER",
        .fIdx = BGM_GAME_OVER_MID,
    },
    {
        .name = "1UP",
        .fIdx = SND_1UP_MID,
    },
    {
        .name = "BREAK",
        .fIdx = SND_BREAK_MID,
    },
    {
        .name = "CHECKPOINT",
        .fIdx = SND_CHECKPOINT_MID,
    },
    {
        .name = "COIN",
        .fIdx = SND_COIN_MID,
    },
    {
        .name = "DIE",
        .fIdx = SND_DIE_MID,
    },
    {
        .name = "HIT",
        .fIdx = SND_HIT_MID,
    },
    {
        .name = "HURT",
        .fIdx = SND_HURT_MID,
    },
    {
        .name = "JUMP 1",
        .fIdx = SND_JUMP_1_MID,
    },
    {
        .name = "JUMP 2",
        .fIdx = SND_JUMP_2_MID,
    },
    {
        .name = "JUMP 3",
        .fIdx = SND_JUMP_3_MID,
    },
    {
        .name = "LEVEL CLEAR JINGLE",
        .fIdx = BGM_LEVEL_CLEAR_JINGLE_MID,
    },
    {
        .name = "LEVEL CLEAR A",
        .fIdx = SND_LEVEL_CLEAR_A_MID,
    },
    {
        .name = "LEVEL CLEAR B",
        .fIdx = SND_LEVEL_CLEAR_B_MID,
    },
    {
        .name = "LEVEL CLEAR C",
        .fIdx = SND_LEVEL_CLEAR_C_MID,
    },
    {
        .name = "LEVEL CLEAR D",
        .fIdx = SND_LEVEL_CLEAR_D_MID,
    },
    {
        .name = "LEVEL CLEAR",
        .fIdx = SND_LEVEL_CLEAR_MID,
    },
    {
        .name = "SWSN CHOOSE SFX MID",
        .fIdx = SWSN_CHOOSE_SFX_MID,
    },
    {
        .name = "MENU DENY",
        .fIdx = SND_MENU_DENY_MID,
    },
    {
        .name = "SWSN MOVE SFX MID",
        .fIdx = SWSN_MOVE_SFX_MID,
    },
    {
        .name = "OUT OF TIME",
        .fIdx = SND_OUT_OF_TIME_MID,
    },
    {
        .name = "PAUSE",
        .fIdx = SND_PAUSE_MID,
    },
    {
        .name = "POWER UP",
        .fIdx = SND_POWER_UP_MID,
    },
    {
        .name = "SQUISH",
        .fIdx = SND_SQUISH_MID,
    },
    {
        .name = "TALLY",
        .fIdx = SND_TALLY_MID,
    },
    {
        .name = "WARP",
        .fIdx = SND_WARP_MID,
    },
    {
        .name = "WAVE BALL",
        .fIdx = SND_WAVE_BALL_MID,
    },
};

static const jukeboxSong_t bgm_danceNetwork[] = {
    {
        .name = "Pawn's Gambit",
        .fIdx = AP_PAWNS_GAMBIT_BGM_MID,
    },
    {
        .name = "Pawn's Gambit Percussion",
        .fIdx = AP_PAWNS_GAMBIT_PERCUSSION_MID,
    },
    {
        .name = "Ten Steps Ahead",
        .fIdx = AP_TEN_STEPS_AHEAD_BGM_MID,
    },
    {
        .name = "Ten Steps Ahead Percussion",
        .fIdx = AP_TEN_STEPS_AHEAD_PERCUSSION_MID,
    },
    {
        .name = "The Will to Win",
        .fIdx = AP_THE_WILL_TO_WIN_BGM_MID,
    },
    {
        .name = "The Will to Win Percussion",
        .fIdx = AP_THE_WILL_TO_WIN_PERCUSSION_MID,
    },
    {
        .name = "Final Path",
        .fIdx = AP_FINAL_PATH_BGM_MID,
    },
    {
        .name = "Final Path Percussion",
        .fIdx = AP_FINAL_PATH_PERCUSSION_MID,
    },
    {
        .name = "Return of the Valiant",
        .fIdx = AP_RETURN_OF_THE_VALIANT_BGM_MID,
    },
    {
        .name = "Return of the Valiant Percussion",
        .fIdx = AP_RETURN_OF_THE_VALIANT_PERCUSSION_MID,
    },
    {
        .name = "Next Move",
        .fIdx = AP_NEXT_MOVE_BGM_MID,
    },
    {
        .name = "Next Move Percussion",
        .fIdx = AP_NEXT_MOVE_PERCUSSION_MID,
    },
};

static const jukeboxSong_t bgm_credits[] = {
    {
        .fIdx = MAXIMUM_HYPE_CREDITS_MID,
        .name = "Maximum Hype",
    },
};

static const jukeboxSong_t bgm_tutorial[] = {
    {
        .fIdx = MAXIMUM_HYPE_CREDITS_TEASER_MID,
        .name = "Maximum Hype Teaser",
    },
};

static const jukeboxSong_t sfx_tutorial[] = {
    {
        .fIdx = INTROJINGLE_MID,
        .name = "Intro Jingle",
    },
};

static const jukeboxSong_t sfx_mainMenu[] = {
    {
        .fIdx = SECRET_MID,
        .name = "Secret",
    },
};

static const jukeboxSong_t bgm_swadgesona[] = {
    {
        .fIdx = SWSN_CREATOR_BGM1_MID,
        .name = "Creator BGM",
    },
};

static const jukeboxSong_t sfx_swadgesona[] = {
    {
        .fIdx = SWSN_CHOOSE_SFX_MID,
        .name = "Choose SFX",
    },
    {
        .fIdx = SWSN_MOVE_SFX_MID,
        .name = "Move SFX",
    },
};

static const jukeboxSong_t bgm_atrium[] = {
    {
        .fIdx = ATRTHEME1_MID,
        .name = "Atrium Theme 1",
    },
    {
        .fIdx = ATRTHEME2_MID,
        .name = "Atrium Theme 2",
    },
    {
        .fIdx = ATRVIBE_MID,
        .name = "Atrium Vibe",
    },
};

static const jukeboxSong_t bgm_findingFaces[] = {
    {
        .fIdx = FINDER_BGM_MENU_MID,
        .name = "Menu",
    },
    {
        .fIdx = LULLABY_IN_NUMBERS_MID,
        .name = "Lullaby in Numbers",
    },
    {
        .fIdx = FINDER_BGM_SLOW_MID,
        .name = "Slow",
    },
    {
        .fIdx = FINDER_BGM_MED_MID,
        .name = "Med",
    },
    {
        .fIdx = FINDER_BGM_FAST_MID,
        .name = "Fast",
    },
    {
        .fIdx = FINDER_BGM_DEATH_MID,
        .name = "Death",
    },
};

static const jukeboxSong_t sfx_findingFaces[] = {
    {
        .fIdx = FINDER_RIGHT_MID,
        .name = "Right",
    },
    {
        .fIdx = FINDER_WRONG_MID,
        .name = "Wrong",
    },
    {
        .fIdx = FINDER_WRONGER_MID,
        .name = "Wronger",
    },
    {
        .fIdx = FINDER_DIE_MID,
        .name = "Die",
    },
};

static const jukeboxCategory_t bgmCategories[] = {
    {
        .category    = &modePlatformer,
        .songs       = bgm_megaPulse,
        .numSongs    = ARRAY_SIZE(bgm_megaPulse),
        .generalMidi = true,
        .shouldLoop  = true,
    },
    {
        .category    = &cosCrunchMode,
        .songs       = bgm_cosplayCrunch,
        .numSongs    = ARRAY_SIZE(bgm_cosplayCrunch),
        .generalMidi = true,
        .shouldLoop  = true,
    },
    {
        .category    = &artilleryMode,
        .songs       = bgm_vectorTanks,
        .numSongs    = ARRAY_SIZE(bgm_vectorTanks),
        .generalMidi = true,
        .shouldLoop  = true,
    },
    {
        .category    = &danceNetworkMode,
        .songs       = bgm_danceNetwork,
        .numSongs    = ARRAY_SIZE(bgm_danceNetwork),
        .generalMidi = true,
        .shouldLoop  = true,
    },
    {
        .category    = &roboRunnerMode,
        .songs       = bgm_roboRunner,
        .numSongs    = ARRAY_SIZE(bgm_roboRunner),
        .generalMidi = true,
        .shouldLoop  = true,
    },
    {
        .category    = &findingFacesMode,
        .songs       = bgm_findingFaces,
        .numSongs    = ARRAY_SIZE(bgm_findingFaces),
        .generalMidi = true,
        .shouldLoop  = true,
    },
    {
        .category    = &modePicross,
        .songs       = bgm_picross,
        .numSongs    = ARRAY_SIZE(bgm_picross),
        .generalMidi = true,
        .shouldLoop  = true,
    },
    {
        .category    = &swsnCreatorMode,
        .songs       = bgm_swadgesona,
        .numSongs    = ARRAY_SIZE(bgm_swadgesona),
        .generalMidi = true,
        .shouldLoop  = true,
    },
    {
        .category    = &atriumMode,
        .songs       = bgm_atrium,
        .numSongs    = ARRAY_SIZE(bgm_atrium),
        .generalMidi = true,
        .shouldLoop  = true,
    },
    {
        .category    = &introMode,
        .songs       = bgm_tutorial,
        .numSongs    = ARRAY_SIZE(bgm_tutorial),
        .generalMidi = true,
        .shouldLoop  = true,
    },
    {
        .category    = &modeCredits,
        .songs       = bgm_credits,
        .numSongs    = ARRAY_SIZE(bgm_credits),
        .generalMidi = true,
        .shouldLoop  = true,
    },
};

static const jukeboxCategory_t sfxCategories[] = {
    {
        .category    = &modePlatformer,
        .songs       = sfx_megaPulse,
        .numSongs    = ARRAY_SIZE(sfx_megaPulse),
        .generalMidi = true,
        .shouldLoop  = false,
    },
    {
        .category    = &cosCrunchMode,
        .songs       = sfx_cosplayCrunch,
        .numSongs    = ARRAY_SIZE(sfx_cosplayCrunch),
        .generalMidi = true,
        .shouldLoop  = false,
    },
    {
        .category    = &findingFacesMode,
        .songs       = sfx_findingFaces,
        .numSongs    = ARRAY_SIZE(sfx_findingFaces),
        .generalMidi = true,
        .shouldLoop  = false,
    },
    {
        .category    = &swsnCreatorMode,
        .songs       = sfx_swadgesona,
        .numSongs    = ARRAY_SIZE(sfx_swadgesona),
        .generalMidi = true,
        .shouldLoop  = false,
    },
    {
        .category    = &introMode,
        .songs       = sfx_tutorial,
        .numSongs    = ARRAY_SIZE(sfx_tutorial),
        .generalMidi = true,
        .shouldLoop  = false,
    },
    {
        .category    = &mainMenuMode,
        .songs       = sfx_mainMenu,
        .numSongs    = ARRAY_SIZE(sfx_mainMenu),
        .generalMidi = true,
        .shouldLoop  = false,
    },
};

/*==============================================================================
 * Variables
 *============================================================================*/

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

jukebox_t* jukebox;

/*============================================================================
 * Functions
 *==========================================================================*/

/**
 * Initializer for jukebox
 */
void jukeboxEnterMode(void)
{
    // Allocate zero'd memory for the mode
    jukebox = heap_caps_calloc(1, sizeof(jukebox_t), MALLOC_CAP_8BIT);

    // Enter music submode
    jukebox->inMusicSubmode = true;

    // Load fonts
    loadFont(RADIOSTARS_FONT, &jukebox->radiostars, false);

    // Load images
    loadWsg(ARROW_10_WSG, &jukebox->arrow, false);
    loadWsg(JUKEBOX_WSG, &jukebox->jukeboxSprite, false);

    for (int i = 0; i < ARRAY_SIZE(noteGraphics); i++)
    {
        loadWsg(noteGraphics[i], &jukebox->noteGraphics[i], false);
    }

    // Load midis
    jukebox->bgmMidis = jukeboxLoadCategories(bgmCategories, ARRAY_SIZE(bgmCategories));
    jukebox->sfxMidis = jukeboxLoadCategories(sfxCategories, ARRAY_SIZE(sfxCategories));

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
    freeFont(&jukebox->radiostars);

    // Free images
    freeWsg(&jukebox->arrow);
    freeWsg(&jukebox->jukeboxSprite);

    for (int i = 0; i < ARRAY_SIZE(noteGraphics); i++)
    {
        freeWsg(&jukebox->noteGraphics[i]);
    }

    // Free allocated midis
    jukeboxFreeCategories(bgmCategories, ARRAY_SIZE(bgmCategories), jukebox->bgmMidis);
    jukeboxFreeCategories(sfxCategories, ARRAY_SIZE(sfxCategories), jukebox->sfxMidis);

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
                        soundGetPlayerBgm()->loop = bgmCategories[jukebox->categoryIdx].shouldLoop;
                    }

                    if (bgmCategories[jukebox->categoryIdx].generalMidi)
                    {
                        midiGmOn(soundGetPlayerBgm());
                    }
                    else
                    {
                        midiGmOff(soundGetPlayerBgm());
                    }

                    soundPlayBgmCb(&jukebox->bgmMidis[jukebox->categoryIdx][jukebox->songIdx], BZR_STEREO,
                                   jukeboxBzrDoneCb);
                }
                else
                {
                    if (soundGetPlayerSfx() != NULL)
                    {
                        soundGetPlayerSfx()->loop = sfxCategories[jukebox->categoryIdx].shouldLoop;
                    }

                    if (sfxCategories[jukebox->categoryIdx].generalMidi)
                    {
                        midiGmOn(soundGetPlayerSfx());
                    }
                    else
                    {
                        midiGmOff(soundGetPlayerSfx());
                    }

                    soundPlaySfxCb(&jukebox->sfxMidis[jukebox->categoryIdx][jukebox->songIdx], BZR_STEREO,
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
                length = ARRAY_SIZE(bgmCategories);
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
                length = ARRAY_SIZE(bgmCategories);
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
                length = bgmCategories[jukebox->categoryIdx].numSongs;
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
                length = bgmCategories[jukebox->categoryIdx].numSongs;
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
 * @brief Update the display by drawing the current state of affairs
 *
 * @param elapsedUs The time since this was last called
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
            uint8_t index         = esp_random() % ARRAY_SIZE(noteGraphics);
            decoration->graphic   = &jukebox->noteGraphics[index];
            decoration->x         = (esp_random() % TFT_WIDTH) - (decoration->graphic->w / 2);
            decoration->y         = TFT_HEIGHT - 1;
            decoration->rotateDeg = (esp_random() % (MAX_DECORATION_ROTATE_DEG * 2 + 1)) - MAX_DECORATION_ROTATE_DEG;

            if (jukeboxIsDecorationUpsideDownable(noteGraphics[index]) && (esp_random() % 4) == 0)
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
            categoryName = bgmCategories[jukebox->categoryIdx].category->modeName;
            songName     = bgmCategories[jukebox->categoryIdx].songs[jukebox->songIdx].name;
            songTypeName = str_bgm;
            numSongs     = bgmCategories[jukebox->categoryIdx].numSongs;
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
            categoryName = sfxCategories[jukebox->categoryIdx].category->modeName;
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
        if ((jukebox->inMusicSubmode && ARRAY_SIZE(bgmCategories) > 1) || ARRAY_SIZE(sfxCategories) > 1)
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

/**
 * @brief Callback after a song is done playing. Handles looping
 */
void jukeboxBzrDoneCb(void)
{
    // printf("bzr done\n");
    const jukeboxCategory_t* category;
    if (jukebox->inMusicSubmode)
    {
        category = bgmCategories;
    }
    else
    {
        category = sfxCategories;
    }

    if (!category[jukebox->categoryIdx].shouldLoop)
    {
        jukebox->isPlaying = false;
    }
}

/**
 * @brief Load all MIDIs referenced by the categories
 *
 * @param categoryArray A list of categories, for reference
 * @param numCategories The number of categories, for reference
 * @return A 2D array of loaded MIDIs aligned to the categories and their songs
 */
midiFile_t** jukeboxLoadCategories(const jukeboxCategory_t* categoryArray, uint8_t numCategories)
{
    // Allocate an array of pointers, one for each category
    midiFile_t** loadedMidis = heap_caps_calloc(numCategories, sizeof(midiFile_t*), MALLOC_CAP_SPIRAM);
    // For each category
    for (int categoryIdx = 0; categoryIdx < numCategories; categoryIdx++)
    {
        // Convenience
        const jukeboxCategory_t* category = &categoryArray[categoryIdx];

        // Allocate an array of midiFile_t
        loadedMidis[categoryIdx] = heap_caps_calloc(category->numSongs, sizeof(midiFile_t), MALLOC_CAP_SPIRAM);

        // For each song
        for (int songIdx = 0; songIdx < category->numSongs; songIdx++)
        {
            // Load it
            loadMidiFile(category->songs[songIdx].fIdx, &loadedMidis[categoryIdx][songIdx], true);
        }
    }
    return loadedMidis;
}

/**
 * @brief Free all loaded MIDIs
 *
 * @param categoryArray A list of categories, for reference
 * @param numCategories The number of categories, for reference
 * @param midis A 2D array of MIDIs to free
 */
void jukeboxFreeCategories(const jukeboxCategory_t* categoryArray, uint8_t numCategories, midiFile_t** midis)
{
    for (uint8_t categoryIdx = 0; categoryIdx < numCategories; categoryIdx++)
    {
        for (uint8_t songIdx = 0; songIdx < categoryArray[categoryIdx].numSongs; songIdx++)
        {
            unloadMidiFile(&midis[categoryIdx][songIdx]);
        }
        heap_caps_free(midis[categoryIdx]);
    }
    heap_caps_free(midis);
    midis = NULL;
}

/**
 * @brief Return if a graphic can be rotated 180 degrees
 *
 * @param i The index of the graphic
 * @return true if it can be flipped, false if it can't
 */
bool jukeboxIsDecorationUpsideDownable(cnfsFileIdx_t i)
{
    switch (i)
    {
        default:
        case F_CLEF_WSG:
        case TREBLE_CLEF_WSG:
        {
            return false;
        }
        case QUAVER_WSG:
        case DOUBLE_QUAVER_WSG:
        case HALF_NOTE_WSG:
        case TAILLESS_QUAVER_WSG:
        case TWO_TAIL_QUAVER_WSG:
        case WHOLE_NOTE_WSG:
        {
            return true;
        }
    }
}
