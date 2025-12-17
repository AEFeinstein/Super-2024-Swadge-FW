#include "atrium.h"
#include "nameList.h"
#include "swadgesona.h"
#include "swadgePass.h"
#include "trophy.h"
#include "modeIncludeList.h"

//---------------------------------------------------------------------------------//
// DEFINES
//---------------------------------------------------------------------------------//

#define TFT_WIDTH             280
#define TFT_HEIGHT            240
#define MAX_NUM_SWADGE_PASSES 50

// Lobby
#define SONA_PER            4
#define MAX_SWADGESONA_IDXS (MAX_NUM_SWADGE_PASSES / SONA_PER) - (MAX_NUM_SWADGE_PASSES % SONA_PER) / SONA_PER
#define ANIM_TIMER_MS       16667
#define LOBBY_ARROW_Y       200
#define LOBBY_BORDER_X      20

// Profile
#define CARDTEXTPAD 4
/* #define TEXTBOX1WIDTH 156
#define TEXTBOX2WIDTH 172 */
#define SONALOC_X 24
#define SONALOC_Y 36

#define ATRIUM_PROFILE_NVS_NAMESPACE "atrium"
// #define PROFKEY                      "atrprof"
// #define SPSONA_NVS_KEY               "spSona"
#define TROPHY_NVS_NAMESPACE "trophy"
// #define TROPHY_LATEST_NVS_KEY "latest"
#define TROPHY_POINTS_NVS_KEY "points"
#define ATRIUM_PACKEDKEY      "packedProfile"
// #define ATRIUM_NUMPASSESKEY   "numPasses"

//---------------------------------------------------------------------------------//
// CONSTS
//---------------------------------------------------------------------------------//

// CNFS index lists

// Sona bodies
static const cnfsFileIdx_t sonaBodies[] = {
    BODY_1_WSG, BODY_2_WSG, DAISY_WSG, FALLOUT_WSG, HANDSOME_WSG, KIRBY_WSG, LINK_WSG,   MARIO_WSG, PACMAN_WSG,
    PEACH_WSG,  RAYMAN_WSG, SANIC_WSG, SORA_WSG,    STAFF_WSG,    STEVE_WSG, STEVEN_WSG, WALDO_WSG, ZELDA_WSG,
};

static const cnfsFileIdx_t uiImages[] = {
    ARROWBUTTON_1_WSG, ARROWBUTTON_2_WSG, ABUTTON_1_WSG,   ABUTTON_2_WSG, BBUTTON_1_WSG,  BBUTTON_2_WSG, ATRIUMLOGO_WSG,
    KEEPON_WSG,        LOADING_1_WSG,     LOADING_2_WSG,   LOADING_3_WSG, LOADING_4_WSG,  LOADING_5_WSG, LOADING_6_WSG,
    LOADING_7_WSG,     LOADING_8_WSG,     GOLD_TROPHY_WSG, ARROW_WSG,     CARDSELECT_WSG,
};

static const cnfsFileIdx_t bgImages[] = {
    GAZEBO_WSG, ATRIUMPLANT_1_WSG, ARCADE_1_WSG, ARCADE_2_WSG, ARCADE_3_WSG, ARCADE_4_WSG, CONCERT_1_WSG, CONCERT_2_WSG,
};

// Images used for the backgrounds
static const cnfsFileIdx_t cardImages[] = {
    CARDSTAFF_WSG, CARDGEN_WSG,    CARDBLOSS_WSG, CARDBUBB_WSG,   CARDDINO_WSG, CARDMAGFEST_WSG, CARDMUSIC_WSG,
    CARDSPACE_WSG, CARDARCADE_WSG, CARDSTARS_WSG, CARDSUNSET_WSG, CARDMIVS_WSG, CARDLEOPARD_WSG,
};

static const cnfsFileIdx_t midiBGM[] = {
    ATRTHEME1_MID,
    ATRTHEME2_MID,
    ATRVIBE_MID,
};

