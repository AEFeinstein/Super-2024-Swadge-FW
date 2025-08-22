#include "atrium.h"

/// @brief 
const char atriumModeName[] = "Atrium"; //idk working title

static void atriumEnterMode(void);
static void atriumExitMode(void);
static void atriumMainLoop(int64_t elapsedUs);
static void sonaDraw(void);
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

//wsgs

//Backgrounds 
typedef struct
{
    wsg_t gazebo;
} backgrounds_t;

backgrounds_t* bgs;

//bodies 
typedef struct 
{
    wsg_t stick1;
    wsg_t body;
    wsg_t running;
    wsg_t sad;

 } bodies_t;

bodies_t* bods;

//cards
typedef struct 
{

} cards_t;

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
} misc_t;

misc_t* misc;

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
        .drawFromBottom   = false,
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
    getSwadgePasses(&spList, &atriumMode, true);
    //when its ready, I need to add sona data extraction from swadgepass packet, for now I am just hardcoding some sonas in to use:
    loadWsg(GAZEBO_WSG, &bgs->gazebo, true);

    loadWsg(BALD_WSG, &misc->bald, true);
    loadWsg(MAINCHAR_WSG, &misc->mainc, true);
    loadWsg(NUM_1_WSG, &misc->num1, true);
    loadWsg(NUM_2_WSG, &misc->num2, true);
    loadWsg(THURSDAY_WSG, &misc->thurs, true);
    loadWsg(POMP_WSG, &misc->pomp, true);
    loadWsg(COW_WSG, &misc->cow, true);
    loadWsg(STICK_1_WSG, &bods->stick1, true);
    loadWsg(BODY_WSG, &bods->body, true);
    loadWsg(RUNNING_WSG, &bods->running, true);
    loadWsg(SAD_WSG, &bods->sad, true);


    //draw initial background
    drawWsgSimple(&bgs->gazebo, 0, 0);
    sonaDraw();

   };

static void atriumExitMode()
{
    freeWsg(&bgs->gazebo);
        freeWsg(&misc->bald);
        freeWsg(&misc->mainc);
        freeWsg(&misc->num1);
        freeWsg(&misc->cow);
        freeWsg(&bods->stick1);
        freeWsg(&bods->body);
        freeWsg(&bods->running);
        freeWsg(&bods->sad);
        //i need to learn the fast way to do this...
    heap_caps_free(bgs);
    heap_caps_free(misc);
    heap_caps_free(bods);

    freeSwadgePasses(&spList);	
};

static void atriumMainLoop(int64_t elapsedUs)
{

};


void sonaDraw() {
//tester to see if the sonas show up right. up to 4 sonas drawn on the screen at one time. eventually, random swadgesona lines shall be selected to draw. in this tester, random hardcoded sonas are drawn in the required positions.
        int x_coords[] = {5, 74, 143, 212};
        int y_coords[] = {120, 80}; 
        int headoffset = 44; 
    int j = 0; //placeholder for drawing a second row maybe someday. one struggle at a time

    for (int i = 0; i <4; i++) {
    int h = rand() % 4; //randomize head select
    int b = rand() % 2; //randomize body select
    
        switch (b) {
        case 0:
        drawWsgSimple(&bods->sad, x_coords[i]+1, y_coords[j]); //place body, bodies don't seem 
        break;

        case 1:
        drawWsgSimple(&bods->body, x_coords[i]+1, y_coords[j]); 
        break;
        
        case 2:
        drawWsgSimple(&bods->running, x_coords[i]+1, y_coords[j]); 
        break;
        
    }
    
    switch (h) {
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