#include "atrium.h"
#include "nameList.h"
#include "swadgesona.h"
#include "swadgePass.h"

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
#define SPSONA_NVS_KEY               "spSona"
#define TROPHY_NVS_NAMESPACE         "trophy"
#define TROPHY_POINTS_NVS_KEY        "points"

//---------------------------------------------------------------------------------//
// CONSTS
//---------------------------------------------------------------------------------//

// CNFS index lists
static const cnfsFileIdx_t sonaBodies[] = {DANCEBODY_1_WSG}; // Sona bodies
static const cnfsFileIdx_t uiImages[]
    = {ARROWBUTTON_1_WSG, ARROWBUTTON_2_WSG, ABUTTON_1_WSG, ABUTTON_2_WSG, BBUTTON_1_WSG,   BBUTTON_2_WSG,
       ATRIUMLOGO_WSG,    KEEPON_WSG,        LOADING_1_WSG, LOADING_2_WSG, LOADING_3_WSG,   LOADING_4_WSG,
       LOADING_5_WSG,     LOADING_6_WSG,     LOADING_7_WSG, LOADING_8_WSG, GOLD_TROPHY_WSG, ARROW_WSG, CARDSELECT_WSG};
static const cnfsFileIdx_t bgImages[] = {
    GAZEBO_WSG, ATRIUMPLANT_1_WSG, ARCADE_1_WSG, ARCADE_2_WSG, ARCADE_3_WSG, ARCADE_4_WSG, CONCERT_1_WSG, CONCERT_2_WSG,
}; // Images used for the backgrounds
static const cnfsFileIdx_t cardImages[]
    = {CARDGEN_WSG, CARDBLOSS_WSG, CARDBUBB_WSG, CARDDINO_WSG, CARDMAGFEST_WSG, CARDMUSIC_WSG, CARDSPACE_WSG};
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

const char atriumModeName[] = "Atrium";

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

static const char* const fact0[] = {"PB&J", "BLT", "Cheese", "Reuben", "Hoagie", "Ice Cream", "Hot Dog", "Knuckle"};
static const char* const fact1[]
    = {"Bard", "Superfan", "Pinball Wizard", "Maker", "Sharpshooter", "Trashman", "Speed Runner", "Medic"};
static const char* const fact2[]
    = {"Arena", "Arcade", "Gazebo", "Soapbox", "Marketplace", "Panels", "Main Stage", "Tabletop"};
static const char* const preambles[] = {"Fave Sandwich: ", "I am a: ", "Fave place: "};
static const char* const editPromptText[] = {
    "Choose Card", "Choose Identity", "Choose Location", "Pick Sandwich"
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

//---------------------------------------------------------------------------------//
// ENUMS
//---------------------------------------------------------------------------------//

typedef enum
{
    ATR_TITLE,        /// TITLE SCREEN
    ATR_DISPLAY,      /// GAZEBO ETC
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

typedef struct __attribute__((packed))
{
    int cardSelect; // Active card
    int fact0;
    int fact1;
    int fact2;
    bool local;         // If Local user
    int numPasses;      // Number of other unique passes encountered
    swadgesona_t* swsn; // Swadgesona data
} userProfile_t;

typedef struct __attribute__((packed))
{
    swadgesonaCore_t sona;
    userProfile_t profile;
    int latestTrophyIdx : 32;
} profilePacket_t;

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
    profilePacket_t packedProfile;
    userProfile_t loadedProfile; // Loaded profile for drawing

    // Main
    atriumState state;
    int8_t numRemoteSwsn;
    profilePacket_t sonaList[MAX_NUM_SWADGE_PASSES];

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

    // BGM
    midiPlayer_t* player;

    // SwadgePass List
    list_t spList;
    list_t loadedProfilesList; // list of userProfile_t*

    // Profile viewer/editor
    int selection;
    int xloc;
    int yloc;

    // trophy data
    trophyData_t* latestTrophy;
    int points;
    /* int selector; // which item is selected in the editor
    bool confirm;
    int x; // x position in editor select
    int y; // y position in editor select

 */
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
static void drawCard(userProfile_t profile);
void drawEditSelection(int yloc);
void drawEditUI(int yloc);

profilePacket_t loadProfileFromNVS();
trophyData_t* getTrophy();

// Swadgepass
static void atriumAddSP(struct swadgePassPacket* packet);
void loadProfiles(list_t* spList, list_t* loadedProfilesList, int maxProfiles, bool local, int page);

//---------------------------------------------------------------------------------//
// VARIABLES
//---------------------------------------------------------------------------------//

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

trophySettings_t atriumTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 4,
    .slideDurationUs  = DRAW_SLIDE_US,
};

trophyDataList_t atriumTrophyData = {
    .settings = &atriumTrophySettings,
    .list     = atriumTrophies,
    .length   = ARRAY_SIZE(atriumTrophies),
};

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
    //.trophyData               = &atriumTrophyData,
    .fnAddToSwadgePassPacket = atriumAddSP, // function to add data to swadgepass packet
};

