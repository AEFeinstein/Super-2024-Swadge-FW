#include "atrium.h"

/// @brief 
const char atriumModeName[] = "Atrium"; //idk working title

static void atriumEnterMode(void);
static void atriumExitMode(void);
static void atriumMainLoop(int64_t elapsedUs);
static void sonaDraw(void);
static void sonaIdle(void);
static void atriumTitle(void);
static void viewProfile(void);


//static void createProfile(void);
//static void viewProfile(void);

swadgeMode_t atriumMode = {
    .modeName                 = atriumModeName,  // Assign the name we created here
    .wifiMode                 = ESP_NOW,         // If we want WiFi 
    .overrideUsb              = false,           // Overrides the default USB behavior.
    .usesAccelerometer        = false,           // If we're using motion controls
    .usesThermometer          = false,           // If we're using the internal thermometer
    .overrideSelectBtn        = false,           // The select/Menu button has a default behavior. If you want to override it, you can set this to true but you'll need to re-implement the behavior.
    .fnEnterMode              = atriumEnterMode, // The enter mode function
    .fnExitMode               = atriumExitMode,  // The exit mode function
    .fnMainLoop               = atriumMainLoop,  // The loop function
    .fnAudioCallback          = NULL,            // If the mode uses the microphone
    .fnBackgroundDrawCallback = NULL,            // Draws a section of the display
    .fnEspNowRecvCb           = NULL,            // If using Wifi, add the receive function here
    .fnEspNowSendCb           = NULL,            // If using Wifi, add the send function here
    .fnAdvancedUSB            = NULL,            // If using advanced USB things.
};

typedef enum
{
    /// title
    ATR_TITLE,
    /// TITLE SCREEN
    ATR_ATR,
    /// GAZEBO
    ATR_PROFILE,
    /// LOOKING AT SELF PROFILE

} atriumState;

atriumState state = 0;

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
int ticker = 0;

//wsgs, fonts, etc


//Backgrounds 
typedef struct
{
    wsg_t gazebo;
} backgrounds_t;

backgrounds_t* bgs;

//bodies 
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
    int loopcount = 0; 
    int numloops = 0;
    int s = 0; //sona index, 0-3
    int d = 0; //dance chance

//cards
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

int testercardselect = 0; //for testing card selection, 0-6

int card_coords_x[] = {24, 99, 208};
int card_coords_y[] = {24, 112};
int cardpadding = 4; //padding for text written in card
int textbox1width = 156;
int textbox2width = 172;


//buttons
typedef struct 
{

} butts_t; //haha


//misc head sprites for testing
typedef struct
{
   wsg_t bald;
   wsg_t mainc;
   wsg_t num1;
   wsg_t num2;
   wsg_t pomp;
   wsg_t thurs;
   wsg_t cow;
   wsg_t trophy;

   midiFile_t bgm;          // BGM
} misc_t;

misc_t* misc;

buttonEvt_t evt;

//sona locations in plaza mode
        int x_coords[] = {5, 74, 143, 212};
        int y_coords[] = {120, 80}; 
        int headoffset = 48; 

//swadgepass
list_t spList = {0};

