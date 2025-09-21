#include "atrium.h"

/// @brief

static void atriumEnterMode(void);
static void atriumExitMode(void);
static void atriumMainLoop(int64_t elapsedUs);
static void sonaDraw(void);
static void sonaIdle(void);
static void atriumTitle(void);
static void viewProfile(void);
static void editProfile(int xselect, int yselect);
static void atriumAddSP(struct swadgePassPacket* packet);

//---------------------------------------------------------------------------------//
// TROPHY CASE SPECIFIC INFORMATION
//---------------------------------------------------------------------------------//
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
    .drawFromBottom   = true,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
};

trophyDataList_t atriumTrophyData = {
    .settings = &atriumTrophySettings,
    .list     = atriumTrophies,
    .length   = ARRAY_SIZE(atriumTrophies),
};

//---------------------------------------------------------------------------------//
// Setup
//---------------------------------------------------------------------------------//

const char atriumModeName[] = "Atrium"; // idk working title
swadgeMode_t atriumMode     = {
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

int testercardselect = 0; // for testing card selection, 0-6
int card_coords_x[]  = {24, 99, 208};
int card_coords_y[]
    = {24 + 12, 112 + 12}; // needs testing, original location at 24,112 was too high on screen and clipping into radius
int cardpadding   = 4;     // padding for text written in card, in px
int textbox1width = 156;
int textbox2width = 172;

buttonEvt_t evt;

// STATE MACHINE FOR ATRIUM MAIN LOOP
typedef enum
{

    ATR_TITLE,
    /// TITLE SCREEN
    ATR_ATR,
    /// GAZEBO ETC
    ATR_PROFILE,
    /// LOOKING AT A PROFILE
    ATR_EDIT_PROFILE,
    //  EDITING MY PROFILE

} atriumState;

atriumState state = 0;
int loader        = 0;

//---------------------------------------------------------------------------------//
// SONA ATRIUM SPECIFIC INFORMATION
//---------------------------------------------------------------------------------//

typedef enum
{
    /// nothing selected
    NOTHING,
    /// SONA 0
    SONA0,
    /// SONA 1
    SONA1,
    /// SONA 2
    SONA2,
    /// SONA 3
    SONA3,
    /// PAGE FWD
    PAGE_FWD,
    /// PAGE BACK
    PAGE_BACK,
    /// RETURN TO TITLE
    RETURN_TITLE,

} sonaSelect;

sonaSelect selection = 0;
int ticker           = 0;
int PAGE             = 0;
int planter          = 0;

// sona locations
int x_coords[] = {5, 74, 143, 212};
int y_coords[] = {120, 80};
int headoffset = 48;

//---------------------------------------------------------------------------------//
// STRUCTURES FOR WSGS, MIDIS, ETC
//---------------------------------------------------------------------------------//

// Backgrounds
typedef struct
{
    wsg_t gazebo;
    wsg_t arcade1;
    wsg_t concert1;
    wsg_t arcade2;
    wsg_t concert2;
    wsg_t plant1;
    wsg_t plant2;
} backgrounds_t;

backgrounds_t* bgs;
const wsg_t* bgsArray[sizeof(backgrounds_t)];

// Bodies
typedef struct
{
    wsg_t d1;
    wsg_t d2;
    wsg_t d3;
    wsg_t d4;
    wsg_t d5;
    wsg_t d6;
    wsg_t d7;
    wsg_t d8;
    wsg_t d9;
    wsg_t d10;

} bodies_t;

bodies_t* bods;
const wsg_t* bodsArray[sizeof(bodies_t)];

int dancecount = 0;
int loopcount  = 0;
int numloops   = 0;
int s          = 0; // sona index, 0-3
int d          = 0; // dance chance

// Cards
typedef struct
{
    wsg_t card1;
    wsg_t card2;
    wsg_t card3;
    wsg_t card4;
    wsg_t card5;
    wsg_t card6;
    wsg_t card7;

} cards_t;

cards_t* cards;
const wsg_t* cardsArray[sizeof(cards_t)];

// Buttons
typedef struct
{
    wsg_t arrow;
} butts_t; // haha

butts_t* butts;

// misc
typedef struct
{
    wsg_t bald;
    wsg_t mainc;
    wsg_t num1;
    wsg_t num2;
    wsg_t pomp;
    wsg_t num3;
    wsg_t cow;
    wsg_t trophy;
    font_t font;
} misc_t;

misc_t* misc;
const wsg_t* miscArray[sizeof(misc_t)];

// midis

typedef struct
{
    midiFile_t bgm;
} amidi_t;

amidi_t* amidi;
const midiFile_t* midiArray[sizeof(amidi_t)];

// editor variables
int x = 0; // x position in editor
int y = 0; // y position in editor

//---------------------------------------------------------------------------------//
// SWADGEPASS SEPECIFIC INFORMATION
//---------------------------------------------------------------------------------//
list_t spList = {0};

typedef enum {
    PBNJ,
    BLT,
    GRILLED,
    REUBEN,
    HOAGIE,
    ICECREAM,
    HOTDOG,
    ANDKNUCKLES,
} fact0; //SANDWICH

typedef enum {
    BARD,
    TECHNOMANCER,
    PINBALL,
    MAKER,
    SHARPSHOOTER,
    TRASHMAN,
    SPEEDRUNNER,
    MEDIC,
} fact1; //IAMA

typedef enum {
    ARENA,
    ARCADE,
    GAZEBO,
    SOAPBOX,
    MAKERSPACE,
    PANEL,
    STAGE,
    TABLETOP,
} fact2; //LOCATION

static void atriumEnterMode()
{
    bgs   = (backgrounds_t*)heap_caps_calloc(1, sizeof(backgrounds_t), MALLOC_CAP_8BIT);
    bods  = (bodies_t*)heap_caps_calloc(1, sizeof(bodies_t), MALLOC_CAP_8BIT);
    misc  = (misc_t*)heap_caps_calloc(1, sizeof(misc_t), MALLOC_CAP_8BIT);
    cards = (cards_t*)heap_caps_calloc(1, sizeof(cards_t), MALLOC_CAP_8BIT);
    amidi = (amidi_t*)heap_caps_calloc(1, sizeof(amidi_t), MALLOC_CAP_8BIT);
    butts = (butts_t*)heap_caps_calloc(1, sizeof(butts_t), MALLOC_CAP_8BIT);
    // Dear Emily, you need to do this for every struct. love, past you.

    font = misc->font;
    loadFont(IBM_VGA_8_FONT,font,true);

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

    // when its ready, I need to add sona data extraction from swadgepass packet, for now I am just hardcoding some
    // sonas in to use:
    loadWsg(BALD_WSG, &misc->bald, true);
    loadWsg(MAINCHAR_WSG, &misc->mainc, true);
    loadWsg(NUM_1_WSG, &misc->num1, true);
    loadWsg(NUM_2_WSG, &misc->num2, true);
    loadWsg(NUM_3_WSG, &misc->num3, true);
    loadWsg(POMP_WSG, &misc->pomp, true);
    loadWsg(COW_WSG, &misc->cow, true);

    printf("loaded misc wsgs!\n");

    // test animations for sonas - turned off for now
    loadWsg(DANCEBODY_1_WSG, &bods->d1, true);
    /* loadWsg(DANCEBODY_2_WSG, &bods->d2, true);
     loadWsg(DANCEBODY_3_WSG, &bods->d3, true);
     loadWsg(DANCEBODY_4_WSG, &bods->d4, true);
     loadWsg(DANCEBODY_5_WSG, &bods->d5, true);
     loadWsg(DANCEBODY_6_WSG, &bods->d6, true);
     loadWsg(DANCEBODY_7_WSG, &bods->d7, true);
     loadWsg(DANCEBODY_8_WSG, &bods->d8, true);
     loadWsg(DANCEBODY_9_WSG, &bods->d9, true);
     loadWsg(DANCEBODY_10_WSG, &bods->d10, true); */

    loadWsg(ARROW_18_WSG, &butts->arrow, true);

    bodsArray[0] = &bods->d1;
    bodsArray[1] = &bods->d2;
    bodsArray[2] = &bods->d3;
    bodsArray[3] = &bods->d4;
    bodsArray[4] = &bods->d5;
    bodsArray[5] = &bods->d6;
    bodsArray[6] = &bods->d7;
    bodsArray[7] = &bods->d8;
    bodsArray[8] = &bods->d9;
    bodsArray[9] = &bods->d10;

    miscArray[0] = &misc->bald;
    miscArray[1] = &misc->mainc;
    miscArray[2] = &misc->num1;
    miscArray[3] = &misc->num2;
    miscArray[4] = &misc->pomp;
    miscArray[5] = &misc->num3;
    miscArray[6] = &misc->cow;
    miscArray[7] = &misc->trophy;

    loadMidiFile(YALIKEJAZZ_MID, &amidi->bgm, true);
    printf("loaded midi file!\n");

    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    player->loop         = true;
    midiGmOn(player);
    globalMidiPlayerPlaySong(&amidi->bgm, MIDI_BGM);
    globalMidiPlayerSetVolume(MIDI_BGM, 12);
    printf("Entered Atrium Mode!\n");
    atriumTitle(); // draw the title screen
}

static void atriumExitMode()
{
        for (int i = 0; i < 10; i++)
    {
        freeWsg(bodsArray[i]);
        // freeWsg(miscArray[i]);
    }

    heap_caps_free(bgs);
    heap_caps_free(misc);
    heap_caps_free(bods);
    heap_caps_free(cards);
    heap_caps_free(amidi);
    heap_caps_free(butts);

    freeSwadgePasses(&spList);

    globalMidiPlayerStop(MIDI_BGM);
    unloadMidiFile(&amidi->bgm);
}

static void atriumMainLoop(int64_t elapsedUs)
{
    if (state == ATR_TITLE)
    {
        atriumTitle();
        while (checkButtonQueueWrapper(&evt))
        {
            if (evt.down && state == ATR_TITLE) // if the button is pressed down on the title screen

            {
                if ((evt.button & PB_A))
                {
                    state = ATR_ATR;
                    loader = 0;
                }
                else if ((evt.button & PB_B))
                {
                    state = ATR_PROFILE; // if B is pressed, go to profile view
                    loader = 0;
                }
                else
                {
                }
            }
        }
    }
    if (state == ATR_ATR) // if the state is atrium
    {
        sonaIdle();
    }
    else if (state == ATR_PROFILE) // if the state is title screen
    {
        viewProfile();
    }

    else if (state == ATR_EDIT_PROFILE) // if the state is edit profile
    {
        while (checkButtonQueueWrapper(&evt))
        {
            if (evt.down) // if the button is pressed

            {
                if ((evt.button & PB_RIGHT))
                {
                }
                else if ((evt.button & PB_LEFT))
                {
                }
            }
            else if ((evt.button & PB_UP))
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
            else if ((evt.button & PB_DOWN))
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
            else if ((evt.button & PB_A))
            {
                // select something
            }
            else if ((evt.button & PB_B))
            {
                state = ATR_PROFILE; // if B is pressed, go back to profile view
                loader = 0;
            }
            else
            {
            }
        }
    }
    editProfile(x, y);


printf("state: %d\n", state);
}
;

void sonaDraw()
{
    // tester to see if the sonas show up right. up to 4 sonas drawn on the screen at one time. eventually, random
    // swadgesona lines shall be selected to draw. in this tester, random hardcoded sonas are drawn in the required
    // positions.

    int j = 0; // placeholder for drawing a second row maybe someday. one struggle at a time

    for (int i = 0; i < 4; i++)
    {
        // int h = rand() % 4; //randomize head select
        // int h = 2; //for now, just use this head
        switch (i)
        {
            case 0:
                drawWsgSimple(&misc->bald, x_coords[i],
                              y_coords[j] - headoffset); // place head on top of body minus offset
                break;

            case 1:
                drawWsgSimple(&misc->pomp, x_coords[i], y_coords[j] - headoffset);
                break;

            case 2:
                drawWsgSimple(&misc->num1, x_coords[i], y_coords[j] - headoffset);
                break;

            case 3:
                drawWsgSimple(&misc->cow, x_coords[i], y_coords[j] - headoffset);
                break;

            case 4:
                drawWsgSimple(&misc->num2, x_coords[i], y_coords[j] - headoffset);
                break;
        }
    }

    // end sona tester
}

void sonaIdle()
{
    trophyUpdate(atriumTrophies[0], 1, 1); // trigger the trophy for entering the atrium

    if (loader == 0)
    {
        switch (PAGE)
        {
            case 0:
                freeWsg(&bgs->arcade1);
                freeWsg(&bgs->concert1);
                loadWsg(GAZEBO_WSG, &bgs->gazebo, true);
                loadWsg(ATRIUMPLANT_1_WSG, &bgs->plant1, true);
                loadWsg(ATRIUMPLANT_2_WSG, &bgs->plant2, true);
                drawWsgSimple(&bgs->gazebo, 0, 0); // draw the background based on page
                printf("Loaded GAZEBO_WSG!\n");
                break;

            case 1:
                freeWsg(&bgs->gazebo);
                freeWsg(&bgs->plant1);
                freeWsg(&bgs->plant2);
                freeWsg(&bgs->concert1);
                loadWsg(ARCADE_1_WSG, &bgs->arcade1, true);
                drawWsgSimple(&bgs->arcade1, 0, 0); // draw the background based on page

                break;

            case 2:
                freeWsg(&bgs->gazebo);
                freeWsg(&bgs->plant1);
                freeWsg(&bgs->plant2);
                freeWsg(&bgs->arcade1);
                loadWsg(CONCERT_1_WSG, &bgs->concert1, true);
                drawWsgSimple(&bgs->concert1, 0, 0); // draw the background based on page

                break;

            default:
                printf("All WSGs loaded or invalid bgloader value.\n");
                break;
        }
        loader = 1;
    }

    if (PAGE == 0 || PAGE == 1)
    {
        drawWsg(&butts->arrow, 260, 40, false, false, 90); // draw right arrow
    }
    if (PAGE == 1 || PAGE == 2)
    {
        drawWsg(&butts->arrow, 20, 40, true, false, 270); // draw left arrow
    }

    printf("Page: %d\n", PAGE);

    // drawWsgSimple(bgsArray[PAGE], 0, 0); //draw the background based on page
    sonaDraw();

    if ((PAGE == 0) & (planter <= 48))
    {                                                    // if on page 0, draw the plants rising up
        drawWsgSimple(&bgs->plant1, 0, 240 - planter);   // draw plant 1 rising into view
        drawWsgSimple(&bgs->plant2, 168, 240 - planter); // draw plant 2 rising into view
        planter++;
    }
    else if ((PAGE == 0) & (planter > 48))
    {                                               // if on page 0, draw the plants normally
        drawWsgSimple(&bgs->plant1, 0, 240 - 48);   // draw plant 1
        drawWsgSimple(&bgs->plant2, 168, 240 - 48); // draw plant 2
    }

    drawText(font, c000, "Press B to return to menu", 48, 216); // draw the text for selecting a card

    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down) // if the button is pressed

        {
            if ((evt.button & PB_A))
            {
            }
            else if ((evt.button & PB_B))
            {
                state = ATR_TITLE; // if B is pressed, go to profile view}
            }
            else if ((evt.button & PB_RIGHT))
            {
                if (PAGE < 2)
                {
                    PAGE++;     // next page
                    loader = 0; // reset the loader so the next background wsg can be loaded
                }
                else
                {
                    PAGE = 2; // cap at max page
                }
                if (PAGE == 0)
                {
                    planter = 0; // reset the planter so it can rise again
                }
            }
        }
        else if ((evt.button & PB_LEFT))
        {
            if (PAGE > 0)
            {
                PAGE--;     // previous page
                loader = 0; // reset the loader so the next background wsg can be loaded
                if (PAGE == 0)
                {
                    planter = 0; // reset the planter so it can rise again
                }
            }
        }
        else
        {
        }
    }

    if (loopcount == 0)
    { // if nobody's idling
        for (int i = 0; i < 4; i++)
        {
            drawWsgSimple(bodsArray[0], x_coords[i] + 1, y_coords[0]); // draw everybody
        }
        // decide if any of the sonas are gonna idle

        s = rand() % 4; // randomize which sona idles, 0-3
        d = 2;          // rand() % 1000; //randomize idle chance, 0-999

        if (d <= 1) // 1% chance to dance
        {
            // if the random number is less than or equal to 1, then a sona will idle
            printf("Dance count: %d\n", dancecount);
            dancecount = 1;
            loopcount  = 1;
            numloops   = rand() % 60 + 6; // randomize number of loops, 6-60 (the sonas like to dance)
        }
    }

    else
    { // if somebody's idling
        for (int i = 0; i < 4; i++)
        {
            if (i != s)
            {                                                              // if this sona is not the one idling
                drawWsgSimple(bodsArray[0], x_coords[i] + 1, y_coords[0]); // draw the other sonas
            }
        }

        if (loopcount <= numloops)
        {
            if (dancecount <= 9)
            { // if the dance count is less than or equal to 9, then keep dancing
                printf("Sona %d is dancing and the loop count is %d!\n", s, loopcount);

                // draw the remaining sona bodiess not dancing
                for (int i = 0; i < 4; i++)
                {
                    if (i != s)
                    { // if this sona is not the one dancing
                        drawWsgSimple(bodsArray[0], x_coords[i] + 1, y_coords[0]); // draw the other sonas
                    }
                }

                drawWsgSimple(bodsArray[dancecount], x_coords[s] + 1,
                              y_coords[0]); // draw the dancing sona body todo: add other idles
                printf("Sona %d has danced for %d frames!\n", s, dancecount);
                dancecount++;
            }

            else
            { // if the dance count is greater than 9, then stop dancing
                dancecount = 0;

                for (int i = 0; i < 4; i++)
                {
                    drawWsgSimple(bodsArray[0], x_coords[i] + 1, y_coords[0]); // draw everybody
                }

                loopcount++; // dance again nerd
            }
        }

        else
        { // numloops has been reached
            loopcount = 0;
            for (int i = 0; i < 4; i++)
            {
                drawWsgSimple(bodsArray[0], x_coords[i] + 1, y_coords[0]); // draw everybody
            }
        }
    }
}