atrium_t* atr;

//---------------------------------------------------------------------------------//
// FUNCTIONS
//---------------------------------------------------------------------------------//

static void atriumEnterMode()
{
    // Initialize memory
    atr         = (atrium_t*)heap_caps_calloc(1, sizeof(atrium_t), MALLOC_CAP_8BIT);
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
        swadgePassData_t* spd = (swadgePassData_t*)spNode->val;
        atr->sonaList[i].sona = spd->data.packet.swadgesona.core;

        // If the data hasn't been used yet
        if (!isPacketUsedByMode(spd, &atriumMode))
        {
            // Print some packet data
            ESP_LOGI("SP", "Receive from %s. Preamble is %d", spd->key, spd->data.packet.preamble);

            // Mark the packet as used
            setPacketUsedByMode(spd, &atriumMode, true);
        }

        // Iterate to the next data
        i++;
        spNode = spNode->next;
    }

    atr->numRemoteSwsn = i;

    // BGM
    atr->player       = globalMidiPlayerGet(MIDI_BGM);
    atr->player->loop = true;
    midiGmOn(atr->player);
    globalMidiPlayerPlaySong(&atr->bgm[1], MIDI_BGM);
    globalMidiPlayerSetVolume(MIDI_BGM, 13);

    // profile created yet?
    if (!readNvs32("numPasses", &atr->loadedProfile.numPasses))
    {
        atr->state = ATR_EDIT_PROFILE; // go to profile edit if no profile yet
    }
    else
    {
        atr->state = ATR_TITLE; // else go to title screen
    }
}

static void atriumExitMode()
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
                        // Changer
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
            drawLobbies(&evt, elapsedUs);

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

            // Draw the selection boxes

            break;
        }
        default:
        {
            break;
        }
    }
}

void atriumAddSP(struct swadgePassPacket* packet)
{
    // FIXME: Load the appropriate mode
  
}

// States
static void editProfile(buttonEvt_t* evt)
{
    if (atr->loadedProfs == false)
    {
        atr->latestTrophy        = getTrophy(); // get latest trophy
        atr->packedProfile       = loadProfileFromNVS();
        atr->loadedProfile.local = true; // local profile
        atr->loadedProfs         = true;
        printf("Loaded profile with %d card\n", atr->loadedProfile.cardSelect);
    }

    drawCard(atr->loadedProfile);
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
                //drawEditUI(atr->yloc);
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
                if (atr->yloc > 3)
                {
                    atr->yloc = 3;
                }
            }
            if (evt->button & PB_LEFT)
            {
                
            }
            if (evt->button & PB_RIGHT)
            {
               
            }
        }
    }
    drawEditSelection(atr->yloc);
}

static void viewProfile(buttonEvt_t* evt)
{
}

// Draw
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

    // Handle input
    while (checkButtonQueueWrapper(evt))
    {
        atr->lastPage = atr->page;
        if (evt->down)
        {
            if (evt->button & PB_UP)
            {
                atr->up = true;
            }
            else if (evt->button & PB_LEFT)
            {
                atr->left = true;
                atr->page--;
                if (atr->page < 0)
                {
                    atr->page = 0;
                }
            }
            else if (evt->button & PB_RIGHT)
            {
                atr->right = true;
                atr->page++;
                if (atr->page > (atr->numRemoteSwsn - 1) / 4)
                {
                    atr->page = (atr->numRemoteSwsn - 1) / 4;
                }
            }
            else if (evt->button & PB_DOWN)
            {
                atr->down = true;
            }
        }
        else
        {
            if (evt->button & PB_UP)
            {
                atr->up = false;
                if (atr->lbState > 0)
                {
                    atr->lbState--;
                }
                atr->loadAnims = 0;
            }
            else if (evt->button & PB_DOWN)
            {
                atr->down = false;
                if (atr->lbState < 2)
                {
                    atr->lbState++;
                }

                atr->loadAnims = 0;
            }
            else if (evt->button & PB_B)
            {
                atr->state = ATR_TITLE;
            }
        }
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
    drawSonas(atr->page, elapsedUs);
    // Trophy
}

void shuffleSonas()
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
            }
        }
    }
    atr->shuffle = 1; // only shuffle once
}

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

    // Animations
}