//trophies

    const trophyData_t atriumTrophies[] = {
        {
            .title       = "Welcome to the Atrium",
            .description = "Visit the Gazebo",
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


static void atriumEnterMode()
{
    bgs = (backgrounds_t*)heap_caps_calloc(1, sizeof(backgrounds_t), MALLOC_CAP_8BIT);
    bods = (bodies_t*)heap_caps_calloc(1, sizeof(bodies_t), MALLOC_CAP_8BIT);
    misc = (misc_t*)heap_caps_calloc(1, sizeof(misc_t), MALLOC_CAP_8BIT);
    cards = (cards_t*)heap_caps_calloc(1, sizeof(cards_t), MALLOC_CAP_8BIT);
    getSwadgePasses(&spList, &atriumMode, true);
    loadWsg(GAZEBO_WSG, &bgs->gazebo, true);

    //when its ready, I need to add sona data extraction from swadgepass packet, for now I am just hardcoding some sonas in to use:
    loadWsg(BALD_WSG, &misc->bald, true);
    loadWsg(MAINCHAR_WSG, &misc->mainc, true);
    loadWsg(NUM_1_WSG, &misc->num1, true);
    loadWsg(NUM_2_WSG, &misc->num2, true);
    loadWsg(THURSDAY_WSG, &misc->thurs, true);
    loadWsg(POMP_WSG, &misc->pomp, true);
    loadWsg(COW_WSG, &misc->cow, true);
    loadWsg(DANCEBODY_1_WSG, &bods->d1, true);
    loadWsg(DANCEBODY_2_WSG, &bods->d2, true);
    loadWsg(DANCEBODY_3_WSG, &bods->d3, true);
    loadWsg(DANCEBODY_4_WSG, &bods->d4, true);
    loadWsg(DANCEBODY_5_WSG, &bods->d5, true);
    loadWsg(DANCEBODY_6_WSG, &bods->d6, true);
    loadWsg(DANCEBODY_7_WSG, &bods->d7, true);
    loadWsg(DANCEBODY_8_WSG, &bods->d8, true);
    loadWsg(DANCEBODY_9_WSG, &bods->d9, true);
    loadWsg(DANCEBODY_10_WSG, &bods->d10, true);
    loadWsg(CARDBLOSS_WSG, &cards->card1, true);
    loadWsg(CARDBUBB_WSG, &cards->card2, true);
    loadWsg(CARDDINO_WSG, &cards->card3, true);
    loadWsg(CARDGEN_WSG, &cards->card4, true); 
    loadWsg(CARDMAGFEST_WSG, &cards->card5, true);
    loadWsg(CARDMUSIC_WSG, &cards->card6, true);
    loadWsg(CARDSPACE_WSG, &cards->card7, true);
    loadWsg(WINGED_TROPHY_WSG, &misc->trophy, true);
    

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

    cardsArray[0] = &cards->card1;
    cardsArray[1] = &cards->card2;
    cardsArray[2] = &cards->card3;
    cardsArray[3] = &cards->card4;  
    cardsArray[4] = &cards->card5;  
    cardsArray[5] = &cards->card6;  
    cardsArray[6] = &cards->card7;  

 


    //bgm
midiFile_t bgm;

loadMidiFile(YALIKEJAZZ_MID, &misc->bgm, true);

midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
player -> loop = true;
midiGmOn(player);
globalMidiPlayerPlaySong(&misc->bgm, MIDI_BGM);
globalMidiPlayerSetVolume(MIDI_BGM, 12);
    trophyUpdate(atriumTrophies[0], 1, 1); //trigger the trophy for entering the atrium
   };

static void atriumExitMode()
{
    freeWsg(&bgs->gazebo);
        freeWsg(&misc->bald);
        freeWsg(&misc->mainc);
        freeWsg(&misc->num1);
        freeWsg(&misc->cow);

        for (int i = 0; i < 10; i++) {
            freeWsg(bodsArray[i]);
        }

    heap_caps_free(bgs);
    heap_caps_free(misc);
    heap_caps_free(bods);

    freeSwadgePasses(&spList);	

    globalMidiPlayerStop(MIDI_BGM);
    unloadMidiFile(&misc->bgm);
    atriumTitle(); //draw the title screen
};

static void atriumMainLoop(int64_t elapsedUs)
{

if(state == ATR_TITLE) {
    atriumTitle();
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && state == ATR_TITLE) //if the button is pressed down on the title screen

        {
            if ((evt.button & PB_A))
            {
                state = ATR_ATR;
            }
            else if ((evt.button & PB_B))
            {
                state = ATR_PROFILE; //if B is pressed, go to profile view}
            }
            else 
            {

            }

        }
    }
}
if (state == ATR_ATR) //if the state is atrium
        {
            sonaIdle(); //draw the sonas
            drawText(getSysFont(), c000, "Page 1 of 1", 100, 200);
        }
else if (state == ATR_PROFILE) //if the state is title screen
        {
            viewProfile();
        }

        
        
};


void sonaDraw() {

drawWsgSimple(&bgs->gazebo, 0, 0);

//tester to see if the sonas show up right. up to 4 sonas drawn on the screen at one time. eventually, random swadgesona lines shall be selected to draw. in this tester, random hardcoded sonas are drawn in the required positions.

    int j = 0; //placeholder for drawing a second row maybe someday. one struggle at a time

    for (int i = 0; i <4; i++) {
    //int h = rand() % 4; //randomize head select
    //int h = 2; //for now, just use this head           
    switch (i) {
        case 0:
        drawWsgSimple(&misc->bald, x_coords[i], y_coords[j]-headoffset); //place head on top of body minus offset
        break;

        case 1:
        drawWsgSimple(&misc->pomp, x_coords[i], y_coords[j]-headoffset); 
        break;
        
        case 2:
        drawWsgSimple(&misc->num1, x_coords[i], y_coords[j]-headoffset); 
        break;
        
        case 3:
        drawWsgSimple(&misc->cow, x_coords[i], y_coords[j]-headoffset); 
        break;

        case 4:
        drawWsgSimple(&misc->num2, x_coords[i], y_coords[j]-headoffset); 
        break;
    
    }

    }

//end sona tester
};

void sonaIdle() {

sonaDraw();
    drawText(getSysFont(), c000, "Press B to return to menu", 48, 216); //draw the text for selecting a card

while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down) //if the button is pressed down on the title screen

        {
            if ((evt.button & PB_A))
            {
                
            }
            else if ((evt.button & PB_B))
            {
                state = ATR_TITLE; //if B is pressed, go to profile view}
            }
            else 
            {

            }

        }
    }

