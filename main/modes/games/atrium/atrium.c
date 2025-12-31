#include "atrium.h"
#include "nameList.h"
#include "swadgesona.h"
#include "swadgePass.h"
#include "modeIncludeList.h"

//---------------------------------------------------------------------------------//
// DEFINES
//---------------------------------------------------------------------------------//

#define TFT_WIDTH  280
#define TFT_HEIGHT 240

// Lobby
#define SONA_PER            4
#define MAX_SWADGESONA_IDXS (MAX_NUM_SWADGE_PASSES / SONA_PER) - (MAX_NUM_SWADGE_PASSES % SONA_PER) / SONA_PER
#define ANIM_TIMER_MS       16667
#define LOBBY_ARROW_Y       TFT_HEIGHT / 2 - 12

// Profile
#define CARDTEXTPAD 4
#define SONALOC_X   24
#define SONALOC_Y   36

#define ATRIUM_PROFILE_NVS_NAMESPACE "atrium"
#define TROPHY_NVS_NAMESPACE         "trophy"
#define TROPHY_POINTS_NVS_KEY        "points"
#define ATRIUM_PACKEDKEY             "packedProfile"
#define ATRIUM_CREATEDKEY            "profileCreated"
#define TEAMKEY                      "team"

//---------------------------------------------------------------------------------//
// CONSTS
//---------------------------------------------------------------------------------//

// CNFS index lists

// Sona bodies
static const cnfsFileIdx_t sonaBodies[] = {
    BODY_1_WSG, BODY_2_WSG, DAISY_WSG, FALLOUT_WSG, HANDSOME_WSG, KIRBY_WSG, LINK_WSG,   MARIO_WSG, PACMAN_WSG,
    PEACH_WSG,  RAYMAN_WSG, SANIC_WSG, SORA_WSG,    STAFF_WSG,    STEVE_WSG, STEVEN_WSG, WALDO_WSG, ZELDA_WSG,
};

// concert sona bodies
static const cnfsFileIdx_t concertSonaBodies[] = {
    BASS_0_WSG,     BASS_1_WSG,     BASS_2_WSG,     BASS_3_WSG,     BASS_4_WSG,     GUITAR_1_WSG,
    GUITAR_2_WSG,   GUITAR_3_WSG,   GUITAR_4_WSG,   GUITAR_5_WSG,   GUITAR_6_WSG,   GUITAR_7_WSG,
    GUITAR_8_WSG,   TRIANGLE_0_WSG, TRIANGLE_1_WSG, TRIANGLE_2_WSG, TRIANGLE_3_WSG, TRIANGLE_4_WSG,
    TRIANGLE_5_WSG, TRIANGLE_6_WSG, TRIANGLE_7_WSG, TRIANGLE_8_WSG, TRIANGLE_9_WSG, TRUMPET_0_WSG,
    TRUMPET_1_WSG,  TRUMPET_2_WSG,  TRUMPET_3_WSG,  TRUMPET_4_WSG,  TRUMPET_5_WSG,
};

static const cnfsFileIdx_t uiImages[] = {
    ARROWBUTTON_1_WSG, ARROWBUTTON_2_WSG, ABUTTON_1_WSG,   ABUTTON_2_WSG, BBUTTON_1_WSG,  BBUTTON_2_WSG, ATRIUMLOGO_WSG,
    KEEPON_WSG,        LOADING_1_WSG,     LOADING_2_WSG,   LOADING_3_WSG, LOADING_4_WSG,  LOADING_5_WSG, LOADING_6_WSG,
    LOADING_7_WSG,     LOADING_8_WSG,     GOLD_TROPHY_WSG, ARROW_WSG,     CARDSELECT_WSG, ATRSAVE1_WSG,  ATRSAVE2_WSG,
};

static const cnfsFileIdx_t bgImages[] = {
    GAZEBO_WSG,    ATRIUMPLANT_1_WSG, ARCADEBOT_WSG, ARCADE_1_WSG,  ARCADE_3_WSG,
    ARCADE_5_WSG,  ARCADE_7_WSG,      ARCADE_9_WSG,  ARCADE_11_WSG, CONCERTBOT_WSG,
    CONCERT_1_WSG, CONCERT_3_WSG,     CONCERT_5_WSG, CONCERT_7_WSG, CONCERT_9_WSG,

};

static const cnfsFileIdx_t cardImages[] = {
    CARDSTAFF_WSG, CARDGEN_WSG,    CARDBLOSS_WSG, CARDBUBB_WSG,   CARDDINO_WSG, CARDMAGFEST_WSG, CARDMUSIC_WSG,
    CARDSPACE_WSG, CARDARCADE_WSG, CARDSTARS_WSG, CARDSUNSET_WSG, CARDMIVS_WSG, CARDLEOPARD_WSG,
};

static const cnfsFileIdx_t midiBGM[] = {
    ATRTHEME1_MID,
    ATRTHEME2_MID,
};

static const cnfsFileIdx_t midiSFX[] = {
    SWSN_CHOOSE_SFX_MID,
    SWSN_MOVE_SFX_MID,
};

static const cnfsFileIdx_t fontsIdxs[] = {
    OXANIUM_13MED_FONT,
};

static const cnfsFileIdx_t teams[] = {
    TEAMRED_WSG,
    TEAMBLUE_WSG,
    TEAMYELLOW_WSG,
};

const char atriumModeName[] = "Atrium";

const char ATR_TAG[] = "ATR";

// Strings

static const char* const fact0[] = {
    "PB&J", "BLT", "Cheese", "Reuben", "Hoagie", "Ice Cream", "Hot Dog", "Knuckle",
};

static const char* const fact1[] = {
    "Bard", "Superfan", "Pinball Wizard", "Maker", "Sharpshooter", "Trashman", "Speed Runner", "Medic",
};

static const char* const fact2[] = {
    "Arena", "Arcade", "Gazebo", "Soapbox", "Marketplace", "Panels", "Main Stage", "Tabletop",
};

static const char* const preambles[] = {
    "My Sandwich: ",
    "I am a: ",
    "Fave place: ",
};

static const char* const editPromptText[] = {
    "Choose Card", "Pick Sandwich", "Choose Identity", "Choose Location", "Save Profile", "Saved!",
};

// Trophy Case
const trophyData_t atriumTrophies[] = {
    {
        .title       = "Welcome to the Atrium",
        .description = "We've got Music And Games",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .hidden      = true,
        .identifier  = NULL,
    },
    {
        .title       = "Join the Red Cats",
        .description = "Shred the Signal",
        .image       = TEAMRED_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .hidden      = true,
        .identifier  = NULL,
    },
    {
        .title       = "Join the Blue Bots",
        .description = "Keep the Beat",
        .image       = TEAMBLUE_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .hidden      = true,
        .identifier  = NULL,
    },

    {
        .title       = "Join the Big Yellows",
        .description = "RemiXing the World",
        .image       = TEAMYELLOW_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .hidden      = true,
        .identifier  = NULL,
    },
    {
        .title       = "Collector",
        .description = "Find 10 SwadgePass profiles",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 10,
        .hidden      = false,
        .identifier  = NULL,
    },
    {
        .title       = "Social Butterfly",
        .description = "Find 30 SwadgePass profiles",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 30,
        .hidden      = false,
        .identifier  = NULL,
    },

    {
        .title       = "John Magfest Himself",
        .description = "Find 100 SwadgePass profiles",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 100,
        .hidden      = false,
        .identifier  = NULL,
    },

    {
        .title       = "Red Team's Score",
        .description = "Shred the Competition",
        .image       = TEAMRED_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 150000,
        .hidden      = false,
        .identifier  = NULL,
    },
    {
        .title       = "Blue Team's Score",
        .description = "Rhythm Rules",
        .image       = TEAMBLUE_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 150000,
        .hidden      = false,
        .identifier  = NULL,
    },
    {
        .title       = "Yellow Team's Score",
        .description = "Number One, as Expected",
        .image       = TEAMYELLOW_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 150000,
        .hidden      = false,
        .identifier  = NULL,
    },

};

