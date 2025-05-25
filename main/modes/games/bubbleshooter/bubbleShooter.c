// For debugging
#include <esp_log.h>

// For the game
#include "bubbleShooter.h"

#include "mainMenu.h"


//==============================================================================
// Defines
//==============================================================================

// UI
#define BUBBLE_SHOOTER_FPS 30


//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    BBS_MENU,
    BBS_GAME
} bubbleShooterScreen_t;

typedef struct 
{
    /* data */
    bubbleShooterScreen_t screen;
    
    menu_t* menu;
    menuManiaRenderer_t* menuRenderer;

    wsg_t sample;

} bubbleShooter_t;

//==============================================================================
// Function Declarations
//==============================================================================

static void bubbleShooterEnterMode(void);
static void bubbleShooterExitMode(void);
static void bubbleShooterMainLoop(int64_t elapsedUs);

static void bubbleShooterMenuCallback(const char* label, bool selected, uint32_t value);
static void bubbleShooterBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

//==============================================================================
// Constants
//==============================================================================
const char* BUB_TAG                     = "BUBBLE";
static const char bubbleShooterStrName[]  = "Bubble X Bubble";
static const char bubbleShooterStrPlay[]  = "Play";
static const char bubbleShooterStrExit[]  = "Exit";

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t bubbleShooterMode = {

    .modeName                 = bubbleShooterStrName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = bubbleShooterEnterMode,
    .fnExitMode               = bubbleShooterExitMode,
    .fnMainLoop               = bubbleShooterMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = bubbleShooterBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
    .fnAddToSwadgePassPacket  = NULL,
};

static bubbleShooter_t* bbs;


//==============================================================================
// Functions
//==============================================================================


static void bubbleShooterEnterMode(void)
{
    setFrameRateUs(1000000 / BUBBLE_SHOOTER_FPS);


    bbs = heap_caps_calloc(1, sizeof(bubbleShooter_t), MALLOC_CAP_8BIT);

    ESP_LOGI(BUB_TAG, "Hello world!");

    bbs->menu = initMenu(bubbleShooterStrName, bubbleShooterMenuCallback);
    addSingleItemToMenu(bbs->menu, bubbleShooterStrPlay);
    addSingleItemToMenu(bbs->menu, bubbleShooterStrExit);
    bbs->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);

    //Gotta learn to use this properly
    //loadWsg("kid0.wsg", &bbs->sample, true);
    //loadWsg("sample.wsg", &bbs->sample, true);
}

static void bubbleShooterMainLoop(int64_t elapsedUs)
{
    //Get button presses
    buttonEvt_t evts[42] = {0};
    uint8_t evtCount     = 0;
    buttonEvt_t evt      = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        evts[evtCount++] = evt;
        if (bbs->screen == BBS_MENU)
        {
            bbs->menu = menuButton(bbs->menu, evt);
        }
        else if (bbs->screen == BBS_GAME)
        {

        }
    }


    //Setup screen
    switch (bbs->screen)
    {
        case BBS_MENU:
        {
            drawMenuMania(bbs->menu, bbs->menuRenderer, elapsedUs);
            break;
        }
    }
}

static void bubbleShooterBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    switch (bbs->screen)
    {
        case BBS_GAME:
        {
            
            drawWsgSimple(&bbs->sample, 0, 0);
            break;
        }
        default:
        {
            break;
        }
    }
}

static void bubbleShooterMenuCallback(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if ((bubbleShooterStrPlay == label))
        {
            ESP_LOGI(BUB_TAG,"Bubble Bubble!");
            bbs->screen = BBS_GAME;
        }
        else if ((bubbleShooterStrExit == label))
        {            
            switchToSwadgeMode(&mainMenuMode); // Why don't we go back to games? 
        }
        
    }
}

static void bubbleShooterExitMode(void)
{
    //If the game is still active, destroy the game

    deinitMenuManiaRenderer(bbs->menuRenderer);
    deinitMenu(bbs->menu);

    //free the fonts
    heap_caps_free(bbs);
}