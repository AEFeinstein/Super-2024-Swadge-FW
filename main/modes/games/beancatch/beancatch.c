/**
 * @file beancatch.c
 * @author J.Vega (JVeg199X)
 * @brief Game & Swadge #1 - Bean Catch (JPV-MGSW-C01-BC)
 * @date 2026-09-18
 *
 */
 
//==================== ==========================================================
// Includes
//==============================================================================
#include "beancatch.h"
#include "bcTables.h"
#include <inttypes.h>
#include <stdbool.h>
#include "esp_random.h"

//==============================================================================
// Consts
//==============================================================================

const char beancatchModeName[]   = "Bean Catch";
const char beancatchNVSKey[] = "beancatch";

//==============================================================================
// Structs
//==============================================================================

typedef void (*gameUpdateFuncton_t)();

struct beancatch_t {
    int16_t btnState;
    int16_t prevBtnState;

    bc_gameStateEnum_t state;
    gameUpdateFuncton_t update;
    bool refreshScreen;
    wsg_t wsgs[BC_WSG_SIZE];

    uint8_t waitLoopCounter;
    uint8_t waitLoopMax;
    uint8_t emptyLoopCountdown;
    uint8_t idleConveyorCounter;

    uint8_t currentConveyor;

    uint8_t beans[4];
    uint8_t beanCount;
    uint8_t maxBeans;

    bc_conveyorIndex_t beanInDanger;
    bc_conveyorIndex_t droppedBeanLocation;

    bc_conveyorIndex_t playerPosition;
    
    bool eggyDevito;
    bool eggyDevitoed;

    uint8_t strikes;
    bool halfStrike;

    uint16_t score;
    uint16_t highScoreGameA;
    uint16_t highScoreGameB;
};

//==============================================================================
// Function declarations
//==============================================================================

static void beancatchEnterMode(void);
static void beancatchExitMode(void);
static void beancatchMainLoop(int64_t elapsedUs);

static void bcUpdateAcl(void);
static void bcUpdateClock(void);
static void bcUpdateGameA(void);
static void bcUpdateGameB(void);
static void bcUpdateGameOver(void);


//==============================================================================
// Variables
//==============================================================================

swadgeMode_t beancatchMode = {
    .modeName                = beancatchModeName,
    .wifiMode                = NO_WIFI,
    .overrideUsb             = false,
    .usesAccelerometer       = false,
    .usesThermometer         = false,
    .overrideSelectBtn       = false,
    .fnEnterMode             = beancatchEnterMode,
    .fnExitMode              = beancatchExitMode,
    .fnMainLoop              = beancatchMainLoop,
    .trophyData              = NULL,
    .fnAddToSwadgePassPacket = NULL,
};

beancatch_t* beancatch;

//==============================================================================
// Functions
//==============================================================================

void beancatchEnterMode(void)
{
    // Allocate mode memory
    beancatch = (beancatch_t*)heap_caps_calloc(1, sizeof(beancatch_t), MALLOC_CAP_8BIT);

    //beancatch->wsgs = heap_caps_calloc(BC_WSG_SIZE, sizeof(wsg_t), MALLOC_CAP_8BIT);

    for (uint16_t i = 0; i < BC_WSG_SIZE; i++)
    {
        loadWsg(BC_WSGS[i], &beancatch->wsgs[i], false);
    }

    beancatch->refreshScreen = true;
    beancatch->update = &bcUpdateAcl;
}

void beancatchExitMode(void)
{
     for (uint16_t i = 0; i < BC_WSG_SIZE; i++)
    {
        freeWsg(&beancatch->wsgs[i]);
    }
    heap_caps_free(beancatch);
}

void beancatchMainLoop(int64_t elapsedUs)
{
    // Check inputs
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        beancatch->btnState          = evt.state;

        // if (beancatch->update == &bcUpdateMainMenu)
        // {
        //     // Pass button events to the menu
        //     beancatch->menu = menuButton(beancatch->menu, evt);
        // }
    }

    beancatch->update();

    beancatch->prevBtnState          = beancatch->btnState;
}

void bcUpdateAcl(void)
{
    if(beancatch->refreshScreen)
    {
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c445);
        drawWsgSimple(&beancatch->wsgs[BC_WSG_BEAN_CATCH_BG], 0, 28);

        bc_LcdSegment_t segment;
        for (uint16_t i = 0; i < ARRAY_SIZE(BC_LCD_SEGMENTS); i++)
        {
            if(esp_random() % 2){ //testing segments for now...
                segment = BC_LCD_SEGMENTS[i];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
            }
        }

        beancatch->refreshScreen = false;
    }

    if( (esp_random() % 10) > 8){ //testing segments for now...
        beancatch->refreshScreen = true;
    }
}