if (loopcount == 0) { //if nobody's idling
    for (int i = 0; i < 4; i++) {
    drawWsgSimple(bodsArray[0], x_coords[i]+1, y_coords[0]); //draw everybody

    }
    //decide if any of the sonas are gonna idle

    s = rand() % 4; //randomize which sona idles, 0-3
    d = rand() % 1000; //randomize idle chance, 0-999
   
    
    if (d <= 1) //1% chance to dance
        {      

    //if the random number is less than or equal to 1, then a sona will idle
    printf("Dance count: %d\n", dancecount);
    dancecount = 1;
    loopcount = 1;   
    numloops = rand() % 60 + 6; //randomize number of loops, 6-60 (the sonas like to dance)
        }

}

else { //if somebody's idling
        for (int i = 0; i < 4; i++) {
            if (i != s) { //if this sona is not the one idling
                drawWsgSimple(bodsArray[0], x_coords[i]+1, y_coords[0]); //draw the other sonas
            }
        }

        if(loopcount <= numloops)
        { 
            if (dancecount<= 9) { //if the dance count is less than or equal to 9, then keep dancing
            printf("Sona %d is dancing and the loop count is %d!\n", s,loopcount);
 
                //draw the remaining sona bodiess not dancing
                for (int i = 0; i < 4; i++) {
                    if (i != s) { //if this sona is not the one dancing
                        drawWsgSimple(bodsArray[0], x_coords[i]+1, y_coords[0]); //draw the other sonas
                    }
                }
         
                drawWsgSimple(bodsArray[dancecount], x_coords[s]+1, y_coords[0]); //draw the dancing sona body todo: add other idles
                printf("Sona %d has danced for %d frames!\n", s, dancecount);
                dancecount++;

            }
    
                else { //if the dance count is greater than 9, then stop dancing
                    dancecount = 0;
                   
                        for (int i = 0; i < 4; i++) {
                            drawWsgSimple(bodsArray[0], x_coords[i]+1, y_coords[0]); //draw everybody
                        }
                    
                    loopcount++; //dance again nerd
                    }
    }
    
        else {//numloops has been reached
            loopcount = 0;
            for (int i = 0; i < 4; i++) {
                    drawWsgSimple(bodsArray[0], x_coords[i]+1, y_coords[0]); //draw everybody
                }

        }
}


};

void atriumTitle() {
    state = ATR_TITLE; //set the state to title screen
    //draw the title screen for the atrium mode
    drawRectFilled(0, 0, 280, 240, c000); //fill the screen with black
    drawText(getSysFont(), c555, "SwadgePass", 20, 20);
    drawText(getSysFont(), c555, "Atrium", 20, 40);
    drawText(getSysFont(), c555,"Press A to Enter the Atrium", 20, 100);
    drawText(getSysFont(), c555,"Press B to View/Edit Your Profile", 20, 120);


};

void viewProfile() {

    //draw the card info
        while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down) //if the button is pressed down on the title screen

        {
            if ((evt.button & PB_A))
            {
                testercardselect = rand() % 7; //randomize the card selection, 0-6
            }
            else if ((evt.button & PB_B))
            {
                state = ATR_TITLE; //if B is pressed, go to profile view}
            }
            else 
            {

            }

        }
    }
    drawWsgSimple(&bgs->gazebo, 0, 0);
    drawWsgSimple(cardsArray[testercardselect], 0, 0); //draw the next card
    drawWsgSimple(&misc->bald, card_coords_x[0], card_coords_y[0]); //draw the sona head
    drawText(getSysFont(), c030, "Magfester69420", card_coords_x[1] + cardpadding, card_coords_y[0] + cardpadding); //draw the name
    drawText(getSysFont(), c000, "Sandwich: Hot Dog", card_coords_x[1] + cardpadding, card_coords_y[0] + cardpadding+12); //draw the sandwich info
    drawText(getSysFont(), c000, "I am a: Cosplayer", card_coords_x[1] + cardpadding, card_coords_y[0] + cardpadding+24); //draw the identity
    drawText(getSysFont(), c000, "Class: Bard", card_coords_x[1] + cardpadding, card_coords_y[0] + cardpadding+36); //draw the identity info
    drawText(getSysFont(), c000, "Hello World!", card_coords_x[0] + cardpadding, card_coords_y[1] + cardpadding); //draw the second text box info
    drawText(getSysFont(), c000, "This is 22 characters.", card_coords_x[0] + cardpadding, card_coords_y[1] + cardpadding+12); //draw the second text box info
    drawText(getSysFont(), c000, "Magfest is a: Donut", card_coords_x[0] + cardpadding, card_coords_y[1] + cardpadding+24); //draw the second text box info
    drawWsgSimpleScaled(&misc->trophy, card_coords_x[2]+7, card_coords_y[1]+3, 1, 1); //draw the trophy to fill in the small box for testing
    drawText(getSysFont(), c000, "Press A to select a card", 48, 200); //draw the text for selecting a card
    drawText(getSysFont(), c000, "Press B to return to menu", 48, 216); //draw the text for selecting a card
};