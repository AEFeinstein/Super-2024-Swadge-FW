#include "atrium.h"
#include "nameList.h"
#include "swadgesona.h"

//---------------------------------------------------------------------------------//
// DEFINES
//---------------------------------------------------------------------------------//

// Lobby
#define MAX_SWADGESONA_IDXS (BG_COUNT * 4)
#define ANIM_TIMER_MS 16667
#define PLANT_FINAL_Y 192

/* int buttoncoord_y = 200; // y coord for the button row
int buttonpadding = 4;   // padding for text written in button, in px
int buttonwidth   = 60;  // width of the button, in px
int buttonheight  = 20;  // height of the button, in px, i probably want a sprite here but i'll draw a box for now

int cardpadding   = 4; // padding for text written in card, in px
int textbox1width = 156;
int textbox2width = 172;

// sona locations
int headoffset = 48; */

//---------------------------------------------------------------------------------//
// CONSTS
//---------------------------------------------------------------------------------//

// CNFS index lists
static const cnfsFileIdx_t sonaBodies[] = {DANCEBODY_1_WSG}; // Sona bodies
static const cnfsFileIdx_t uiImages[]   = {
    ARROWBUTTON_1_WSG, ARROWBUTTON_2_WSG, ABUTTON_1_WSG,  ABUTTON_2_WSG,
    BBUTTON_1_WSG,     BBUTTON_2_WSG,     ATRIUMLOGO_WSG, KEEPON_WSG,
};
static const cnfsFileIdx_t bgImages[] = {
    GAZEBO_WSG, ATRIUMPLANT_1_WSG, ATRIUMPLANT_2_WSG, ARCADE_1_WSG, ARCADE_2_WSG, CONCERT_1_WSG, CONCERT_2_WSG,
}; // Images used for the backgrounds
static const cnfsFileIdx_t cardImages[]
    = {CARDGEN_WSG, CARDBLOSS_WSG, CARDBUBB_WSG, CARDDINO_WSG, CARDMAGFEST_WSG, CARDMUSIC_WSG, CARDSPACE_WSG};
static const cnfsFileIdx_t midiBGM[] = {
    MADEIT_MID,
    ATRIUM_VIBE_MID,
};
static const cnfsFileIdx_t midiSFX[]   = {};
static const cnfsFileIdx_t fontsIdxs[] = {
    OXANIUM_13MED_FONT,
};

const char atriumModeName[] = "Atrium";

// Strings
static const char* const titleScreenText[] = {
    "SwadgePass",
    "Atrium",
    "Press A to Enter the Atrium",
    "Press B to View/Edit Your Profile",
};
/* static const char* const editButtonText[] = {
    "CANCEL",
    "SAVE",
};
static const char* const editPromptText[] = {
    "Choose Card", "Edit Sona", "Pick Sandwich", "Choose Identity", "Choose Location",
};
static const char* const editConfirmText[] = {
    "Are you sure?",
    "This will overwrite your profile.",
};
static const char* const editInstructText[] = {
    "Press arrows to scroll",
    "Press A to confirm selection",
}; */

/*
const char atriumNVSprofile[]    = "Atrium Profile:";
static const char* const fact0[] = {"PB&J", "BLT", "Cheese", "Reuben", "Hoagie", "Ice Cream", "Hot Dog", "Knuckle"};
static const char* const fact1[]
    = {"Bard", "Superfan", "Pinball Wizard", "Maker", "Sharpshooter", "Trashman", "Speed Runner", "Medic"};
static const char* const fact2[]
    = {"Arena", "Arcade", "Gazebo", "Soapbox", "Marketplace", "Panels", "Main Stage", "Tabletop"};
static const char* const preambles[] = {"Fave Sandwich: ", "I am a: ", "Find me at: "}; */

// static const char* const buttontext[] = {"CANCEL", "SAVE"};
// static const char* const prompttext[] = {"Choose Card", "Edit Sona", "Pick Sandwich", "Choose Identity", "Choose
// Location"};
// static const char* const confirmtext[] = {"Are you sure?", "This will overwrite your profile."};
// static const char* const instructtext[] = {"Press arrows to scroll", "Press A to confirm selection"};