static const cnfsFileIdx_t midiSFX[] = {
    SWSN_CHOOSE_SFX_MID,
    SWSN_MOVE_SFX_MID,
};

static const cnfsFileIdx_t fontsIdxs[] = {
    OXANIUM_13MED_FONT,
};

const char atriumModeName[] = "Atrium";

const char ATR_TAG[] = "ATR";

// Strings
/*
 static const char* const editButtonText[] = {
    "CANCEL",
    "SAVE",
};

static const char* const editConfirmText[] = {
    "Are you sure?",
    "This will overwrite your profile.",
};
static const char* const editInstructText[] = {
    "Press arrows to scroll",
    "Press A to confirm selection",
}; */

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
    "Fave Sandwich: ",
    "I am a: ",
    "Fave place: ",
};

static const char* const editPromptText[] = {
    "Choose Card", "Choose Identity", "Choose Location", "Pick Sandwich", "Save Profile",
};

// // Coordinates
// static const int textboxcoords_x[]  = {99, 24};             // x coords for the two textboxes
// static const int textbox1coords_y[] = {39, 51, 63, 75};     // y coords for the first textbox lines
// static const int textbox2coords_y[] = {124, 136, 148, 160}; // y coords for the second textbox lines
// static const int buttoncoord_x[]    = {99, 24};             // x coord for the button columns

// static const char* const buttontext[] = {"CANCEL", "SAVE"};
// static const char* const prompttext[] = {"Choose Card", "Edit Sona", "Pick Sandwich", "Choose Identity", "Choose
// Location"};
// static const char* const confirmtext[] = {"Are you sure?", "This will overwrite your profile."};
// static const char* const instructtext[] = {"Press arrows to scroll", "Press A to confirm selection"};

// Trophy Case
const trophyData_t atriumTrophies[] = {
    {
        .title       = "Welcome to the Atrium",
        .description = "We've got music and games",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .hidden      = true,
        .identifier  = NULL,
    },
};

const trophySettings_t atriumTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 4,
    .slideDurationUs  = DRAW_SLIDE_US,
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

/* typedef enum
{
    EDIT_CARD,
    EDIT_SONA,
    EDIT_TEXT0,
    EDIT_TEXT1,
    EDIT_SYMBOL,
    EDIT_CANCEL,
    EDIT_SAVE
} editSelect_t;

typedef enum
{
    LINE0,
    LINE1,
    LINE2,
    LINE3
} lineSelect_t; */

/* typedef enum
{
    NOTHING, // Initial state
    SONA0,
    SONA1,
    SONA2,
    SONA3,
    PAGE_FWD,
    PAGE_BACK,
    RETURN_TITLE,
} sonaSelect; */

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
    int32_t packedProfile; // card select 0-3, fact0 4-7, fact1 8-11, fact2 12-15, numpasses 16-23

    swadgesona_t swsn; // Swadgesona data
    int32_t points;
} userProfile_t;

typedef struct
{
    // Data
    wsg_t* bodies;
    wsg_t* backgroundImages;
    wsg_t* uiElements;
    wsg_t* cards;
    font_t* fonts;
    midiFile_t* bgm;
    midiFile_t* sfx;

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
    bool left, right, up, down;
    int64_t animTimer;
    int loadAnims;
    bool fakeLoad;
    bool shuffle;
    bool loadedProfs;
    bool drawnProfs;
    int8_t page;
    int8_t lastPage;
    int8_t remSwsn;
    int8_t totalPages;
    int8_t bodyIdx[SONA_PER];

    // BGM
    midiPlayer_t* player;

    // SwadgePass List
    list_t spList;

    // Profile viewer/editor
    int selection;
    int xloc;
    int yloc;

    int32_t points;

    // SwadgePass Profile
    userProfile_t spProfile;

} atrium_t;