void atriumTitle()
{
    state = ATR_TITLE; // set the state to title screen
    // draw the title screen for the atrium mode
    drawRectFilled(0, 0, 280, 240, c000); // fill the screen with black
    drawText(font, c555, "SwadgePass", 20, 20);
    drawText(font, c555, "Atrium", 20, 40);
    drawText(font, c555, "Press A to Enter the Atrium", 20, 100);
    drawText(font, c555, "Press B to View/Edit Your Profile", 20, 120);
}

void viewProfile()
{
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
                state = ATR_EDIT_PROFILE; // if A is pressed, go to edit profile view
            }
            else if ((evt.button & PB_B))
            {
                state  = ATR_TITLE; // if B is pressed, go back to title view
                loader = 0;         // reset the loader so the card wsgs are freed and reloaded next time
                for (int i = 0; i <= 6; i++)
                {
                    freeWsg(cardsArray[i]);
                    printf("freed card wsg %d\n", i);
                }
            }
            else
            {
            }
        }
    }

    // draw the card info
    drawWsgSimple(&bgs->gazebo, 0, 0);
    drawWsgSimple(cardsArray[testercardselect], 0, 0 + 12);                         // draw the next card
    drawWsgSimple(miscArray[testercardselect], card_coords_x[0], card_coords_y[0]); // draw the sona head
    drawText(font, c030, "Magfester69420", card_coords_x[1] + cardpadding,
             card_coords_y[0] + cardpadding); // draw the name
    drawText(font, c000, "Sandwich: Hot Dog", card_coords_x[1] + cardpadding,
             card_coords_y[0] + cardpadding + 12
                 + 4); // draw the sandwich info, needs extra padding to be balanced correctly
    drawText(font, c000, "I am a: Cosplayer", card_coords_x[1] + cardpadding,
             card_coords_y[0] + cardpadding + 24
                 + 4); // draw the identity, needs extra padding to be balanced correctly
    drawText(font, c000, "Class: Bard", card_coords_x[1] + cardpadding,
             card_coords_y[0] + cardpadding + 36 + 4); // draw the identity info
    drawText(font, c000, "Hello World!", card_coords_x[0] + cardpadding,
             card_coords_y[1] + cardpadding); // draw the second text box info
    drawText(font, c000, "This is 22 characters.", card_coords_x[0] + cardpadding,
             card_coords_y[1] + cardpadding + 12); // draw the second text box info
    drawText(font, c000, "Magfest is a: Donut", card_coords_x[0] + cardpadding,
             card_coords_y[1] + cardpadding + 24); // draw the second text box info
    drawWsgSimpleScaled(&misc->trophy, card_coords_x[2] + 7, card_coords_y[1] + 3, 1,
                        1); // draw the trophy to fill in the small box for testing
    drawText(font, c000, "Press A to Edit My Profile", 48, 200); // draw the text for selecting a card
    drawText(font, c000, "Press B to return to menu", 48, 216);  // draw the text for selecting a card
}