/* int card_coords_x[] = {24, 99, 208};
int card_coords_y[]
    = {24 + 12, 112 + 12}; // needs testing, original location at 24,112 was too high on screen and clipping into radius
*/

// Coordinates
/* static const int textboxcoords_x[]  = {99, 24};             // x coords for the two textboxes
static const int textbox1coords_y[] = {39, 51, 63, 75};     // y coords for the first textbox lines
static const int textbox2coords_y[] = {124, 136, 148, 160}; // y coords for the second textbox lines
static const int buttoncoord_x[]    = {99, 24};             // x coord for the button columns
static const int x_coords[]         = {5, 74, 143, 212};
static const int y_coords[]         = {120, 80}; */

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

typedef struct
{
    int cardSelect;     // Active card
    int facts[3];       // List of facts
    bool local;         // If Local user
    int numPasses;      // Number of other unique passes encountered
    swadgesona_t* swsn; // Swadgesona data
} userProfile_t;

typedef struct __attribute__((packed))
{
    swadgesonaCore_t sona;
    int numPasses       : 13;
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
    userProfile_t loadedProfile; // Loaded profile for drawing

    // Main
    atriumState state;
    int8_t numRemoteSwsn;
    swadgesonaCore_t remoteSonas[MAX_NUM_SWADGE_PASSES];

    // Lobbies
    lobbyState_t lbState;
    uint8_t lobbySwsnIdxs[MAX_SWADGESONA_IDXS];
    bool left, right;
    int64_t animTimer;
    int loadAnims;

    // BGM
    midiPlayer_t* player;

    /* int selector; // which item is selected in the editor
    bool confirm;
    int x; // x position in editor select
    int y; // y position in editor select

    sonaSelect selection;
    int ticker;
    int PAGE;
    int planter;
    int dancecount;
    int loopcount;
    int numloops;
    int s; // sona index, 0-3
    int d; // dance chance */
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

// Draw
static void drawAtriumTitle(void);
static void drawLobbies(buttonEvt_t* evt, uint64_t elapsedUs);
static void shuffleSonas(void);
static void drawSonas(uint64_t elapsedUs);
static void drawArcade(uint64_t elapsedUs);
static void drawConcert(uint64_t elapsedUs);
static void drawGazebo(uint64_t elapsedUs);
static void drawGazeboForeground(uint64_t elapsedUs);
static void drawArrows(bool, bool);

// Swadgepass
static void atriumAddSP(struct swadgePassPacket* packet);

/*
static void viewProfile(userProfile prof);
uint32_t packProfile(userProfile prof);
userProfile unpackProfile(uint32_t packedProfile); */

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
    .wifiMode          = ESP_NOW,        // If we want WiFi
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

    /* // Swadgepass
    list_t spList = {0};
    getSwadgePasses(&spList, &atriumMode, true);
    node_t* spNode = spList.first;
    while (spNode)
    {
        // Make a convenience pointer to the data in this node
        swadgePassData_t* spd = (swadgePassData_t*)spNode->val;

        // If the data hasn't been used yet
        if (!isPacketUsedByMode(spd, &atriumMode))
        {
            // Print some packet data
            ESP_LOGI("SP", "Receive from %s. Preamble is %d", spd->key, spd->data.packet.preamble);

            // Mark the packet as used
            setPacketUsedByMode(spd, &atriumMode, true);
        }

        // Iterate to the next data
        spNode = spNode->next;
    }

     if (!readNvs32(atriumNVSprofile, &myProfile.created))
    {
        // We check if it found a value and if it didn't, we randomize a new profile for the user
        myProfile.cardselect = rand() % 8;
        myProfile.fact0      = rand() % 8;
        myProfile.fact1      = rand() % 8;
        myProfile.fact2      = rand() % 8;
        myProfile.sona       = rand() % 5;
        myProfile.mine       = 1; // this profile is mine

        myProfile.created = 1;
        printf("no profile found, creating new one\n");
        printf("randomized profile data: %" PRId32 ", %" PRId32 ", %" PRId32 ", %" PRId32 ", %" PRId32 ", %" PRId32
               "\n",
               myProfile.cardselect, myProfile.fact0, myProfile.fact1, myProfile.fact2, myProfile.sona, myProfile.mine);
        printf("profile nvs int is %" PRId32 "\n", myProfile.created);
        writeNvs32(atriumNVSprofile, myProfile.created);
    } */

    /* myUser = getSystemUsername();
    readNvs32(atriumNVSprofile, &myProfile.created);
    printf("my profile nvs int is %" PRId32 "\n", myProfile.created);
    // myProfile = unpackProfile(myProfile.created); */

    // BGM
    atr->player       = globalMidiPlayerGet(MIDI_BGM);
    atr->player->loop = true;
    midiGmOn(atr->player);
    globalMidiPlayerPlaySong(&atr->bgm[0], MIDI_BGM);
    globalMidiPlayerSetVolume(MIDI_BGM, 10);
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
    buttonEvt_t evt;
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
                        atr->state = ATR_PROFILE; // if B is pressed, go to profile view
                    }
                    // TODO: Load Profile before editing
                }
            }
            drawAtriumTitle();
            break;
        }
        case ATR_DISPLAY:
        {
            drawLobbies(&evt, elapsedUs);
            break;
        }
        case ATR_PROFILE:
        {
            while (checkButtonQueueWrapper(&evt))
            {
            }
            // viewProfile(atr->loadedProfile); // FIXME: Load swadge user's profile
            break;
        }
        case ATR_EDIT_PROFILE:
        {
            // TODO: Load Profile before editing (should be handled before this point)
            // editProfile(&evt);
            // Draw the panel as it is

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
    packet->atriumMode.profile.bodyMarks = atr->loadedProfile.swsn->core.bodyMarks; // TODO: Load everything
    packet->atriumMode.numPasses         = atr->loadedProfile.numPasses;
    packet->atriumMode.latestTrophyIdx   = 0; // FIXME: Load from NVS
}