//---------------------------------------------------------------------------------//
// FUNCTION DECLARATIONS
//---------------------------------------------------------------------------------//

// Main
static void atriumEnterMode(void);
static void atriumExitMode(void);
static void atriumMainLoop(int64_t elapsedUs);

// Editors
static void editProfile(buttonEvt_t* evt);
static void viewProfile(buttonEvt_t* evt);

// Draw
static void drawAtriumTitle(uint64_t elapsedUs);
static void drawLobbies(buttonEvt_t* evt, uint64_t elapsedUs);
void shuffleSonas(void);
static void drawSonas(int8_t page, uint64_t elapsedUs);
static void drawArcade(uint64_t elapsedUs);
static void drawConcert(uint64_t elapsedUs);
static void drawGazebo(uint64_t elapsedUs);
static void drawGazeboForeground(uint64_t elapsedUs);
static void drawArrows(bool, bool, bool, bool);
static void drawCard(userProfile_t profile, bool local);
void drawEditSelection(buttonEvt_t* evt, int yloc);
void drawEditUI(buttonEvt_t* evt, int yloc, bool direction);
void drawSonaSelector(buttonEvt_t evt, int selection);

// Swadgepass
static void atriumAddSP(struct swadgePassPacket* packet);
void loadProfiles(int maxProfiles, int page);
userProfile_t loadProfileFromNVS(void);
void packProfileData(userProfile_t* profile);
void unpackProfileData(userProfile_t* profile);

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
    .trophyData               = NULL,            // TODO enable &atriumTrophyData,
    .fnAddToSwadgePassPacket  = atriumAddSP,     // function to add data to swadgepass packet
};

atrium_t* atr;

//---------------------------------------------------------------------------------//
// FUNCTIONS
//---------------------------------------------------------------------------------//