void atriumAddSP(struct swadgePassPacket* packet)
{
    packet->atriumMode.cardSelected = testercardselect; // for now, just set the card to the selected card
    packet->atriumMode.fact0        = 0;                // select fave sandwich
    packet->atriumMode.fact1        = 0;                // choose a class
    packet->atriumMode.fact2        = 0;                // why you're here
    packet->atriumMode.festers
        = sizeof(spList); // set the number of festers collected to the number of swadgepasses in the list
}

void editProfile(int xselect, int yselect)
{
    /* int card_coords_x[] = {24, 99, 208};
    int card_coords_y[] = {24+12, 112+12}; //needs testing, original location at 24,112 was too high on screen and
    clipping into radius int cardpadding = 4; //padding for box drawn around the card int textbox1width = 156; int
    textbox2width = 172;*/

    int textboxcoords_x[]  = {99, 24};             // x coords for the two textboxes
    int textbox1coords_y[] = {36, 48, 60, 72};     // y coords for the first textbox lines
    int textbox2coords_y[] = {124, 136, 148, 160}; // y coords for the second textbox lines

    drawTriangleOutlined(textboxcoords_x[x] - 10, textbox1coords_y[y], textboxcoords_x[x] - 10,
                         textbox1coords_y[y] + 11, textboxcoords_x[x], textbox1coords_y[y] + 5, c555, c500);
}