const trophySettings_t atriumTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 4,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = "atriumTrophies",
};

const trophyDataList_t atriumTrophyData = {
    .settings = &atriumTrophySettings,
    .list     = atriumTrophies,
    .length   = ARRAY_SIZE(atriumTrophies),
};

//---------------------------------------------------------------------------------//
// ENUMS
//---------------------------------------------------------------------------------//

typedef enum
{
    ATR_TITLE,        /// TITLE SCREEN
    ATR_DISPLAY,      /// GAZEBO ETC
    ATR_SELECT,       /// PICKING A SONA TO LOOK AT
    ATR_PROFILE,      /// LOOKING AT A PROFILE
    ATR_EDIT_PROFILE, /// EDITING MY PROFILE
} atriumState;

typedef enum
{
    BG_GAZEBO,
    BG_ARCADE,
    BG_CONCERT,
    BG_COUNT
} lobbyState_t;

//---------------------------------------------------------------------------------//
// STRUCTS
//---------------------------------------------------------------------------------//

typedef struct
{
    int8_t cardSelect;
    int8_t fact0;
    int8_t fact1;
    int8_t fact2;
    int8_t numPasses;      // Number of other unique passes encountered
    int32_t packedProfile; // card select 0-3, fact0 4-7, fact1 8-11, fact2 12-15, team 16-19, numpasses 20-28
    swadgesona_t swsn;     // Swadgesona data
    int32_t points;
    int8_t team; // 0 = red, 1 = blue, 2 = yellow
} userProfile_t;

typedef struct
{
    // Data
    wsg_t bodies[ARRAY_SIZE(sonaBodies)];
    wsg_t concertBodies[ARRAY_SIZE(concertSonaBodies)];
    wsg_t backgroundImages[ARRAY_SIZE(bgImages)];
    wsg_t uiElements[ARRAY_SIZE(uiImages)];
    wsg_t cards[ARRAY_SIZE(cardImages)];
    wsg_t teamElements[ARRAY_SIZE(teams)];
    font_t fonts[ARRAY_SIZE(fontsIdxs)];
    midiFile_t bgm[ARRAY_SIZE(midiBGM)];
    midiFile_t sfx[ARRAY_SIZE(midiSFX)];

    // Profile data
    userProfile_t loadedProfile; // Loaded profile for drawing
    swadgesona_t* profileSona;

    // Main
    atriumState state;
    int8_t numRemoteSwsn;
    userProfile_t sonaList[MAX_NUM_SWADGE_PASSES];

    // Lobbies
    lobbyState_t lbState;
    uint8_t lobbySwsnIdxs[MAX_SWADGESONA_IDXS];
    int64_t animTimer;
    int64_t animBodyTimer;
    int loadBodyAnims;
    int loadAnims;
    bool fakeLoad;
    bool shuffle;
    bool loadedProfs;
    bool drawnProfs;
    bool loadedBodies;
    bool concertLoadedBodies;
    bool animDirection;
    int8_t page;
    int8_t lastPage;
    int8_t remSwsn;
    int8_t totalPages;
    int8_t selectedArrow;
    int8_t bodyIdx[SONA_PER];
    int8_t concertBodyIdx[SONA_PER];

    // BGM
    midiPlayer_t* player;

    // SwadgePass List
    list_t spList;

    // Profile viewer/editor
    int selection;
    int xloc;
    int yloc;
    bool drawSaved;
    int32_t created;
    bool loadedTeams;
    int32_t points;

    // SwadgePass Profile
    userProfile_t spProfile;

    // LEDs
    led_t leds[CONFIG_NUM_LEDS];
    int8_t ledChase;

} atrium_t;

//---------------------------------------------------------------------------------//
// FUNCTION DECLARATIONS
//---------------------------------------------------------------------------------//

// Main
static void atriumEnterMode(void);
static void atriumExitMode(void);
static void atriumMainLoop(int64_t elapsedUs);

// Editors
static void editProfile(buttonEvt_t* evt, uint64_t elapsedUs);
static void viewProfile(buttonEvt_t* evt, uint64_t elapsedUs);

// Draw
static void drawAtriumTitle(uint64_t elapsedUs);
static void drawLobbies(buttonEvt_t* evt, uint64_t elapsedUs);
void shuffleSonas(void);
static void drawSonas(int8_t page, uint64_t elapsedUs);
static void drawArcade(uint64_t elapsedUs);
static void drawConcert(uint64_t elapsedUs);
static void drawGazebo(uint64_t elapsedUs);
static void drawGazeboForeground(uint64_t elapsedUs);
static void drawLobbyArrows(int selected);
static void drawCard(userProfile_t profile, bool local, uint64_t elapsedUs);
void drawEditSelection(buttonEvt_t* evt, int yloc);
void drawEditUI(buttonEvt_t* evt, int yloc, bool direction);
void drawSonaSelector(buttonEvt_t evt, int selection);
static void drawConcertBodies(int8_t sonas, uint64_t elapsedUs);

// Swadgepass
static void atriumAddSP(struct swadgePassPacket* packet);
void loadProfiles(int maxProfiles, int page);
userProfile_t loadProfileFromNVS(void);
void packProfileData(userProfile_t* profile);
void unpackProfileData(userProfile_t* profile);
void updateTeamScores(void);

// LEDS
static void atrSetLeds(int team, uint64_t elapsedUs);
static void atrClearLeds(void);

//---------------------------------------------------------------------------------//
// VARIABLES
//---------------------------------------------------------------------------------//

// Main variables
swadgeMode_t atriumMode = {
    .modeName          = atriumModeName, // Assign the name we created here
    .wifiMode          = false,          // If we want WiFi
    .overrideUsb       = false,          // Overrides the default USB behavior.
    .usesAccelerometer = false,          // If we're using motion controls
    .usesThermometer   = false,          // If we're using the internal thermometer
    .overrideSelectBtn = false, // The select/Menu button has a default behavior. If you want to override it, you can
                                // set this to true but you'll need to re-implement the behavior.
    .fnEnterMode              = atriumEnterMode, // The enter mode function
    .fnExitMode               = atriumExitMode,  // The exit mode function
    .fnMainLoop               = atriumMainLoop,  // The loop function
    .fnAudioCallback          = NULL,            // If the mode uses the microphone
    .fnBackgroundDrawCallback = NULL,            // Draws a section of the display
    .fnEspNowRecvCb           = NULL,            // If using Wifi, add the receive function here
    .fnEspNowSendCb           = NULL,            // If using Wifi, add the send function here
    .fnAdvancedUSB            = NULL,            // If using advanced USB things.
    .trophyData               = &atriumTrophyData,
    .fnAddToSwadgePassPacket  = atriumAddSP, // function to add data to swadgepass packet
};

atrium_t* atr;

//---------------------------------------------------------------------------------//
// FUNCTIONS
//---------------------------------------------------------------------------------//