static void atriumEnterMode(void)
{
    // Initialize memory
    atr = (atrium_t*)heap_caps_calloc(1, sizeof(atrium_t), MALLOC_CAP_8BIT);

    atr->bodies = heap_caps_calloc(ARRAY_SIZE(sonaBodies), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(sonaBodies); idx++)
    {
        loadWsg(sonaBodies[idx], &atr->bodies[idx], true);
    }
    atr->uiElements = heap_caps_calloc(ARRAY_SIZE(uiImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(uiImages); idx++)
    {
        loadWsg(uiImages[idx], &atr->uiElements[idx], true);
    }
    atr->backgroundImages = heap_caps_calloc(ARRAY_SIZE(bgImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bgImages); idx++)
    {
        loadWsg(bgImages[idx], &atr->backgroundImages[idx], true);
    }
    atr->cards = heap_caps_calloc(ARRAY_SIZE(cardImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(cardImages); idx++)
    {
        loadWsg(cardImages[idx], &atr->cards[idx], true);
    }
    atr->bgm = heap_caps_calloc(ARRAY_SIZE(midiBGM), sizeof(midiFile_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(midiBGM); idx++)
    {
        loadMidiFile(midiBGM[idx], &atr->bgm[idx], true);
    }
    atr->sfx = heap_caps_calloc(ARRAY_SIZE(midiSFX), sizeof(midiFile_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(midiSFX); idx++)
    {
        loadMidiFile(midiSFX[idx], &atr->sfx[idx], true);
    }
    atr->fonts = heap_caps_calloc(ARRAY_SIZE(fontsIdxs), sizeof(font_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(fontsIdxs); idx++)
    {
        loadFont(fontsIdxs[idx], &atr->fonts[idx], true);
    }

    // Swadgepass
    getSwadgePasses(&atr->spList, &atriumMode, true);
    node_t* spNode = atr->spList.first;
    int i          = 0;
    atr->loadAnims = 0;

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
    atr->page          = 0;
    atr->totalPages    = (atr->numRemoteSwsn / SONA_PER) + ((atr->numRemoteSwsn % SONA_PER) ? 1 : 0);
    atr->remSwsn       = atr->numRemoteSwsn % SONA_PER;

    ESP_LOGI(ATR_TAG, "Num remote swsn: %d, total pages: %d", atr->numRemoteSwsn, atr->totalPages);

    // BGM
    atr->player       = globalMidiPlayerGet(MIDI_BGM);
    atr->player->loop = true;
    midiGmOn(atr->player);
    globalMidiPlayerPlaySong(&atr->bgm[1], MIDI_BGM);
    globalMidiPlayerSetVolume(MIDI_BGM, 13);

    // profile created yet?
    if (!readNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, ATRIUM_PACKEDKEY, &atr->loadedProfile.packedProfile))
    {
        atr->state = ATR_EDIT_PROFILE; // go to profile edit if no profile yet
    }
    else
    {
        atr->state = ATR_TITLE; // else go to title screen
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
    heap_caps_free(atr->fonts);
    for (int idx = 0; idx < ARRAY_SIZE(midiSFX); idx++)
    {
        unloadMidiFile(&atr->sfx[idx]);
    }
    heap_caps_free(atr->sfx);
    for (int idx = 0; idx < ARRAY_SIZE(midiBGM); idx++)
    {
        unloadMidiFile(&atr->bgm[idx]);
    }
    heap_caps_free(atr->bgm);
    for (int idx = 0; idx < ARRAY_SIZE(cardImages); idx++)
    {
        freeWsg(&atr->cards[idx]);
    }
    heap_caps_free(atr->cards);
    for (int idx = 0; idx < ARRAY_SIZE(bgImages); idx++)
    {
        freeWsg(&atr->backgroundImages[idx]);
    }
    heap_caps_free(atr->backgroundImages);
    for (int idx = 0; idx < ARRAY_SIZE(uiImages); idx++)
    {
        freeWsg(&atr->uiElements[idx]);
    }
    heap_caps_free(atr->uiElements);
    for (int idx = 0; idx < ARRAY_SIZE(sonaBodies); idx++)
    {
        freeWsg(&atr->bodies[idx]);
    }
    heap_caps_free(atr->bodies);
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
                        atr->state = ATR_DISPLAY;
                    }
                    else if ((evt.button & PB_B))
                    {
                        atr->state = ATR_EDIT_PROFILE; // if B is pressed, go to edit profile view
                    }
                }
            }
            drawAtriumTitle(elapsedUs);
            atr->loadedProfs = false;
            shuffleSonas();

            break;
        }
        case ATR_DISPLAY:
        {
            // Handle input
            while (checkButtonQueueWrapper(&evt))
            {
                atr->lastPage = atr->page;

                if (evt.down)
                {
                    if (evt.button & PB_UP)
                    {
                        atr->up = true;
                    }
                    else if (evt.button & PB_LEFT)
                    {
                        atr->left = true;
                        atr->page--;
                        if (atr->page < 0)
                        {
                            atr->page = 0;
                        }
                    }
                    else if (evt.button & PB_RIGHT)
                    {
                        atr->right = true;
                        atr->page++;
                        if (atr->page > (atr->numRemoteSwsn - 1) / 4)
                        {
                            atr->page = (atr->numRemoteSwsn - 1) / 4;
                        }
                    }
                    else if (evt.button & PB_DOWN)
                    {
                        atr->down = true;
                    }
                    if (evt.button & PB_UP)
                    {
                        atr->up = false;
                        if (atr->lbState > 0)
                        {
                            atr->lbState--;
                        }
                        atr->loadAnims = 0;
                    }
                    else if (evt.button & PB_DOWN)
                    {
                        atr->down = false;
                        if (atr->lbState < 2)
                        {
                            atr->lbState++;
                        }

                        atr->loadAnims = 0;
                    }
                    else if (evt.button & PB_A)
                    {
                        atr->state     = ATR_SELECT;
                        atr->selection = 0;
                    }
                    else if (evt.button & PB_B)
                    {
                        atr->state = ATR_TITLE;
                    }
                }
            }
            drawLobbies(&evt, elapsedUs);

            break;
        }
        case ATR_SELECT:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down) // if the button is pressed down on the title screen
                {
                    if ((evt.button & PB_A))
                    {
                        atr->state = ATR_PROFILE;
                    }
                    if ((evt.button & PB_LEFT))
                    {
                        atr->selection--;
                        if (atr->selection < 0)
                        {
                            if (atr->numRemoteSwsn % SONA_PER == 0)
                            {
                                atr->selection = SONA_PER - 1;
                            }
                            else
                            {
                                atr->selection = (atr->numRemoteSwsn % SONA_PER) - 1;
                            }
                        }
                    }
                    else if ((evt.button & PB_RIGHT))
                    {
                        atr->selection++;
                        if (atr->selection > SONA_PER - 1)
                        {
                            if (atr->numRemoteSwsn % SONA_PER == 0)
                            {
                                atr->selection = 0;
                            }
                            else
                            {
                                atr->selection = (atr->numRemoteSwsn % SONA_PER) - 1;
                            }
                        }
                    }
                    else if ((evt.button & PB_B))
                    {
                        atr->state = ATR_DISPLAY; // if B is pressed, go to display view
                    }
                }
            }
            drawLobbies(&evt, elapsedUs);
            drawSonaSelector(evt, atr->selection);

            break;
        }
        case ATR_PROFILE:
        {
            viewProfile(&evt);

            break;
        }
        case ATR_EDIT_PROFILE:
        {
            // Draw the panel as it is
            editProfile(&evt);

            break;
        }
        default:
        {
            break;
        }
    }
}