// States
static void editProfile(buttonEvt_t* evt)
{
    /* int editorState = 0;
    switch (xselect)
    {
        case 0:
        {
            editorState = EDIT_CARD;
            drawText(font, c000, prompttext[0], 48, 200); // draw the text for selecting a card below the buttons
            printf("editor state is card\n");
            break;
        }
        case 1:
        {
            switch (yselect)
            {
                case 0:
                {
                    editorState = EDIT_SONA;
                    drawText(font, c000, prompttext[1], 48,
                             200); // draw the text for selecting a card below the buttons
                    printf("editor state is sona\n");
                    break;
                }
                case 1:
                {
                    editorState = EDIT_TEXT1;
                    // do more stuff with text lines later
                    // drawsomethingattextline();
                    printf("editor state is text1\n");
                    break;
                }
                case 2:
                {
                    editorState = EDIT_CANCEL;
                    printf("editor state is cancel\n");
                    break;
                }
            }
            break;
        }
        case 2:
        {
            switch (yselect)
            {
                case 0:
                {
                    editorState = EDIT_TEXT0;
                    printf("editor state is text0\n");
                    break;
                }
                case 1:
                {
                    editorState = EDIT_SYMBOL;
                    printf("editor state is symbol\n");
                    break;
                }
                case 2:
                {
                    editorState = EDIT_SAVE;
                    printf("editor state is save\n");
                    break;
                }
            }
            break;
        }
        case 3:
        {
            if (yselect == 0)
            {
                editorState = EDIT_CANCEL;
                printf("editor state is cancel\n");
            }
            else
            {
                editorState = EDIT_SAVE;
                printf("editor state is save\n");
            }
            break;
        }
    }

    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if ((evt.button & PB_A))
            {
                // do something based on editorState
            }

            if ((evt.button & PB_DOWN))
            {
                if (y < 2)
                {
                    y++;
                }
                else
                {
                    y = 0; // wraparound
                }
            }

            if ((evt.button & PB_UP))
            {
                if (y > 0)
                {
                    y--;
                }
                else
                {
                    y = 2; // wraparound
                }
            }

            if ((evt.button & PB_LEFT))
            {
                if (x > 0)
                {
                    x--;
                }
                else
                {
                    x = 3; // wraparound
                }
            }

            if ((evt.button & PB_RIGHT))
            {
                if (x < 3)
                {
                    x++;
                }
                else
                {
                    x = 0; // wraparound
                }
            }
        }
    }

    return editorState;
    // if (confirm == 1)
    // {
    //     // Save to NVS
    //     writeNvs32(atriumNVSprofile, packProfile(myProfile));
    //     confirm = 0;
    // }
     */
}