static void atriumEnterMode(void)
{
    // Initialize memory
    atr = (atrium_t*)heap_caps_calloc(1, sizeof(atrium_t), MALLOC_CAP_8BIT);

    // Load images with a common decoder and decode space
    {
        // Initialize memory for a lot of big image loads
        heatshrink_decoder* hsd = heatshrink_decoder_alloc(256, 8, 4);
        uint8_t* decodeSpace
            = heap_caps_calloc(1, 4 + sizeof(paletteColor_t) * TFT_HEIGHT * TFT_WIDTH, MALLOC_CAP_SPIRAM);

        for (int idx = 0; idx < ARRAY_SIZE(sonaBodies); idx++)
        {
            loadWsgInplace(sonaBodies[idx], &atr->bodies[idx], true, decodeSpace, hsd);
        }

        for (int idx = 0; idx < ARRAY_SIZE(uiImages); idx++)
        {
            loadWsgInplace(uiImages[idx], &atr->uiElements[idx], true, decodeSpace, hsd);
        }

        for (int idx = 0; idx < ARRAY_SIZE(bgImages); idx++)
        {
            loadWsgInplace(bgImages[idx], &atr->backgroundImages[idx], true, decodeSpace, hsd);
        }

        for (int idx = 0; idx < ARRAY_SIZE(cardImages); idx++)
        {
            loadWsgInplace(cardImages[idx], &atr->cards[idx], true, decodeSpace, hsd);
        }
        for (int idx = 0; idx < ARRAY_SIZE(concertSonaBodies); idx++)
        {
            loadWsgInplace(concertSonaBodies[idx], &atr->concertBodies[idx], true, decodeSpace, hsd);
        }

        heap_caps_free(decodeSpace);
        heatshrink_decoder_free(hsd);
    }

    for (int idx = 0; idx < ARRAY_SIZE(midiBGM); idx++)
    {
        loadMidiFile(midiBGM[idx], &atr->bgm[idx], true);
    }

    for (int idx = 0; idx < ARRAY_SIZE(midiSFX); idx++)
    {
        loadMidiFile(midiSFX[idx], &atr->sfx[idx], true);
    }

    for (int idx = 0; idx < ARRAY_SIZE(fontsIdxs); idx++)
    {
        loadFont(fontsIdxs[idx], &atr->fonts[idx], true);
    }

    for (int idx = 0; idx < 3; idx++)
    {
        loadWsg(teams[idx], &atr->teamElements[idx], true);
    }

    // Swadgepass
    getSwadgePasses(&atr->spList, &atriumMode, true);
    node_t* spNode = atr->spList.first;
    int i          = 0;
    atr->loadAnims = 0;

    trophyUpdate(&atriumTrophyData.list[0], 1, true); // Award "Welcome to the Atrium" trophy

    while (spNode)
    {
        // Make a convenience pointer to the data in this node
        swadgePassData_t* spd      = (swadgePassData_t*)spNode->val;
        atr->sonaList[i].swsn.core = spd->data.packet.swadgesona.core;
        setUsernameFrom32(&atr->sonaList[i].swsn.name, spd->data.packet.swadgesona.core.packedName);

        atr->sonaList[i].packedProfile = spd->data.packet.atrium.packedProfile;
        atr->sonaList[i].points        = spd->data.packet.atrium.points;

        // If the data hasn't been used yet
        if (!isPacketUsedByMode(spd, &atriumMode))
        {
            // Print some packet data
            ESP_LOGI("SP", "Receive from %s. Preamble is %" PRIu16, spd->key, spd->data.packet.preamble);

            // Mark the packet as used
            setPacketUsedByMode(spd, &atriumMode, true);
        }

        // Iterate to the next data
        i++;
        spNode = spNode->next;
    }

    atr->numRemoteSwsn = i;

    trophyUpdate(&atriumTrophyData.list[4], atr->numRemoteSwsn, true); // update count for 10 passes
    trophyUpdate(&atriumTrophyData.list[5], atr->numRemoteSwsn, true); // count for 30 passes
    trophyUpdate(&atriumTrophyData.list[6], atr->numRemoteSwsn, true); // count for 100 passes

    // I don't really need an if statement here so i just deleted it, it was causing more problems than it was solving

    atr->page       = 0;
    atr->totalPages = (atr->numRemoteSwsn / SONA_PER) + ((atr->numRemoteSwsn % SONA_PER) ? 1 : 0);
    atr->remSwsn    = atr->numRemoteSwsn % SONA_PER;

    ESP_LOGI(ATR_TAG, "Num remote swsn: %" PRId8 ", total pages: %" PRId8, atr->numRemoteSwsn, atr->totalPages);

    // BGM
    atr->player       = globalMidiPlayerGet(MIDI_BGM);
    atr->player->loop = true;
    midiGmOn(atr->player);
    globalMidiPlayerPlaySong(&atr->bgm[0], MIDI_BGM);
    globalMidiPlayerSetVolume(MIDI_BGM, 13);

    // profile created yet?
    if (!readNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, ATRIUM_CREATEDKEY, &atr->created))
    {
        ESP_LOGI(ATR_TAG, "No profile found in NVS, generating random profile");
        atr->created = 0;
        int team     = rand() % 3;
        writeNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, TEAMKEY, team);
        atr->loadedProfs = false;
        atr->state       = ATR_EDIT_PROFILE; // go to profile edit if no profile yet
    }
    else
    {
        atr->created = 1;
        atr->state   = ATR_TITLE; // else go to title screen
        loadProfileFromNVS();     // Immediately update the SP count when entering the mode for the local user
    }
}

static void atriumExitMode(void)
{
    // Turn off BGM
    midiGmOff(atr->player);

    // Unload
    for (int idx = 0; idx < ARRAY_SIZE(fontsIdxs); idx++)
    {
        freeFont(&atr->fonts[idx]);
    }

    for (int idx = 0; idx < ARRAY_SIZE(midiSFX); idx++)
    {
        unloadMidiFile(&atr->sfx[idx]);
    }

    for (int idx = 0; idx < ARRAY_SIZE(midiBGM); idx++)
    {
        unloadMidiFile(&atr->bgm[idx]);
    }

    for (int idx = 0; idx < ARRAY_SIZE(cardImages); idx++)
    {
        freeWsg(&atr->cards[idx]);
    }

    for (int idx = 0; idx < ARRAY_SIZE(bgImages); idx++)
    {
        freeWsg(&atr->backgroundImages[idx]);
    }

    for (int idx = 0; idx < ARRAY_SIZE(uiImages); idx++)
    {
        freeWsg(&atr->uiElements[idx]);
    }

    for (int idx = 0; idx < ARRAY_SIZE(sonaBodies); idx++)
    {
        freeWsg(&atr->bodies[idx]);
    }

    for (int idx = 0; idx < 3; idx++)
    {
        freeWsg(&atr->teamElements[idx]);
    }

    freeWsg(&atr->loadedProfile.swsn.image);

    heap_caps_free(atr);
}