// States
static void editProfile(buttonEvt_t* evt)
{
    if (atr->loadedProfs == false)
    {
        atr->loadedProfile = loadProfileFromNVS();
        atr->loadedProfs   = true;
    }

    drawCard(atr->loadedProfile, true); // draw own profile
    while (checkButtonQueueWrapper(evt))
    {
        if (evt->down)
        {
            if (evt->button & PB_B)
            {
                atr->state = ATR_TITLE;
            }
            if (evt->button & PB_A)
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
                }
            }
            if (evt->button & PB_UP)
            {
                atr->yloc--;
                if (atr->yloc < 0)
                {
                    atr->yloc = 0;
                }
            }
            if (evt->button & PB_DOWN)
            {
                atr->yloc++;
                if (atr->yloc > 4)
                {
                    atr->yloc = 4;
                }
            }
            if (evt->button & PB_LEFT)
            {
                drawEditUI(evt, atr->yloc, 1);
            }
            if (evt->button & PB_RIGHT)
            {
                drawEditUI(evt, atr->yloc, 0);
            }
        }
    }
    drawEditSelection(evt, atr->yloc);
}

static void viewProfile(buttonEvt_t* evt)
{
    ESP_LOGI(ATR_TAG, "Viewing profile %d on page %" PRId8, atr->selection, atr->page);
    ESP_LOGI(ATR_TAG, "sonas name is %s", atr->sonaList[atr->page * SONA_PER + atr->selection].swsn.name.nameBuffer);
    drawCard(atr->sonaList[atr->page * SONA_PER + atr->selection], false); // draw selected profile

    while (checkButtonQueueWrapper(evt))
    {
        if (evt->down)
        {
            if (evt->button & PB_B)
            {
                atr->state = ATR_DISPLAY;
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
    else
    {
        atr->fakeLoad = 1; // skip this next time
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

    // UI
    drawArrows(atr->left, atr->right, atr->up, atr->down);

    // Sonas
    loadProfiles(SONA_PER, atr->page);
    drawSonas(atr->page, elapsedUs);
}

// Draw
static void drawArcade(uint64_t elapsedUs)
{
    atr->animTimer += elapsedUs;
    if (atr->animTimer >= ANIM_TIMER_MS && atr->loadAnims < 20)
    {
        atr->animTimer = 0;
        atr->loadAnims++;
    }

    // Animations
    if (atr->loadAnims < 4)
    {
        drawWsgSimple(&atr->backgroundImages[2 + (atr->loadAnims % 4)], 0, 0);
    }
    else
    {
        atr->loadAnims = 0;
    }
}

static void drawConcert(uint64_t elapsedUs)
{
    // Draw base BG
    drawWsgSimple(&atr->backgroundImages[7], 0, 0);
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
    }

    // Draw sonas
    ESP_LOGI(ATR_TAG, "Drawing sonas for page %" PRId8 " and the remainder is %" PRId8, page, atr->remSwsn);
    ESP_LOGI(ATR_TAG, "Total pages: %" PRId8, atr->totalPages);

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
                // TODO: instrument animations
                drawWsgSimple(&atr->bodies[0], 20 + (i * 60),
                              128); // draw the body
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

static void drawArrows(bool left, bool right, bool up, bool down)
{ // TODO: change scale and positions
    if (atr->lbState != 0)
    {
        drawWsg(&atr->uiElements[(up) ? 1 : 0], LOBBY_BORDER_X, LOBBY_ARROW_Y, false, false, 270);
    }
    if (atr->lbState != BG_COUNT - 1)
    {
        drawWsg(&atr->uiElements[(down) ? 1 : 0], TFT_WIDTH - (LOBBY_BORDER_X + atr->uiElements[0].w), LOBBY_ARROW_Y,
                false, false, 90);
    }
}

static void drawCard(userProfile_t profile, bool local)
{
    ESP_LOGI(ATR_TAG, "Drawing card with cardSelect %" PRId8 ", fact0 %" PRId8 ", fact1 %" PRId8 ", fact2 %" PRId8,
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
    drawWsgSimple(&atr->cards[profile.cardSelect], 0, 0 + 12); // draw the card
    drawWsgSimple(&profile.swsn.image, SONALOC_X, SONALOC_Y);  // draw the sona image

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

    // drawWsgSimple(&atr->uiElements[16], 212, 124 + CARDTEXTPAD); // draw trophy image  TODO:something else here?
}

void drawEditSelection(buttonEvt_t* evt, int yloc)
{
    switch (yloc)
    {
        case 0:
        {
            drawWsgSimple(&atr->uiElements[18], 0, 12); // card select
            drawText(&atr->fonts[0], c000, editPromptText[0], 25, 200);
            break;
        }
        case 1:
        {
            drawWsg(&atr->uiElements[17], 90 - CARDTEXTPAD, 55, false, false, 270); // fact0
            drawText(&atr->fonts[0], c000, editPromptText[1], 25, 200);
            break;
        }
        case 2:
        {
            drawWsg(&atr->uiElements[17], 90 - CARDTEXTPAD, 68, false, false, 270); // fact1
            drawText(&atr->fonts[0], c000, editPromptText[2], 25, 200);
            break;
        }
        case 3:
        {
            drawWsg(&atr->uiElements[17], 90 - CARDTEXTPAD, 81, false, false, 270); // fact2
            drawText(&atr->fonts[0], c000, editPromptText[3], 25, 200);
            break;
        }
        case 4:
        {
            drawText(&atr->fonts[0], c000, editPromptText[4], 25, 200);
            break;
        }
        default:
        {
            break;
        }
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
                if (atr->loadedProfile.fact0 >= 7)
                {
                    atr->loadedProfile.fact0 = 0;
                }
                break;
            }
            case 2:
            {
                atr->loadedProfile.fact1++;
                if (atr->loadedProfile.fact1 >= 7)
                {
                    atr->loadedProfile.fact1 = 0;
                }
                break;
            }
            case 3:
            {
                atr->loadedProfile.fact2++;
                if (atr->loadedProfile.fact2 >= 7)
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
        for (int i = 0; i < maxProfiles; i++)
        {
            unpackProfileData(&atr->sonaList[page * 4 + i]);
            generateSwadgesonaImage(&atr->sonaList[page * 4 + i].swsn, false);
            ESP_LOGI(ATR_TAG, "Loaded profile %d for page %d", page * 4 + i, page);
            atr->bodyIdx[i] = rand() % ARRAY_SIZE(sonaBodies);
        }
        atr->loadedProfs = true; // mark as loaded
    }
}

userProfile_t loadProfileFromNVS(void)
{
    ESP_LOGI(ATR_TAG, "Loading profile from NVS");

    userProfile_t loadedProfile = {0};

    if (!readNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, ATRIUM_PACKEDKEY,
                            &atr->loadedProfile.packedProfile)) // check if profile created
    {
        loadedProfile.cardSelect = esp_random() % 8;
        loadedProfile.fact0      = esp_random() % 8;
        loadedProfile.fact1      = esp_random() % 8;
        loadedProfile.fact2      = esp_random() % 8;
        loadedProfile.numPasses  = 0;
        loadedProfile.points     = 0;

        packProfileData(&loadedProfile);

        writeNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, ATRIUM_PACKEDKEY, loadedProfile.packedProfile);

        ESP_LOGI(ATR_TAG, "Wrote random profile to NVS");
    }

    readNamespaceNvs32(ATRIUM_PROFILE_NVS_NAMESPACE, ATRIUM_PACKEDKEY, &loadedProfile.packedProfile);
    readNamespaceNvs32(TROPHY_NVS_NAMESPACE, TROPHY_POINTS_NVS_KEY, &loadedProfile.points);
    unpackProfileData(&loadedProfile);
    ESP_LOGI(ATR_TAG, "Loaded local swadgesona data");
    loadedProfile.numPasses = atr->numRemoteSwsn; // update number of passes each time

    loadSPSona(&loadedProfile.swsn.core); // load the sona image from swadgepass data

    ESP_LOGI(ATR_TAG, "Generating swadgesona image");

    generateSwadgesonaImage(&loadedProfile.swsn, false);

    ESP_LOGI(ATR_TAG, "Profile loaded from NVS: packedProfile=%" PRId32 ", numPasses=%" PRId8 ", points=%" PRId32,
             loadedProfile.packedProfile, loadedProfile.numPasses, loadedProfile.points);

    atr->loadedProfile = loadedProfile; // update loaded profile

    return atr->loadedProfile;
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
    profile->packedProfile = atr->loadedProfile.cardSelect; // cardselect cannot go over 15 ever emily
    profile->packedProfile += atr->loadedProfile.fact0 << 4;
    profile->packedProfile += atr->loadedProfile.fact1 << 8;
    profile->packedProfile += atr->loadedProfile.fact2 << 12;
    profile->packedProfile += atr->loadedProfile.numPasses << 16;
    printf("profile packed is %" PRId32, profile->packedProfile);
}

void unpackProfileData(userProfile_t* profile)
{
    /* clang-format off */
    profile->cardSelect = profile->packedProfile  &0b00000000000000000000000000001111;
    profile->fact0 = (profile->packedProfile      &0b00000000000000000000000011110000) >>4;
    profile->fact1 = (profile->packedProfile      &0b00000000000000000000111100000000) >>8;
    profile->fact2 = (profile->packedProfile      &0b00000000000000001111000000000000) >>12;
    profile->numPasses = (profile->packedProfile  &0b00000000111111110000000000000000) >>16;
    printf("unpacked profile is cardselect %" PRId8 ", fact0 %" PRId8 ", fact1 %" PRId8 ", fact2 %" PRId8 "",
           profile->cardSelect, profile->fact0, profile->fact1,
           profile->fact2);
    /* clang-format on */
}