static void drawGazebo(uint64_t elapsedUs)
{
    // Draw base BG
    drawWsgSimple(&atr->backgroundImages[0], 0, 0);

    // Animations
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
    // move down spList past the pages i don't want to draw
    if (atr->lastPage != page)
    {
        for (int i = 0; i < page * 4; i++)
        {
            if ((i == page * 4 - 1 && atr->loadedProfs == true))
            {
                atr->loadedProfs = false; // reset loaded profiles to load new ones
            }
            
        }
        atr->loadedProfs = false; // reset loaded profiles to load new ones
        printf("Page changed from %d to %d, resetting loadedProfs\n", atr->lastPage, page);
    }

    if (atr->loadedProfs == false)
    {
        loadProfiles(&atr->spList, &atr->loadedProfilesList, 4, 1,
                     page); // load up to 4 profiles into loadedProfilesList
        atr->loadedProfs = true;
        
    }

    // Draw sonas
    printf("Drawing sonas for page %d\n", page);
    node_t* currentNode = atr->loadedProfilesList.first;
    for (int i = 0; i < atr->loadedProfilesList.length && currentNode != NULL; i++)
    {
        atr->loadedProfile = *(userProfile_t*)currentNode->val;

        if (atr->loadedProfile.swsn->image.w != 0)
        {
            freeWsg(&atr->loadedProfile.swsn->image);
            
        }

        generateSwadgesonaImage(atr->loadedProfile.swsn, false);
        drawWsgSimple(&atr->loadedProfile.swsn->image, 20 + i * 64, 100);
        currentNode = currentNode->next;
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

static void drawCard(userProfile_t profile)
{
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

    nameData_t username;
    if (profile.local == true)
    {
        username = *getSystemUsername();
    }
    else
    {
        username = profile.swsn->name;
    }

    // loadSPSona(&profile.swsn->core);
    // drawWsgSimple(&profile.swsn->image, SONALOC_X, SONALOC_Y); // draw the swadgesona image

    // TODO Ensure this is generated
    drawText(&atr->fonts[0], c000, username.nameBuffer, 99 + CARDTEXTPAD,
             SONALOC_Y + CARDTEXTPAD); // draw the name
    drawText(&atr->fonts[0], c000, factline0, 99 + CARDTEXTPAD,
             SONALOC_Y + CARDTEXTPAD + 13 + CARDTEXTPAD); // draw the sandwich info
    drawText(&atr->fonts[0], c000, factline1, 99 + CARDTEXTPAD,
             SONALOC_Y + CARDTEXTPAD + 26 + CARDTEXTPAD); // draw the identity
    drawText(&atr->fonts[0], c000, factline2, 99 + CARDTEXTPAD,
             SONALOC_Y + CARDTEXTPAD + 39 + CARDTEXTPAD); // draw the identity info

    drawText(&atr->fonts[0], c000, "Latest Trophy:", 24 + CARDTEXTPAD, 126);         // draw latest trophy title
    drawText(&atr->fonts[0], c000, atr->latestTrophy->title, 24 + CARDTEXTPAD, 138); // draw latest trophy title
    drawText(&atr->fonts[0], c000, "Points:", 24 + CARDTEXTPAD, 150);                // draw latest trophy desc
    char buf[sizeof(&atr->points) + 1];
    snprintf(buf, sizeof(buf), "%d", atr->points);
    drawText(&atr->fonts[0], c000, buf, 65 + CARDTEXTPAD, 150);  // draw latest trophy desc
    drawWsgSimple(&atr->uiElements[16], 212, 124 + CARDTEXTPAD); // draw trophy image
}

void drawEditSelection(int yloc)
{
    switch (yloc)
    {
        case 0:
            drawWsgSimple(&atr->uiElements[18], 0, 12); // card select
            drawText(&atr->fonts[0], c000, editPromptText[0], 25, 200);
            break;
        case 1:
            drawWsg(&atr->uiElements[17], 90-CARDTEXTPAD, 55, false, false, 270); // fact0
            drawText(&atr->fonts[0], c000, editPromptText[1], 25, 200);
            break;
        case 2:
            drawWsg(&atr->uiElements[17], 90-CARDTEXTPAD, 68, false, false, 270); // fact1
            drawText( &atr->fonts[0], c000, editPromptText[2], 25, 200);
            break;
        case 3:
            drawWsg(&atr->uiElements[17], 90-CARDTEXTPAD, 81, false, false, 270); // fact2
            drawText( &atr->fonts[0], c000, editPromptText[3], 25, 200);
            break;
        default:
            break;
    }
}
void loadProfiles(list_t* spList, list_t* loadedProfilesList, int maxProfiles, bool remote, int page)
{
    printf("Loading profiles: maxProfiles=%d, remote=%d, page=%d\n", maxProfiles, remote, page);
    // Clear the loadedProfilesList
    while (loadedProfilesList->first != NULL)
    {
        node_t* temp              = loadedProfilesList->first;
        loadedProfilesList->first = loadedProfilesList->first->next;
        heap_caps_free(temp->val);
        heap_caps_free(temp);
    }
    loadedProfilesList->last   = NULL;
    loadedProfilesList->length = 0;

    if (remote == true)
    {
        // Load profiles from spList into loadedProfilesList
        node_t* spNode = spList->first;
        int count      = 0;
        for (int i = 0; i < page * maxProfiles; i++) // skip pages
        {
            if (spNode != NULL)
            {
                spNode = spNode->next;
            }
            if (i == page * maxProfiles - 1)
            {
                printf("Skipped to page %d, node %d\n", page, i + 1);
            }
        }
        while (spNode != NULL && count < maxProfiles)
        {
            swadgePassData_t* spd = (swadgePassData_t*)spNode->val;

            // Create a new userProfile_t
            userProfile_t* profile = (userProfile_t*)heap_caps_calloc(1, sizeof(userProfile_t), MALLOC_CAP_8BIT);
            profile->cardSelect    = spd->data.packet.atrium.cardSelect;
            profile->fact0         = spd->data.packet.atrium.fact0;
            profile->fact1         = spd->data.packet.atrium.fact1;
            profile->fact2         = spd->data.packet.atrium.fact2;

            profile->local     = remote; // Remote profile?
            profile->numPasses = spd->data.packet.atrium.numPasses;

            // Copy swadgesona data
            profile->swsn = (swadgesona_t*)heap_caps_calloc(1, sizeof(swadgesona_t), MALLOC_CAP_8BIT);
            memcpy(&profile->swsn->core, &spd->data.packet.swadgesona.core, sizeof(swadgesonaCore_t));

            // Add to loadedProfilesList
            node_t* newNode = (node_t*)heap_caps_calloc(1, sizeof(node_t), MALLOC_CAP_8BIT);
            newNode->val    = profile;
            newNode->next   = NULL;
            if (loadedProfilesList->last == NULL)
            {
                loadedProfilesList->first = newNode;
                loadedProfilesList->last  = newNode;
            }
            else
            {
                loadedProfilesList->last->next = newNode;
                loadedProfilesList->last       = newNode;
            }

            loadedProfilesList->length++;
            count++;
            spNode = spNode->next;
        }
    }
    else
    {
    }
}

trophyData_t* getTrophy()
{
    trophyData_t* tempTrophy;
    tempTrophy = trophyGetLatest();
    // load points from NVS
    readNamespaceNvs32(TROPHY_NVS_NAMESPACE, TROPHY_POINTS_NVS_KEY, &atr->points);
    return tempTrophy;
}

profilePacket_t loadProfileFromNVS()
{
    printf("Loading profile from NVS\n");

    profilePacket_t packedProfile;

    if (!readNvs32("cardSelect", &packedProfile.profile.cardSelect))
    {
        packedProfile.profile.cardSelect = rand() % 8;
        packedProfile.profile.fact0      = rand() % 8;
        packedProfile.profile.fact1      = rand() % 8;
        packedProfile.profile.fact2      = rand() % 8;
        packedProfile.profile.local      = true;
        packedProfile.profile.numPasses  = 0;

        writeNvs32("cardSelect", packedProfile.profile.cardSelect);
        writeNvs32("fact0", packedProfile.profile.fact0);
        writeNvs32("fact1", packedProfile.profile.fact1);
        writeNvs32("fact2", packedProfile.profile.fact2);
        writeNvs32("numPasses", packedProfile.profile.numPasses);
        printf("Wrote random profile to NVS\n");
    }

    readNvs32("cardSelect", &packedProfile.profile.cardSelect);
    readNvs32("fact0", &packedProfile.profile.fact0);
    readNvs32("fact1", &packedProfile.profile.fact1);
    readNvs32("fact2", &packedProfile.profile.fact2);

    //loadSPSona(&packedProfile.sona); //TODO: why doesn't this work?
    //generateSwadgesonaImage(packedProfile.profile.swsn, false);

    printf("Loaded local swadgesona data\n");
    packedProfile.profile.numPasses = atr->numRemoteSwsn; // update number of passes each time
    writeNvs32("numPasses", packedProfile.profile.numPasses);
    printf("Profile loaded from NVS: cardSelect=%d, fact0=%d, fact1=%d, fact2=%d, numPasses=%d\n",
           packedProfile.profile.cardSelect, packedProfile.profile.fact0, packedProfile.profile.fact1,
           packedProfile.profile.fact2, packedProfile.profile.numPasses);

    atr->loadedProfile = packedProfile.profile; // update loaded profile

    return atr->packedProfile;
}