static void atriumMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    switch (atr->state)
    {
        case ATR_TITLE:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down) // if the button is pressed down on the title screen
                {
                    if ((evt.button & PB_A))
                    {
                        atr->state     = ATR_DISPLAY;
                        atr->loadAnims = 0;
                        globalMidiPlayerPlaySong(&atr->bgm[1], MIDI_BGM); // change BGM
                        globalMidiPlayerPlaySong(&atr->sfx[0], MIDI_SFX); // play choose sound
                    }
                    else if ((evt.button & PB_B))
                    {
                        atr->state = ATR_EDIT_PROFILE;                    // if B is pressed, go to edit profile view
                        globalMidiPlayerPlaySong(&atr->sfx[0], MIDI_SFX); // play choose sound
                    }
                }
            }
            drawAtriumTitle(elapsedUs);
            atr->loadedProfs = false;
            shuffleSonas();
            updateTeamScores();
            atrSetLeds(3, elapsedUs);

            break;
        }

        case ATR_DISPLAY:
        {
            atrClearLeds();
            // Handle input
            while (checkButtonQueueWrapper(&evt))
            {
                // Don't accept button input while the fake loading screen is shown
                if (0 == atr->fakeLoad)
                {
                    continue;
                }

                atr->lastPage      = atr->page;
                atr->selectedArrow = 0;
                if (evt.down)
                {
                    if (evt.button & PB_LEFT)
                    {
                        atr->page--;
                        if (atr->page < 0)
                        {
                            atr->page = 0;
                        }
                        globalMidiPlayerPlaySong(&atr->sfx[1], MIDI_SFX); // play move sound
                        atr->selectedArrow = 1;
                    }
                    else if (evt.button & PB_RIGHT)
                    {
                        atr->page++;
                        if (atr->page > (atr->numRemoteSwsn - 1) / 4)
                        {
                            atr->page = (atr->numRemoteSwsn - 1) / 4;
                        }
                        globalMidiPlayerPlaySong(&atr->sfx[1], MIDI_SFX); // play move sound
                        atr->selectedArrow = 4;
                    }
                    else if (evt.button & PB_UP)
                    {
                        if (atr->lbState > 0)
                        {
                            atr->lbState--;
                            atr->loadAnims = 0;
                            globalMidiPlayerPlaySong(&atr->sfx[1], MIDI_SFX); // play move sound
                        }
                        atr->selectedArrow = 3;
                    }
                    else if (evt.button & PB_DOWN)
                    {
                        if (atr->lbState < 2)
                        {
                            atr->lbState++;
                            atr->loadAnims = 0;
                            globalMidiPlayerPlaySong(&atr->sfx[1], MIDI_SFX); // play move sound
                        }
                        atr->selectedArrow = 2;
                    }
                    else if (evt.button & PB_A)
                    {
                        atr->state     = ATR_SELECT;
                        atr->selection = 0;
                        globalMidiPlayerPlaySong(&atr->sfx[0], MIDI_SFX); // play choose sound
                    }
                    else if (evt.button & PB_B)
                    {
                        atr->state = ATR_TITLE;
                        globalMidiPlayerPlaySong(&atr->bgm[0], MIDI_BGM); // change BGM
                        globalMidiPlayerPlaySong(&atr->sfx[0], MIDI_SFX); // play choose sound
                    }
                }
            }
            drawLobbies(&evt, elapsedUs);

            break;
        }
        case ATR_SELECT:
        {
            int maxSelect;
            if (atr->totalPages == atr->page + 1)
            {
                maxSelect = atr->remSwsn - 1;
                ESP_LOGI(ATR_TAG, "maxselect is %d", maxSelect);
            }
            else
            {
                maxSelect = SONA_PER - 1;
            }

            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if ((evt.button & PB_A))
                    {
                        atr->state = ATR_PROFILE;
                        globalMidiPlayerPlaySong(&atr->sfx[0], MIDI_SFX); // play choose sound
                    }
                    else if ((evt.button & PB_LEFT))
                    {
                        atr->selection--;
                        if (atr->selection < 0)
                        {
                            atr->selection = maxSelect;
                        }
                        globalMidiPlayerPlaySong(&atr->sfx[1], MIDI_SFX); // play move sound
                    }
                }
                else if ((evt.button & PB_RIGHT))
                {
                    atr->selection++;
                    if (atr->selection > maxSelect)
                    {
                        atr->selection = 0;
                    }
                    globalMidiPlayerPlaySong(&atr->sfx[1], MIDI_SFX); // play move sound
                }

                else if ((evt.button & PB_B))
                {
                    atr->state     = ATR_DISPLAY; // if B is pressed, go back to display view
                    atr->loadAnims = 0;
                    globalMidiPlayerPlaySong(&atr->sfx[0], MIDI_SFX); // play choose sound
                }
            }

            drawLobbies(&evt, elapsedUs);
            atrSetLeds(atr->sonaList[((atr->page) * SONA_PER + atr->selection)].team, elapsedUs);
            drawSonaSelector(evt, atr->selection);

            break;
        }
        case ATR_PROFILE:
        {
            viewProfile(&evt, elapsedUs);

            break;
        }
        case ATR_EDIT_PROFILE:
        {
            // Draw the panel as it is
            editProfile(&evt, elapsedUs);

            break;
        }
        default:
        {
            break;
        }
    }
}

// States
static void editProfile(buttonEvt_t* evt, uint64_t elapsedUs)
{
    if (atr->loadedProfs == false)
    {
        atr->loadedProfile = loadProfileFromNVS();
        generateSwadgesonaImage(&atr->loadedProfile.swsn, true);
        atr->loadedProfs = true;
    }

    drawCard(atr->loadedProfile, true, elapsedUs); // draw own profile
    while (checkButtonQueueWrapper(evt))
    {
        if (evt->down)
        {
            atr->drawSaved = false;

            if (evt->button & PB_B)
            {
                if (atr->created == 0)
                {
                    // prevent user from going back to title until they save
                }
                else
                {
                    atr->state = ATR_TITLE;
                    globalMidiPlayerPlaySong(&atr->sfx[0], MIDI_SFX); // play choose sound
                }
            }
            else if (evt->button & PB_A)
            {
                if (atr->yloc == 4) // save profile
                {
                    packProfileData(&atr->loadedProfile);
                    // save to NVS

                    writeNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, ATRIUM_PACKEDKEY,
                                        atr->loadedProfile.packedProfile);

                    // save to swadgepass data
                    atr->spProfile.packedProfile = atr->loadedProfile.packedProfile;
                    atr->spProfile.numPasses     = atr->loadedProfile.numPasses;
                    atr->spProfile.points        = atr->loadedProfile.points;
                    ESP_LOGI(ATR_TAG,
                             "swadgepass profile saved: packedProfile %" PRId32 ", numPasses %" PRId8
                             ", points %" PRId32 "",
                             atr->spProfile.packedProfile, atr->spProfile.numPasses, atr->spProfile.points);

                    ESP_LOGI(ATR_TAG, "latest points: %" PRId32 "", atr->spProfile.points);

                    atr->drawSaved = true;
                    atr->created   = 1;
                    globalMidiPlayerPlaySong(&atr->sfx[0], MIDI_SFX); // play choose sound
                }
            }
            else if (evt->button & PB_UP)
            {
                atr->yloc--;
                if (atr->yloc < 0)
                {
                    atr->yloc = 0;
                }
                globalMidiPlayerPlaySong(&atr->sfx[1], MIDI_SFX); // play move sound
            }
            else if (evt->button & PB_DOWN)
            {
                atr->yloc++;
                if (atr->yloc > 4)
                {
                    atr->yloc = 4;
                }
                globalMidiPlayerPlaySong(&atr->sfx[1], MIDI_SFX); // play move sound
            }
            else if (evt->button & PB_LEFT)
            {
                drawEditUI(evt, atr->yloc, 1);
                globalMidiPlayerPlaySong(&atr->sfx[1], MIDI_SFX); // play move sound
            }
            else if (evt->button & PB_RIGHT)
            {
                drawEditUI(evt, atr->yloc, 0);
                globalMidiPlayerPlaySong(&atr->sfx[1], MIDI_SFX); // play move sound
            }
        }
    }
    drawEditSelection(evt, atr->yloc);
}

static void viewProfile(buttonEvt_t* evt, uint64_t elapsedUs)
{
    ESP_LOGD(ATR_TAG, "Viewing profile %d on page %" PRId8, atr->selection, atr->page);
    ESP_LOGD(ATR_TAG, "sonas name is %s", atr->sonaList[atr->page * SONA_PER + atr->selection].swsn.name.nameBuffer);
    drawCard(atr->sonaList[atr->page * SONA_PER + atr->selection], false, elapsedUs); // draw selected profile

    while (checkButtonQueueWrapper(evt))
    {
        if (evt->down)
        {
            if (evt->button & PB_B)
            {
                atr->state     = ATR_DISPLAY;
                atr->loadAnims = 0;
            }
        }
    }
}

static void drawAtriumTitle(uint64_t elapsedUs)
{
    // draw the title screen for the atrium mode
    drawRectFilled(0, 0, TFT_WIDTH, TFT_HEIGHT, c555); // fill the screen with white
    atr->animTimer += elapsedUs;
    if (atr->animTimer >= ANIM_TIMER_MS && atr->loadAnims < 20)
    {
        atr->animTimer = 0;
        atr->loadAnims++;
    }

    // draw logo
    if (atr->loadAnims < 20)
    {
        int offset = 21 - atr->loadAnims;
        drawWsgSimple(&atr->uiElements[6], 0, offset);
    }
    else
    {
        drawWsgSimple(&atr->uiElements[6], 0, 0);
        drawWsgSimpleHalf(&atr->uiElements[2], 50, 160);
        drawWsgSimpleHalf(&atr->uiElements[4], 50, 180);
        drawText(&atr->fonts[0], c121, "Enter Atrium", 70, 162);
        drawText(&atr->fonts[0], c121, "Edit Profile", 70, 182);
    }
}