// Draw
static void drawAtriumTitle(void)
{
    // draw the title screen for the atrium mode
    drawRectFilled(0, 0, 280, 240, c000); // fill the screen with black
    drawText(&atr->fonts[0], c555, titleScreenText[0], 20, 20);
    drawText(&atr->fonts[0], c555, titleScreenText[1], 20, 40);
    drawText(&atr->fonts[0], c555, titleScreenText[2], 20, 100);
    drawText(&atr->fonts[0], c555, titleScreenText[3], 20, 120);
}

static void drawLobbies(buttonEvt_t* evt, uint64_t elapsedUs)
{
    // Shuffle
    shuffleSonas();

    // Handle input
    while (checkButtonQueueWrapper(evt))
    {
        if (evt->down)
        {
            if (evt->button & PB_UP)
            {
                shuffleSonas();
            }
            else if (evt->button & PB_LEFT)
            {
                atr->left = true;
            }
            else if (evt->button & PB_RIGHT)
            {
                atr->right = true;
            }
        }
        else
        {
            if (evt->button & PB_LEFT)
            {
                atr->left = false;
                if (atr->lbState > 0)
                {
                    atr->lbState--;
                }
                atr->loadAnims = 0;
            }
            else if (evt->button & PB_RIGHT)
            {
                atr->right = false;
                if (atr->lbState < BG_COUNT - 1)
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

    // Draw the loaded 'sonas
    drawSonas(elapsedUs);

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
    drawArrows(atr->left, atr->right);
    drawText(&atr->fonts[0], c000, "Press B to return to menu", 48, 216); // FIXME: Hardcoded

    // Trophy
    trophyUpdate(atriumTrophies[0], 1, 1); // trigger the trophy for entering a lobby
}

static void shuffleSonas()
{
    // TODO: Get all valid swadgesonas
    int numSwsns = 0;
    int array[atr->numRemoteSwsn];
    if (atr->numRemoteSwsn <= 0) // Bail if empty
    {
        return;
    }
    if (atr->numRemoteSwsn >= MAX_SWADGESONA_IDXS)
    {
        for (int idx = 0; idx < atr->numRemoteSwsn; idx++)
        {
            array[idx] = idx;
        }
        numSwsns = atr->numRemoteSwsn;
    }
    else
    {
        for (int idx = 0; idx < MAX_SWADGESONA_IDXS; idx++)
        {
            array[idx] = idx % atr->numRemoteSwsn;
        }
        numSwsns = MAX_SWADGESONA_IDXS;
    }
    // FISHER YATES
    // Iterate through the array in reverse order
    for (int n = numSwsns - 1; n > 0; n--)
    {
        // Generate a random index 'k' between 0 and n (inclusive)
        int32_t k = esp_random() % (n + 1);

        // Swap the elements at indices 'n' and 'k'
        int32_t temp = array[n];
        array[n]     = array[k];
        array[k]     = temp;
    }
    // Copy the indexs
    for (int idx = 0; idx < MAX_SWADGESONA_IDXS; idx++)
    {
        atr->lobbySwsnIdxs[idx] = array[idx];
    }
}

static void drawArcade(uint64_t elapsedUs)
{
    // Draw base BG
    drawWsgSimple(&atr->backgroundImages[3], 0, 0);

    // Animations
}

static void drawConcert(uint64_t elapsedUs)
{
    // Draw base BG
    drawWsgSimple(&atr->backgroundImages[6], 0, 0);

    // Animations
}

static void drawGazebo(uint64_t elapsedUs)
{
    // Draw base BG
    drawWsgSimple(&atr->backgroundImages[0], 0, 0);

    // Animations
    // Draw plants
    
}

static void drawGazeboForeground(uint64_t elapsedUs)
{
    atr->animTimer += elapsedUs;
    if (atr->animTimer >= ANIM_TIMER_MS)
    {
        atr->animTimer = 0;
        atr->loadAnims++;
    }
    if (atr->loadAnims <= 48)
    {               
        int offset = TFT_HEIGHT - atr->loadAnims;                                    
        drawWsgSimple(&atr->backgroundImages[1], 0, offset);   // draw plant 1 rising into view
        drawWsgSimple(&atr->backgroundImages[2], 168, offset); // draw plant 2 rising into view
    }
    else 
    {                                             
        drawWsgSimple(&atr->backgroundImages[1], 0, PLANT_FINAL_Y);   // draw plant 1
        drawWsgSimple(&atr->backgroundImages[2], 168, PLANT_FINAL_Y); // draw plant 2
    }
}

static void drawSonas(uint64_t elapsedUs)
{
    // tester to see if the sonas show up right. up to 4 sonas drawn on the screen at one time. eventually, random
    // swadgepass lines shall be selected to draw. in this tester, random hardcoded sonas are drawn in the required
    // positions.

    /* int j = 0; // placeholder for drawing a second row maybe someday. one struggle at a time

    for (int i = 0; i < 4; i++)
    {
        int h = rand() % 7; // randomize head select
        // int h = 2; //for now, just use this head
        drawWsgSimple(miscArray[h], x_coords[i],
                      y_coords[j] - headoffset); // place head on top of body minus offset
    }

    changer = 0;
 */
    // end sona tester
}

static void drawArrows(bool left, bool right)
{
    if (atr->lbState != BG_COUNT - 1)
    {
        drawWsg(&atr->uiElements[(right) ? 1 : 0], 260, 40, false, false, 0); // FIXME: Hardcoded
    }
    if (atr->lbState != 0)
    {
        drawWsg(&atr->uiElements[(left) ? 1 : 0], 20, 40, false, false, 180); // FIXME: Hardcoded
    }
}

/*
void viewProfile(userProfile prof)
{
    // printf("In view profile state\n");

    // printf("Cardselect: %d, Fact0: %d, Fact1: %d, Fact2: %d, Sona: %d, Mine: %d\n", prof.cardselect, prof.fact0,
    //        prof.fact1, prof.fact2, prof.sona, prof.mine);
    if (loader == 0)
    {
        // WSG for profile mode only
        loadWsg(CARDBLOSS_WSG, &cards->card1, true);
        loadWsg(CARDBUBB_WSG, &cards->card2, true);
        loadWsg(CARDDINO_WSG, &cards->card3, true);
        loadWsg(CARDGEN_WSG, &cards->card4, true);
        loadWsg(CARDMAGFEST_WSG, &cards->card5, true);
        loadWsg(CARDMUSIC_WSG, &cards->card6, true);
        loadWsg(CARDSPACE_WSG, &cards->card7, true);
        loadWsg(WINGED_TROPHY_WSG, &misc->trophy, true);
        loader = 1;

        cardsArray[0] = &cards->card1;
        cardsArray[1] = &cards->card2;
        cardsArray[2] = &cards->card3;
        cardsArray[3] = &cards->card4;
        cardsArray[4] = &cards->card5;
        cardsArray[5] = &cards->card6;
        cardsArray[6] = &cards->card7;
    }

    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if ((evt.button & PB_A))
            {
                printf("PB_A; prof.mine %d\n", prof.mine);
                if (prof.mine == 1)
                {
                    state = ATR_EDIT_PROFILE; // if A is pressed, go to edit profile
                    printf("Attempting to play edit bgm\n");
                    globalMidiPlayerPlaySong(&amidi->edit_bgm, MIDI_BGM);
                    editProfile(x, y);
                }
                else
                {
                    // do nothing, only let user edit their own profile
                }
            }
            else if ((evt.button & PB_B))
            {
                state  = ATR_TITLE; // if B is pressed, go back to title view
                loader = 0;         // reset the loader so the card wsgs are freed and reloaded next time
                for (int i = 0; i < (sizeof(cards_t) / sizeof(wsg_t)); i++) // don't hardcode this
                {
                    printf("freeing card wsg %d\n", i);
                    freeWsg(cardsArray[i]);
                    printf("freed card wsg %d\n", i);
                }
                freeWsg(&misc->trophy);
            }
        }
    }

    // concat card info
    // line 0: name
    // no concat required
    // line 1: fact0
    char factline0[64];
    snprintf(factline0, sizeof(factline0) - 1, "%s%s", preambles[0], fact0[prof.fact0]);
    // line 2: fact1
    char factline1[64];
    snprintf(factline1, sizeof(factline1) - 1, "%s%s", preambles[1], fact1[prof.fact1]);
    // line 3: fact2
    char factline2[64];
    snprintf(factline2, sizeof(factline2) - 1, "%s%s", preambles[2], fact2[prof.fact2]);
    // draw the card info
    drawWsgSimple(&bgs->gazebo, 0, 0);
    drawWsgSimple(cardsArray[prof.cardselect], 0, 0 + 12);                   // draw the card
    drawWsgSimple(miscArray[prof.sona], card_coords_x[0], card_coords_y[0]); // draw the sona head

    drawText(font, c000, myUser->nameBuffer, card_coords_x[1] + cardpadding,
             card_coords_y[0] + cardpadding); // draw the name
    drawText(font, c000, factline0, card_coords_x[1] + cardpadding,
             card_coords_y[0] + cardpadding + 13 + 4); // draw the sandwich info
    drawText(font, c000, factline1, card_coords_x[1] + cardpadding,
             card_coords_y[0] + cardpadding + 26 + 4); // draw the identity
    drawText(font, c000, factline2, card_coords_x[1] + cardpadding,
             card_coords_y[0] + cardpadding + 39 + 4); // draw the identity info

    drawText(font, c000, "Hello World!", card_coords_x[0] + cardpadding,
             card_coords_y[1] + cardpadding); // draw the second text box info
    drawText(font, c000, "This is 22 characters.", card_coords_x[0] + cardpadding,
             card_coords_y[1] + cardpadding + 12); // draw the second text box info
    drawText(font, c000, "Magfest is a: Donut", card_coords_x[0] + cardpadding,
             card_coords_y[1] + cardpadding + 24); // draw the second text box info
    drawWsgSimpleScaled(&misc->trophy, card_coords_x[2] + 7, card_coords_y[1] + 3, 1,
                        1);                                      // draw the trophy to fill in the small box for testing
    drawText(font, c000, "Press A to Edit My Profile", 48, 200); // draw the text for selecting a card
    drawText(font, c000, "Press B to return to menu", 48, 216);  // draw the text for selecting a card
}

uint32_t packProfile(userProfile prof)
{
    packedUserProfile_t ret = {
        .profile.cardselect = prof.cardselect,
        .profile.fact0      = prof.fact0,
        .profile.fact1      = prof.fact1,
        .profile.fact2      = prof.fact2,
        .profile.sona       = prof.sona,
        .profile.mine       = prof.mine,
    };
    return ret.data;
}

userProfile unpackProfile(uint32_t packedProfile)
{
    packedUserProfile_t pp      = {.data = packedProfile};
    userProfile unpackedprofile = {.cardselect = pp.profile.cardselect,
                                   .fact0      = pp.profile.fact0,
                                   .fact1      = pp.profile.fact1,
                                   .fact2      = pp.profile.fact2,
                                   .sona       = pp.profile.sona,
                                   .mine       = pp.profile.mine};

    printf("unpacked cardselect is %" PRId32 "\n", unpackedprofile.cardselect);
    printf("unpacked fact0 is %" PRId32 "\n", unpackedprofile.fact0);
    printf("unpacked fact1 is %" PRId32 "\n", unpackedprofile.fact1);
    printf("unpacked fact2 is %" PRId32 "\n", unpackedprofile.fact2);
    printf("unpacked sona is %" PRId32 "\n", unpackedprofile.sona);
    printf("unpacked mine is %" PRId32 "\n", unpackedprofile.mine);

    return unpackedprofile;
}
 */