static void drawLobbies(buttonEvt_t* evt, uint64_t elapsedUs)
{
    // fake loading screen to tell them what to do
    if (atr->loadAnims < 20 * 8 && atr->fakeLoad == 0)
    {
        atr->animTimer += elapsedUs;
        if (atr->animTimer >= 8 * 8) // 8 loops
        {
            atr->animTimer = 0;
            atr->loadAnims++;
        }
        drawWsgSimple(&atr->uiElements[7], 0, 0);
        int frame = atr->loadAnims % 8;
        drawWsgSimpleHalf(&atr->uiElements[8 + frame], 180, 200);

        return;
    }
    else if (0 == atr->fakeLoad)
    {
        atr->fakeLoad  = 1; // skip this next time
        atr->loadAnims = 0;
    }

    // Draw
    switch (atr->lbState)
    {
        case BG_GAZEBO:
        {
            drawGazebo(elapsedUs);
            break;
        }
        case BG_ARCADE:
        {
            drawArcade(elapsedUs);

            break;
        }
        case BG_CONCERT:
        {
            drawConcert(elapsedUs);
            break;
        }
        default:
        {
            break;
        }
    }

    // Draw foregrounds
    switch (atr->lbState)
    {
        case BG_GAZEBO:
        {
            drawGazeboForeground(elapsedUs);
            break;
        }
        default:
        {
            break;
        }
    }

    // Sonas
    if (atr->numRemoteSwsn == 0)
    {
        drawRectFilled(30, TFT_HEIGHT / 2 - 50, TFT_WIDTH - 30, TFT_HEIGHT / 2 + 50, c555);
        drawText(&atr->fonts[0], c000, "No SwadgePass profiles found yet!", 40, TFT_HEIGHT / 2 - 45);
        drawText(&atr->fonts[0], c000, "Carry your Swadge during the fest", 39, TFT_HEIGHT / 2 + 15 - 45);
        drawText(&atr->fonts[0], c000, "and leave it turned ON to find 'em.", 38, TFT_HEIGHT / 2 + 30 - 45);
        drawText(&atr->fonts[0], c000, "Then, come back here to browse.", 39, TFT_HEIGHT / 2 + 45 - 45);
        drawText(&atr->fonts[0], c000, "Press B to Return to Title", 65, TFT_HEIGHT / 2 + 60 - 25);
    }
    else
    {
        // UI

        ESP_LOGI(ATR_TAG, "Selected arrow: %" PRId8, atr->selectedArrow);
        drawLobbyArrows(atr->selectedArrow);
        loadProfiles(SONA_PER, atr->page);
        drawSonas(atr->page, elapsedUs);
        drawRectFilled(8, 8, 68, 48, c555);
        drawWsgSimpleHalf(&atr->uiElements[2], 10, 10);
        drawWsgSimpleHalf(&atr->uiElements[4], 10, 30);
        drawText(&atr->fonts[0], c000, "Select", 30, 11);
        drawText(&atr->fonts[0], c000, "Back", 30, 31);
    }
}

// Draw
static void drawArcade(uint64_t elapsedUs)
{
    atr->animTimer += elapsedUs;
    if (atr->animTimer >= ANIM_TIMER_MS * 4 && atr->loadAnims < 20)
    {
        atr->animTimer = 0;
        atr->loadAnims++;
    }

    // Animations
    if (atr->loadAnims < 6)
    {
        drawWsgSimple(&atr->backgroundImages[3 + (atr->loadAnims % 6)], 0, 0);
        drawWsgSimple(&atr->backgroundImages[2], 0, TFT_HEIGHT - atr->backgroundImages[2].h);
    }
    else
    {
        atr->loadAnims = 0;
    }
}

static void drawConcert(uint64_t elapsedUs)
{
    atr->animTimer += elapsedUs;
    if (atr->animTimer >= ANIM_TIMER_MS * 5 && atr->loadAnims < 5 && atr->animDirection == false)
    {
        atr->animTimer = 0;
        atr->loadAnims++;
    }
    else if (atr->animTimer >= ANIM_TIMER_MS * 5 && atr->loadAnims > 0 && atr->animDirection == true)
    {
        atr->animTimer = 0;
        atr->loadAnims--;
    }
    else if (atr->animTimer >= ANIM_TIMER_MS * 100 && atr->loadAnims == 0)
    {
        atr->animDirection = !atr->animDirection;
        atr->animTimer     = 0;
        atr->loadAnims++;
    }
    else if (atr->animTimer >= ANIM_TIMER_MS * 5 && atr->loadAnims == 5)
    {
        atr->animDirection = !atr->animDirection;
        atr->animTimer     = 0;
        atr->loadAnims--;
    }

    // Animations

    drawWsgSimple(&atr->backgroundImages[10 + atr->loadAnims], 0, 0);
    drawWsgSimple(&atr->backgroundImages[9], 0, TFT_HEIGHT - atr->backgroundImages[9].h);
}

static void drawGazebo(uint64_t elapsedUs)
{
    // Draw base BG
    drawWsgSimple(&atr->backgroundImages[0], 0, 0);
}

static void drawGazeboForeground(uint64_t elapsedUs)
{
    atr->animTimer += elapsedUs;
    if (atr->animTimer >= ANIM_TIMER_MS && atr->loadAnims < atr->backgroundImages[1].h)
    {
        atr->animTimer = 0;
        atr->loadAnims++;
    }
    if (atr->loadAnims <= atr->backgroundImages[1].h)
    {
        int offset = TFT_HEIGHT - atr->loadAnims;
        drawWsgSimple(&atr->backgroundImages[1], 0, offset);
        drawWsg(&atr->backgroundImages[1], TFT_WIDTH - atr->backgroundImages[1].w, offset, true, false, 0);
    }
    else
    {
        drawWsgSimple(&atr->backgroundImages[1], 0, TFT_HEIGHT - atr->loadAnims);
        drawWsg(&atr->backgroundImages[1], TFT_WIDTH - atr->backgroundImages[1].w, TFT_HEIGHT - atr->loadAnims, true,
                false, 0);
    }
}

static void drawSonas(int8_t page, uint64_t elapsedUs)
{
    // move past the pages i don't want to draw
    if (atr->lastPage != page)
    {
        for (int i = 0; i < page * 4; i++)
        {
            if ((i == page * 4 - 1 && atr->loadedProfs == true))
            {
                atr->loadedProfs = false; // reset loaded profiles to load new ones
            }
            atr->bodyIdx[i] = rand() % ARRAY_SIZE(sonaBodies);
        }
        atr->loadedProfs = false; // reset loaded profiles to load new ones

        ESP_LOGI(ATR_TAG, "Page changed from %" PRId8 " to %" PRId8 ", resetting loadedProfs", atr->lastPage, page);
        atr->lastPage = page;
    }

    // Draw sonas
    ESP_LOGD(ATR_TAG, "Drawing sonas for page %" PRId8 " and the remainder is %" PRId8, page, atr->remSwsn);
    ESP_LOGD(ATR_TAG, "Total pages: %" PRId8, atr->totalPages);

    int sonas;

    if (page == atr->totalPages - 1 && atr->remSwsn != 0)
    {
        sonas = atr->remSwsn;
    }
    else
    {
        sonas = SONA_PER;
    }

    for (int i = 0; i < sonas; i++)
    {
        drawWsgSimple(&atr->sonaList[i + (page * 4)].swsn.image, 20 + (i * 60), 80); // draw the head
        char buf[10] = {0};
        snprintf(buf, sizeof(buf), "%d/%d", page + 1, atr->totalPages);
        drawRectFilled(TFT_WIDTH / 2 - 40, 202, TFT_WIDTH / 2 + 45, 218, c555); // page box
        drawText(&atr->fonts[0], c000, "Page", TFT_WIDTH / 2 - 36, 203);        // draw the "Page" text
        drawText(&atr->fonts[0], c000, buf, TFT_WIDTH / 2 + 10, 203);           // draw the page number

        switch (atr->lbState)
        {
            case BG_GAZEBO:
            {
                drawWsgSimple(&atr->bodies[atr->bodyIdx[i]], 20 + (i * 60),
                              128); // draw the body
                break;
            }
            case BG_ARCADE:
            {
                drawWsgSimple(&atr->bodies[0], 20 + (i * 60),
                              128); // draw the body
                break;
            }
            case BG_CONCERT:
            {
                drawConcertBodies(sonas, elapsedUs);
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

static void drawConcertBodies(int8_t sonas, uint64_t elapsedUs)
{
    atr->animBodyTimer += elapsedUs;
    if (atr->animBodyTimer >= ANIM_TIMER_MS * 20 && atr->loadBodyAnims < 20)
    {
        atr->animBodyTimer = 0;
        atr->loadBodyAnims++;
    }
    if (atr->loadBodyAnims >= 20)
    {
        atr->loadBodyAnims = 0;
    }

    if (atr->concertLoadedBodies == false)
    {
        for (int i = 0; i < sonas; i++)
        {
            atr->concertBodyIdx[i] = i;
        }

        atr->concertLoadedBodies = true;
    }

    for (int i = 0; i < sonas; i++)
    {
        switch (atr->concertBodyIdx[i])
        {
            case 0: // bass has 5 frames and starts at index 0 in wsg list  69X88

            {
                int frame = atr->loadBodyAnims % 5;

                drawWsgSimple(&atr->concertBodies[0 + frame], 20 + (i * 60) + 6, 110);
                break;
            }
            case 1: // guitar has 7 frames and starts at index 5 in wsg list  57X77

            {
                int frame = atr->loadBodyAnims % 7;
                if (atr->loadBodyAnims >= 7 && atr->loadBodyAnims < 10)
                {
                    frame = 6; // hold last frame for a sec
                    drawWsgSimple(&atr->concertBodies[5 + frame], 20 + (i * 60) + 7, 120);
                }
                if (atr->loadBodyAnims >= 10 && atr->loadBodyAnims < 17)
                {
                    drawWsgSimple(&atr->concertBodies[5 + 6 - frame], 20 + (i * 60) + 7, 120);
                }
                else
                {
                    drawWsgSimple(&atr->concertBodies[5 + frame], 20 + (i * 60) + 7, 120);
                }

                break;
            }
            case 2: // triangle has 10 frames and starts at index 13 in wsg list 75X78

            {
                int frame = atr->loadBodyAnims % 10;
                if (atr->loadBodyAnims >= 10)
                {
                    drawWsgSimple(&atr->concertBodies[13 + 9 - frame], 20 + (i * 60), 120);
                }
                else
                {
                    drawWsgSimple(&atr->concertBodies[13 + frame], 20 + (i * 60), 120);
                }
                break;
            }
            case 3: // trumpet has 6 frames and starts at index 23 in wsg list  94X82
            {
                int frame = atr->loadBodyAnims % 6;
                if (atr->loadBodyAnims < 6 || (atr->loadBodyAnims >= 12 && atr->loadBodyAnims < 17))
                {
                    drawWsgSimple(&atr->concertBodies[23 + frame], 20 + (i * 60) - 3, 116);
                }
                else if ((atr->loadBodyAnims >= 6 && atr->loadBodyAnims < 12))
                {
                    int reverseframe = 5 - frame;
                    drawWsgSimple(&atr->concertBodies[23 + reverseframe], 20 + (i * 60) - 3, 116);
                }
                else if (atr->loadBodyAnims >= 17 && atr->loadBodyAnims < 20)
                {
                    frame = 5 - atr->loadBodyAnims % 5;
                    drawWsgSimple(&atr->concertBodies[23 + frame], 20 + (i * 60) - 3, 116);
                }

                break;
            }
            default:
            {
                break;
            }
        }
    }
}

static void drawLobbyArrows(int selected)
{
    // Left, down, up, right
    const int16_t angles[] = {180, 90, 270, 0};
    bool arrowEnabled[4]   = {true, true, true, true};
    int16_t xloc[]         = {5, TFT_WIDTH / 2 - atr->uiElements[0].w / 2, TFT_WIDTH / 2 - atr->uiElements[0].w / 2,
                              TFT_WIDTH - 5 - atr->uiElements[0].w};
    int16_t yloc[]         = {LOBBY_ARROW_Y, 220, 20, LOBBY_ARROW_Y};

    if (ATR_DISPLAY == atr->state)
    {
        if (atr->lbState == 0)
        {
            // Disable up
            arrowEnabled[2] = false;
        }
        if (atr->lbState == BG_COUNT - 1)
        {
            // Disable down
            arrowEnabled[1] = false;
        }
    }
    else if (ATR_SELECT == atr->state)
    {
        // Disable up and down
        arrowEnabled[1] = false;
        arrowEnabled[2] = false;
    }

    if (atr->page == 0)
    {
        // Disable left
        arrowEnabled[0] = false;
    }
    else if (atr->page == atr->totalPages - 1)
    {
        // Disable right
        arrowEnabled[3] = false;
    }
    selected--;
    ESP_LOGI(ATR_TAG, "Drawing arrows with selected %d", selected);
    for (int16_t aIdx = 0; aIdx < ARRAY_SIZE(angles); aIdx++)
    {
        (selected == aIdx)
            ? (arrowEnabled[aIdx] ? drawWsg(&atr->uiElements[1], xloc[aIdx], yloc[aIdx], 0, 0, angles[aIdx]) : 0)
            : (arrowEnabled[aIdx] ? drawWsg(&atr->uiElements[0], xloc[aIdx], yloc[aIdx], 0, 0, angles[aIdx]) : 0);
    }
}

static void drawCard(userProfile_t profile, bool local, uint64_t elapsedUs)
{
    atrSetLeds(profile.team, elapsedUs);
    ESP_LOGD(ATR_TAG, "Drawing card with cardSelect %" PRId8 ", fact0 %" PRId8 ", fact1 %" PRId8 ", fact2 %" PRId8,
             profile.cardSelect, profile.fact0, profile.fact1, profile.fact2);
    // concat card info
    // line 1: fact0
    char factline0[64];
    snprintf(factline0, sizeof(factline0) - 1, "%s%s", preambles[0], fact0[profile.fact0]);
    // line 2: fact1
    char factline1[64];
    snprintf(factline1, sizeof(factline1) - 1, "%s%s", preambles[1], fact1[profile.fact1]);
    // line 3: fact2
    char factline2[64];
    snprintf(factline2, sizeof(factline2) - 1, "%s%s", preambles[2], fact2[profile.fact2]);

    drawWsgSimple(&atr->backgroundImages[0], 0, 0);
    drawWsgSimple(&atr->cards[profile.cardSelect], 7, 7 + 12); // draw the card
    generateSwadgesonaImage(&profile.swsn, true);
    drawWsgSimple(&profile.swsn.image, SONALOC_X, SONALOC_Y); // draw the sona image

    nameData_t username;

    if (local == true)
    {
        username = *getSystemUsername();
    }
    else
    {
        username = profile.swsn.name;
    }

    drawText(&atr->fonts[0], c000, username.nameBuffer, 99 + CARDTEXTPAD,
             SONALOC_Y + CARDTEXTPAD); // draw the name
    drawText(&atr->fonts[0], c000, factline0, 99 + CARDTEXTPAD,
             SONALOC_Y + CARDTEXTPAD + 13 + CARDTEXTPAD); // draw the sandwich info
    drawText(&atr->fonts[0], c000, factline1, 99 + CARDTEXTPAD,
             SONALOC_Y + CARDTEXTPAD + 26 + CARDTEXTPAD); // draw the identity
    drawText(&atr->fonts[0], c000, factline2, 99 + CARDTEXTPAD,
             SONALOC_Y + CARDTEXTPAD + 39 + CARDTEXTPAD); // draw the identity info

    drawText(&atr->fonts[0], c000, "Swadge Points:", 24 + CARDTEXTPAD, 126);
    char buf[sizeof(profile.points) + 1];
    snprintf(buf, sizeof(buf), "%" PRId32, profile.points);
    drawText(&atr->fonts[0], c000, buf, 120 + CARDTEXTPAD, 126);                  // draw points
    drawText(&atr->fonts[0], c000, "Swadgepasses Found:", 24 + CARDTEXTPAD, 150); // draw points
    char buf1[5];
    snprintf(buf1, sizeof(buf1), "%" PRId8, profile.numPasses);
    drawText(&atr->fonts[0], c000, buf1, 155 + CARDTEXTPAD, 150); // draw numpasses

    drawWsgSimple(&atr->teamElements[profile.team], 208, 124); // draw team image
}

void drawEditSelection(buttonEvt_t* evt, int yloc)
{
    const int16_t text_xloc = 130;
    const int16_t text_yloc = 205;
    bool arrowEnabled[4]    = {true, true, true, true};
    bool drawSave           = false;

    drawRectFilled(text_xloc - 2, text_yloc - 3, TFT_WIDTH / 2 + 47 + 36, text_yloc + 16, c555);
    switch (yloc)
    {
        case 0:
        {
            drawWsgSimple(&atr->uiElements[18], 2, 14); // card select
            drawText(&atr->fonts[0], c000, editPromptText[0], text_xloc + 10, text_yloc);
            arrowEnabled[2] = false;
            break;
        }
        case 1:
        {
            drawWsg(&atr->uiElements[17], 90 - CARDTEXTPAD, 55, false, false, 270); // fact0
            drawText(&atr->fonts[0], c000, editPromptText[1], text_xloc + 7, text_yloc);
            break;
        }
        case 2:
        {
            drawWsg(&atr->uiElements[17], 90 - CARDTEXTPAD, 68, false, false, 270); // fact1
            drawText(&atr->fonts[0], c000, editPromptText[2], text_xloc + 1, text_yloc);
            break;
        }
        case 3:
        {
            drawWsg(&atr->uiElements[17], 90 - CARDTEXTPAD, 81, false, false, 270); // fact2
            drawText(&atr->fonts[0], c000, editPromptText[3], text_xloc - 1, text_yloc);
            break;
        }
        case 4:
        {
            arrowEnabled[0] = false;
            arrowEnabled[1] = false;
            arrowEnabled[3] = false;
            drawSave        = true;
            drawText(&atr->fonts[0], c000, editPromptText[4], text_xloc + 13, text_yloc);

            if (atr->drawSaved)
            {
                drawText(&atr->fonts[0], c000, editPromptText[5], text_xloc - 60, text_yloc + atr->fonts[0].height + 2);
            }
            break;
        }
        default:
        {
            break;
        }
    }

    const int16_t angles[] = {180, 90, 270, 0};
    const int16_t offset   = 35;
    const int16_t xloc[]
        = {75 + offset, (TFT_WIDTH - atr->uiElements[0].w) / 2 + offset,
           (TFT_WIDTH - atr->uiElements[0].w) / 2 + offset, TFT_WIDTH - atr->uiElements[0].w - 75 + offset};
    const int16_t arr_yloc[] = {text_yloc - 2, text_yloc + 18, text_yloc - 20, text_yloc - 2};
    if (yloc != 4)
    {
        drawWsgSimple(&atr->uiElements[20], xloc[0] - atr->uiElements[19].w, arr_yloc[0]);
    }

    if (drawSave)
    {
        drawWsgSimple(&atr->uiElements[19], xloc[0] - atr->uiElements[19].w, arr_yloc[0]);
    }

    for (int16_t aIdx = 0; aIdx < ARRAY_SIZE(angles); aIdx++)
    {
        arrowEnabled[aIdx] == true
            ? drawWsg(&atr->uiElements[0], xloc[aIdx], arr_yloc[aIdx], false, false, angles[aIdx])
            : 0;
    }
}

void drawEditUI(buttonEvt_t* evt, int yloc, bool direction)
{
    if (direction == 0)
    {
        switch (yloc)
        {
            case 0:
            {
                atr->loadedProfile.cardSelect++;
                if (atr->loadedProfile.cardSelect > 12)
                {
                    atr->loadedProfile.cardSelect = 0;
                }
                break;
            }
            case 1:
            {
                atr->loadedProfile.fact0++;
                if (atr->loadedProfile.fact0 > 7)
                {
                    atr->loadedProfile.fact0 = 0;
                }
                break;
            }
            case 2:
            {
                atr->loadedProfile.fact1++;
                if (atr->loadedProfile.fact1 > 7)
                {
                    atr->loadedProfile.fact1 = 0;
                }
                break;
            }
            case 3:
            {
                atr->loadedProfile.fact2++;
                if (atr->loadedProfile.fact2 > 7)
                {
                    atr->loadedProfile.fact2 = 0;
                }
                break;
            }
            case 4:
            default:
            {
                break;
            }
        }
    }
    else
    {
        switch (yloc)
        {
            case 0:
            {
                atr->loadedProfile.cardSelect--;
                if (atr->loadedProfile.cardSelect < 0)
                {
                    atr->loadedProfile.cardSelect = 12;
                }
                break;
            }
            case 1:
            {
                atr->loadedProfile.fact0--;
                if (atr->loadedProfile.fact0 < 0)
                {
                    atr->loadedProfile.fact0 = 7;
                }
                break;
            }
            case 2:
            {
                atr->loadedProfile.fact1--;
                if (atr->loadedProfile.fact1 < 0)
                {
                    atr->loadedProfile.fact1 = 7;
                }
                break;
            }
            case 3:
            {
                atr->loadedProfile.fact2--;
                if (atr->loadedProfile.fact2 < 0)
                {
                    atr->loadedProfile.fact2 = 7;
                }
                break;
            }
            case 4:
            default:
            {
                break;
            }
        }
    }
}

void drawSonaSelector(buttonEvt_t evt, int selection)
{
    drawWsg(&atr->uiElements[17], 20 + (selection * 60) + 24, 66, false, false, 0); // draw selector over sona
}

// utilities

void shuffleSonas(void)
{
    if (atr->shuffle == 0)
    {
        if (atr->numRemoteSwsn <= 0)
        {
            return;
        }
        else
        {
            int j;
            for (int i = atr->numRemoteSwsn - 1; i > 0; i--)
            {
                j = rand() % (i + 1);
                // Swap
                userProfile_t temp = atr->sonaList[i];
                atr->sonaList[i]   = atr->sonaList[j];
                atr->sonaList[j]   = temp;
            }
        }
    }
    atr->shuffle = 1; // only shuffle once
}

void loadProfiles(int maxProfiles, int page)
{
    if (atr->loadedProfs == true)
    {
        return; // already loaded
    }

    else
    {
        ESP_LOGI(ATR_TAG, "Loading profiles: maxProfiles=%d, page=%d", maxProfiles, page);
        if (atr->lastPage == page)
        {
        }
        else
        {
            atr->loadedBodies = false;
        }

        for (int i = 0; i < maxProfiles; i++)
        {
            unpackProfileData(&atr->sonaList[page * 4 + i]);
            generateSwadgesonaImage(&atr->sonaList[page * 4 + i].swsn, false);
            ESP_LOGI(ATR_TAG, "Loaded profile %d for page %d", page * 4 + i, page);
            if (atr->loadedBodies == false)
            {
                atr->bodyIdx[i] = rand() % ARRAY_SIZE(sonaBodies);
            }
        }
        atr->loadedProfs  = true; // mark as loaded
        atr->loadedBodies = true; // don't randomize bodies again
    }
}

userProfile_t loadProfileFromNVS(void)
{
    userProfile_t loadedProfile = {0};
    int8_t team                 = 0;
    int32_t teamchecker         = 0;

    if (!readNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, ATRIUM_CREATEDKEY,
                            &atr->created)) // check if profile created

    {
        ESP_LOGI(ATR_TAG, "No existing profile found, creating new profile");
        loadedProfile.cardSelect = esp_random() % 12;
        loadedProfile.fact0      = esp_random() % 8;
        loadedProfile.fact1      = esp_random() % 8;
        loadedProfile.fact2      = esp_random() % 8;
        loadedProfile.numPasses  = 0;
        loadedProfile.points     = 0;

        team = esp_random() % 3; // assign random team 0,1,2
        ESP_LOGI(ATR_TAG, "team rng is %" PRId8, team);
        loadSPSona(&loadedProfile.swsn.core); // load the sona image from swadgepass data
        loadedProfile.team = team;
        ESP_LOGI(ATR_TAG, "assigned team is %" PRId8, loadedProfile.team);

        packProfileData(&loadedProfile);
        writeNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, ATRIUM_PACKEDKEY, loadedProfile.packedProfile);
        writeNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, ATRIUM_CREATEDKEY, 1); // mark profile as created
        writeNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, TEAMKEY, loadedProfile.team);
        ESP_LOGI(ATR_TAG, "New profile created with packedProfile=%" PRId32 "\n", loadedProfile.packedProfile);
        trophyUpdate(&atriumTrophyData.list[1 + team], 1, true); // award trophy for creating profile
        return loadedProfile;
    }

    readNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, ATRIUM_PACKEDKEY, &loadedProfile.packedProfile);
    readNamespaceNvs32(TROPHY_NVS_NAMESPACE, TROPHY_POINTS_NVS_KEY, &loadedProfile.points);

    readNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, TEAMKEY, &teamchecker);
    unpackProfileData(&loadedProfile);
    ESP_LOGI(ATR_TAG, "Team from NVS is %" PRId32 " and team from packedProfile is %" PRId8, teamchecker,
             loadedProfile.team);
    if (teamchecker != loadedProfile.team)
    {
        ESP_LOGW(ATR_TAG, "Team mismatch! Using team from NVS");
        loadedProfile.team = teamchecker;
        ESP_LOGI(ATR_TAG, "Updated team to %" PRId8, loadedProfile.team);
    }
    ESP_LOGI(ATR_TAG, "Loaded local swadgesona data");
    loadedProfile.numPasses = atr->numRemoteSwsn; // update number of passes each time

    loadSPSona(&loadedProfile.swsn.core); // load the sona image from swadgepass data

    ESP_LOGI(ATR_TAG, "Generating swadgesona image");

    generateSwadgesonaImage(&loadedProfile.swsn, true);

    ESP_LOGI(ATR_TAG,
             "Profile loaded from NVS: packedProfile=%" PRId32 ", numPasses=%" PRId8 ", points=%" PRId32
             ", team=%" PRId8,
             loadedProfile.packedProfile, loadedProfile.numPasses, loadedProfile.points, loadedProfile.team);

    atr->loadedProfile = loadedProfile; // update loaded profile

    return atr->loadedProfile;
}

void updateTeamScores(void)
{
    if (atr->loadedTeams == false)
    {
        int32_t myteam;
        int32_t myPoints;
        readNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, TEAMKEY, &myteam);
        readNamespaceNvs32(TROPHY_NVS_NAMESPACE, TROPHY_POINTS_NVS_KEY, &myPoints);

        ESP_LOGI(ATR_TAG, "Loaded profile with team %" PRId32, myteam);

        int redscore    = 0;
        int bluescore   = 0;
        int yellowscore = 0;
        for (int idx = 0; idx < atr->numRemoteSwsn; idx++)
        {
            unpackProfileData(&atr->sonaList[idx]);
            switch (atr->sonaList[idx].team)
            {
                case 0:
                    redscore += atr->sonaList[idx].points;
                    break;
                case 1:
                    bluescore += atr->sonaList[idx].points;
                    break;
                case 2:
                    yellowscore += atr->sonaList[idx].points;
                    break;
                default:
                    break;
            }
        }

        int myteamscore = 0;
        switch (myteam)
        {
            case 0:
            {
                myteamscore = redscore + myPoints;
                ESP_LOGI(ATR_TAG, "My team score for red team is %d", myteamscore);
                break;
            }
            case 1:
            {
                myteamscore = bluescore + myPoints;
                ESP_LOGI(ATR_TAG, "My team score for blue team is %d", myteamscore);
                break;
            }
            case 2:
            {
                myteamscore = yellowscore + myPoints;
                ESP_LOGI(ATR_TAG, "My team score for yellow team is %d", myteamscore);
                break;
            }
            default:
                break;
        }
        trophyUpdate(&atriumTrophyData.list[7], redscore, false);    // update trophy for red team
        trophyUpdate(&atriumTrophyData.list[8], bluescore, false);   // update trophy for blue team
        trophyUpdate(&atriumTrophyData.list[9], yellowscore, false); // update trophy for yellow team

        trophyUpdate(&atriumTrophyData.list[7 + myteam], myteamscore, true); // update and draw the player's team only
        ESP_LOGI(ATR_TAG, "Team scores updated: Red=%d, Blue=%d, Yellow=%d, MyTeam=%" PRId32 " Score=%d\n", redscore,
                 bluescore, yellowscore, myteam, myteamscore);
    }
    atr->loadedTeams = true;
}

static void atriumAddSP(struct swadgePassPacket* packet)
{
    // Add our profile data to the swadge pass packet
    int32_t packedProfile;
    int32_t points;

    readNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, ATRIUM_PACKEDKEY, &packedProfile);
    readNamespaceNvs32(TROPHY_NVS_NAMESPACE, TROPHY_POINTS_NVS_KEY, &points);
    packet->atrium.packedProfile = packedProfile;
    packet->atrium.points        = points;
}

void packProfileData(userProfile_t* profile)
{
    ESP_LOGI(ATR_TAG,
             "Packing profile data: cardSelect %" PRId8 ", fact0 %" PRId8 ", fact1 %" PRId8 ", fact2 %" PRId8
             ", numPasses %" PRId8 ", team %" PRId8 "",
             profile->cardSelect, profile->fact0, profile->fact1, profile->fact2, profile->numPasses, profile->team);
    profile->packedProfile = atr->loadedProfile.cardSelect;
    profile->packedProfile += atr->loadedProfile.fact0 << 4;
    profile->packedProfile += atr->loadedProfile.fact1 << 8;
    profile->packedProfile += atr->loadedProfile.fact2 << 12;
    profile->packedProfile += atr->loadedProfile.team << 16;
    profile->packedProfile += atr->loadedProfile.numPasses << 20;
}

void unpackProfileData(userProfile_t* profile)
{
    /* clang-format off */
    profile->cardSelect = profile->packedProfile  &0b00000000000000000000000000001111;
    profile->fact0 = (profile->packedProfile      &0b00000000000000000000000011110000) >>4;
    profile->fact1 = (profile->packedProfile      &0b00000000000000000000111100000000) >>8;
    profile->fact2 = (profile->packedProfile      &0b00000000000000001111000000000000) >>12;
    profile->team = (profile->packedProfile       &0b00000000000011110000000000000000) >>16;
    profile->numPasses = (profile->packedProfile  &0b00001111111100000000000000000000) >>20;
    ESP_LOGI(ATR_TAG, "unpacked profile is cardselect %" PRId8 ", fact0 %" PRId8 ", fact1 %" PRId8 ", fact2 %" PRId8 ", numPasses %" PRId8 ", team %" PRId8 "",
           profile->cardSelect, profile->fact0, profile->fact1,
           profile->fact2, profile->numPasses, profile->team);
    /* clang-format on */
}

static void atrSetLeds(int team, uint64_t elapsedUs)
{
    switch (team)
    {
        case 0: // red
            for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
            {
                atr->leds[i].r = 255;
                atr->leds[i].g = 0;
                atr->leds[i].b = 0;
            }
            break;
        case 1: // blue
            for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
            {
                atr->leds[i].r = 0;
                atr->leds[i].g = 0;
                atr->leds[i].b = 255;
            }
            break;
        case 2: // yellow
            for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
            {
                atr->leds[i].r = 255;
                atr->leds[i].g = 255;
                atr->leds[i].b = 0;
            }
            break;
        case 3: // green
            for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
            {
                atr->leds[i].r = 0;
                atr->leds[i].g = 255;
                atr->leds[i].b = 0;
            }
            break;
        default: // off
            for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
            {
                atr->leds[i].r = 0;
                atr->leds[i].g = 0;
                atr->leds[i].b = 0;
            }
            break;
    }

    setLeds(atr->leds, CONFIG_NUM_LEDS);
}

static void atrClearLeds(void)
{
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        atr->leds[i].r = 0;
        atr->leds[i].g = 0;
        atr->leds[i].b = 0;
    }
    setLeds(atr->leds, CONFIG_NUM_LEDS);
}