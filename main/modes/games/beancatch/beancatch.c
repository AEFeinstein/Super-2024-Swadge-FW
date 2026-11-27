/**
 * @file beancatch.c
 * @author J.Vega (JVeg199X)
 * @brief Game & Swadge #1 - Bean Catch (JPV-MGSW-C01-BC)
 * @date 2026-09-18
 *
 */

//==============================================================================
// Includes
//==============================================================================
#include "beancatch.h"
#include "bcTables.h"
#include <inttypes.h>
#include <stdbool.h>
#include "esp_random.h"
#include "esp_log.h"
#include "macros.h"
#include <time.h>
#include <sys/time.h>
#include "mainMenu.h"

//==============================================================================
// Consts
//==============================================================================

const char beancatchModeName[] = "Bean Catch";
const char beancatchNVSKey[]   = "beancatch";

const char beancatchStrGameA[] = "Game A (Easy)";
const char beancatchStrGameB[] = "Game B (Normal)";
const char beancatchStrClock[] = "Clock Mode";
const char beancatchStrExit[] = "Exit";

const char KEY_SCORE_PB_GAME_A[] = "bc_hiscore_pb_a";
const char KEY_SCORE_PB_GAME_B[] = "bc_hiscore_pb_b";

const trophyData_t bcTrophies[] = {
    {
        .title       = "Out of sight, out of mind",
        .description = "Drop a bean unseen",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
    },
    {
        .title       = "My Watch? Game &.",
        .description = "Start clock mode",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
    },
    {
        .title       = "Cool Beans",
        .description = "Start Game A",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
    },
    {
        .title       = "Bean Intern (unpaid)",
        .description = "Scored 200 Points in Game A",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 200,
    },
    {
        .title       = "Bean Candidate (Interview Round 7)",
        .description = "Scored 400 Points in Game A",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 400,
    },
    {
        .title       = "Jr. Bean Counter",
        .description = "Scored 600 Points in Game A",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 600,
    },
    {
        .title       = "Senior Jr. Bean Counter",
        .description = "Scored 800 Points in Game A",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 800,
    },
    {
        .title       = "Dr. Jr. Bean Counter",
        .description = "Scored 1000 Points in Game A",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1000,
    },
    {
        .title       = "How you BEAN?",
        .description = "Start Game B",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
    },
    {
        .title       = "Bean Contractor",
        .description = "Scored 200 Points in Game B",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 200,
    },
    {
        .title       = "Bean Employee (Part Time)",
        .description = "Scored 400 Points in Game B",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 400,
    },
    {
        .title       = "Bean Employee (Full Time, Hourly)",
        .description = "Scored 600 Points in Game B",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 600,
    },
    {
        .title       = "Bean Salaryman",
        .description = "Scored 800 Points in Game B",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 800,
    },
    {
        .title       = "Bean <four titles at once, same pay though>",
        .description = "Scored 1000 Points in Game B",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 1000,
    },
    {
        .title       = "John Bean Catch, CEO of Beans",
        .description = "All trophies awarded!",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 1,
    }
};

//==============================================================================
// Structs
//==============================================================================

typedef void (*gameUpdateFuncton_t)();

typedef struct
{
    int16_t btnState;
    int16_t prevBtnState;
    int16_t frameCounter;

    int64_t usCounter;
    uint8_t clockSeconds;
    uint8_t clockMinutes;
    uint8_t clockHours;
    bool clockSecondsPulse;
    bool clockHalfSecondsPulse;

    bc_gameStateEnum_t state;
    bc_gameStateEnum_t tempState;
    gameUpdateFuncton_t update;
    bool refreshScreen;

    font_t lcdNumbersFont;
    wsg_t wsgs[BC_WSG_SIZE];
    midiFile_t sounds[BC_SOUND_INDEX_MAX];

    bc_gameLevelIndex_t gameLevel;

    uint8_t waitLoopCounter;
    uint8_t waitLoopMax;
    uint8_t emptyLoopCountdown;
    uint8_t occupiedConveyorCount;

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

    menu_t* menu;
    menuZorldoRenderer_t* menuRenderer;
} beancatch_t;

//==============================================================================
// Function declarations
//==============================================================================

static void beancatchEnterMode(void);
static void beancatchExitMode(void);
static void beancatchMainLoop(int64_t elapsedUs);

static void bcPopulateMenu(void);
bool bcMenuCb(const char* label, bool selected, uint32_t value);

static void bcUpdateAcl(void);
static void bcUpdateClock(void);
static void bcUpdateGame(void);
static void bcUpdateMiss(void);
static void bcUpdateGameOver(void);
static void bcUpdatePause(void);
static void bcUpdateMainMenu(void);

static void bcDrawGame(void);
static void bcDrawScoreHud(uint16_t value);

static void bcClearBeans(void);
static bool bcIgnoreCurrentConveyorForGameA(void);
static void bcScorePoint(void);
static void bcNewGame(void);
static void bcChangeStateGameA(void);
static void bcChangeStateGameB(void);
static void bcChangeStateClock(void);
static void bcChangeStateMiss(void);
static void bcChangeStateGameOver(void);
static void bcChangeStatePause(void);
static void bcResumeTempState(void);

static void bcCheckPlayerCatchBean(void);


void* gameStateToUpdateFunctions[] = {
    &bcUpdateAcl,
    &bcUpdateAcl, 
    &bcUpdateClock, 
    &bcUpdateGame, 
    &bcUpdateGame, 
    &bcUpdateMiss, 
    &bcUpdateGameOver, 
    &bcUpdatePause
};

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

    // beancatch->wsgs = heap_caps_calloc(BC_WSG_SIZE, sizeof(wsg_t), MALLOC_CAP_8BIT);

    for (uint16_t i = 0; i < BC_WSG_SIZE; i++)
    {
        loadWsg(BC_WSGS[i], &beancatch->wsgs[i], false);
    }

    for (uint16_t i = 0; i < BC_SOUND_INDEX_MAX; i++)
    {
        loadMidiFile(BC_SOUND_MAP[i], &beancatch->sounds[i], false);
    }

    loadFont(LCD_NUMBERS_FONT, &beancatch->lcdNumbersFont, false);

    beancatch->usCounter = 0;
    beancatch->clockHours = 0;
    beancatch->clockMinutes = 0;
    beancatch->clockSeconds = 0;
    beancatch->clockSecondsPulse = false;
    beancatch->clockHalfSecondsPulse = false;

    beancatch->refreshScreen = true;

    beancatch->menuRenderer = initMenuZorldoRenderer(NULL, NULL);
    beancatch->menu = initMenu(beancatchModeName, bcMenuCb);
    bcPopulateMenu();

    size_t size = sizeof(uint16_t);
    // Load Game A high score
    if (false == readNvsBlob(KEY_SCORE_PB_GAME_A, &(beancatch->highScoreGameA), &(size)))
    {
        // Value didn't exist, so write the default
        beancatch->highScoreGameA = 0;
    }

    // Load Game B high score
    if (false == readNvsBlob(KEY_SCORE_PB_GAME_B, &(beancatch->highScoreGameB), &(size)))
    {
        // Value didn't exist, so write the default
        beancatch->highScoreGameB = 0;
    }

    beancatch->state = BC_ST_ACL;
    beancatch->tempState = BC_ST_NULL;

    beancatch->update = &bcUpdateMainMenu;
}

void beancatchExitMode(void)
{
    for (uint16_t i = 0; i < BC_WSG_SIZE; i++)
    {
        freeWsg(&beancatch->wsgs[i]);
    }

    for (uint16_t i = 0; i < BC_SOUND_INDEX_MAX; i++)
    {
        unloadMidiFile(&beancatch->sounds[i]);
    }

    freeFont(&beancatch->lcdNumbersFont);
    deinitMenuZorldoRenderer(beancatch->menuRenderer);
    deinitMenu(beancatch->menu);

    heap_caps_free(beancatch);
}

void beancatchMainLoop(int64_t elapsedUs)
{
    // Check inputs
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        beancatch->btnState = evt.state;

        if (beancatch->update == &bcUpdateMainMenu)
        {
            // Pass button events to the menu
            beancatch->menu = menuButton(beancatch->menu, evt);
        }
    }

    beancatch->update();

    beancatch->clockSecondsPulse = false;
    beancatch->usCounter += elapsedUs;

    if(beancatch->usCounter == 500000)
    {
        beancatch->clockHalfSecondsPulse = true;
    }
    else if(beancatch->usCounter > 999999)
    {
        beancatch->usCounter -= 1000000;
        beancatch->clockSecondsPulse = true;
        beancatch->clockHalfSecondsPulse = true;
    }

    // Get the current time
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);

    time_t epochSec   = tv.tv_sec;
    struct tm* tmTime = localtime(&epochSec);

    beancatch->clockSeconds = tmTime->tm_sec;
    beancatch->clockMinutes = tmTime->tm_min;
    beancatch->clockHours = tmTime->tm_hour;

    beancatch->prevBtnState = beancatch->btnState;
}

void bcPopulateMenu(void)
{
    removeAllItemsFromMenu(beancatch->menu);

    addSingleItemToMenu(beancatch->menu, beancatchStrGameB);
    addSingleItemToMenu(beancatch->menu, beancatchStrGameA);
    addSingleItemToMenu(beancatch->menu, beancatchStrClock);
    addSingleItemToMenu(beancatch->menu, beancatchStrExit);
}

bool bcMenuCb(const char* label, bool selected, uint32_t value)
{
    if(selected)
    {
        if(label == beancatchStrGameB)
        {
            bcChangeStateGameB();
        }
        else if(label == beancatchStrGameA)
        {
            bcChangeStateGameA();
        }
        else if(label == beancatchStrClock)
        {
            bcChangeStateClock();
        }
        else if(label == beancatchStrExit)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }

    return false;
}

void bcUpdateAcl(void)
{
    if (beancatch->refreshScreen)
    {
        // Unit outer bezel
        fillDisplayArea(0, 0, TFT_WIDTH, 21, c540);
        fillDisplayArea(0, 221, TFT_WIDTH, TFT_HEIGHT, c540);
        drawWsgSimple(&beancatch->wsgs[BC_WSG_TOP_LABEL], 0, 5);
        drawWsgSimple(&beancatch->wsgs[BC_WSG_BOTTOM_LBL], 0, 226);

        // Unit inner bezel
        fillDisplayArea(0, 21, TFT_WIDTH, 28, c510);
        fillDisplayArea(0, 212, TFT_WIDTH, 221, c510);

        // Reflector
        fillDisplayArea(0, 36, TFT_WIDTH, 205, c445);

        drawWsgSimple(&beancatch->wsgs[BC_WSG_BEAN_CATCH_BG], 0, 28);

        bc_LcdSegment_t segment;
        for (uint16_t i = 0; i < ARRAY_SIZE(BC_LCD_SEGMENTS); i++)
        {
            segment = BC_LCD_SEGMENTS[i];
            drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
        }

        char buffer[5];

        snprintf(buffer, sizeof(buffer) - 1, "%2d", 18 /*esp_random() % 18*/);
        drawText(&beancatch->lcdNumbersFont, c000, buffer, 25, 53);

        snprintf(buffer, sizeof(buffer) - 1, "%02d", 88 /*esp_random() % 18*/);
        drawText(&beancatch->lcdNumbersFont, c000, buffer, 68, 53);

        beancatch->refreshScreen = false;
    }

    beancatch->frameCounter++;
    if (beancatch->frameCounter > 59)
    {
        beancatch->frameCounter = 0;
        bcChangeStateClock();
    }
}

void bcUpdateClock(void)
{
    //Uncomment for classic UX
    /*
        if (!(beancatch->prevBtnState & PB_A) && (beancatch->btnState & PB_A))
        {
            beancatch->refreshScreen = true;
        }

        if (!(beancatch->prevBtnState & PB_B) && (beancatch->btnState & PB_B))
        {
            beancatch->refreshScreen = true;
        }
        if ((beancatch->prevBtnState & PB_A) && !(beancatch->btnState & PB_A))
        {
            bcChangeStateGameA();
            return;
        }
        if ((beancatch->prevBtnState & PB_B) && !(beancatch->btnState & PB_B))
        {
            bcChangeStateGameB();
            return;
        }
    */

    if (!(beancatch->prevBtnState & PB_B) && (beancatch->btnState & PB_B))
    {
        beancatch->update = &bcUpdateMainMenu;
    }

    if (beancatch->clockSecondsPulse)
    {
        beancatch->waitLoopCounter = 200;
        beancatch->refreshScreen = true;
        bcUpdateGame();
    }

    bcDrawGame();
}

void bcUpdateGame(void)
{
    beancatch->waitLoopCounter++;

    if(beancatch->waitLoopCounter > (beancatch->waitLoopMax >> (beancatch->occupiedConveyorCount > 1)) )
    {
        if(beancatch->beanInDanger > -1)
        {
            bcChangeStateMiss();
        }
        else
        {
            //handle beans
            for(uint8_t i = 0; i < BC_CONVEYOR_SIZE; i++)
            {
                if(beancatch->beans[beancatch->currentConveyor] & 0b10000)
                {
                    beancatch->beanInDanger = beancatch->currentConveyor;
                }

                //move bean
                beancatch->beans[beancatch->currentConveyor] = beancatch->beans[beancatch->currentConveyor] << 1;
                
                //decide whether to add a new bean
                if(
                    !bcIgnoreCurrentConveyorForGameA() //If we haven't chosen to ignore this conveyor for Game
                    && 
                    !(beancatch->beans[beancatch->currentConveyor] & 0b10) //If there is not a bean in the second position (we just moved it out of the first one)
                    &&
                    beancatch->beanCount < beancatch->maxBeans //If beans are not maxed out according to the current difficulty scale
                    &&
                    ( 
                        ((esp_random() % 10) > 5) //Random yes/no decision
                        || 
                        !beancatch->beanCount    //Force to yes if there are no beans on the playfield 
                    )

                )
                {
                    if (!beancatch->beans[beancatch->currentConveyor])
                    {
                        beancatch->occupiedConveyorCount++;
                    }

                    beancatch->beans[beancatch->currentConveyor]++;
                    beancatch->beanCount++;
                    
                    beancatch->refreshScreen = true;
                }

                //Did we actually move a bean?
                if (beancatch->beans[beancatch->currentConveyor] > 0)
                {
                    bcCheckPlayerCatchBean();

                    if(beancatch->state != BC_ST_CLOCK)
                    {
                        globalMidiPlayerPlaySong(&beancatch->sounds[beancatch->currentConveyor], MIDI_SFX);
                    } 
                    else if(beancatch->beans[beancatch->currentConveyor] & 0b100000)
                    {
                        beancatch->playerPosition = beancatch->currentConveyor;
                        bcCheckPlayerCatchBean();
                    }

                    beancatch->waitLoopCounter=0;
                    beancatch->refreshScreen = true;
                    beancatch->currentConveyor = (beancatch->currentConveyor + 1) % 4;
                    break;
                }

                //If not, try the next conveyor
                beancatch->currentConveyor = (beancatch->currentConveyor + 1) % 4;
            }
        }
    }
    
    if(beancatch->state != BC_ST_CLOCK)
    {
        if((beancatch->btnState & PB_START) && !(beancatch->prevBtnState & PB_START))
        {
            bcChangeStatePause();
            return;
        }
        
        switch (beancatch->playerPosition)
        {
            case BC_CONVEYOR_LU:
                if((beancatch->btnState & PB_RIGHT) && !(beancatch->btnState & PB_LEFT))
                {
                    beancatch->playerPosition = BC_CONVEYOR_RU;
                    bcCheckPlayerCatchBean();
                    beancatch->refreshScreen = true;
                } else if((beancatch->btnState & PB_DOWN) && !(beancatch->btnState & PB_UP))
                {
                    beancatch->playerPosition = BC_CONVEYOR_LD;
                    bcCheckPlayerCatchBean();
                    beancatch->refreshScreen = true;
                }
                break;
            case BC_CONVEYOR_LD:
                if((beancatch->btnState & PB_RIGHT) && !(beancatch->btnState & PB_LEFT))
                {
                    beancatch->playerPosition = BC_CONVEYOR_RD;
                    bcCheckPlayerCatchBean();
                    beancatch->refreshScreen = true;
                } else if((beancatch->btnState & PB_UP) && !(beancatch->btnState & PB_DOWN))
                {
                    beancatch->playerPosition = BC_CONVEYOR_LU;
                    bcCheckPlayerCatchBean();
                    beancatch->refreshScreen = true;
                }
                break;
            case BC_CONVEYOR_RU:
                if((beancatch->btnState & PB_LEFT) && !(beancatch->btnState & PB_RIGHT))
                {
                    beancatch->playerPosition = BC_CONVEYOR_LU;
                    bcCheckPlayerCatchBean();
                    beancatch->refreshScreen = true;
                } else if((beancatch->btnState & PB_DOWN) && !(beancatch->btnState & PB_UP))
                {
                    beancatch->playerPosition = BC_CONVEYOR_RD;
                    bcCheckPlayerCatchBean();
                    beancatch->refreshScreen = true;
                }
                break;
            case BC_CONVEYOR_RD:
                if((beancatch->btnState & PB_LEFT) && !(beancatch->btnState & PB_RIGHT))
                {
                    beancatch->playerPosition = BC_CONVEYOR_LD;
                    bcCheckPlayerCatchBean();
                    beancatch->refreshScreen = true;
                } else if((beancatch->btnState & PB_UP) && !(beancatch->btnState & PB_DOWN))
                {
                    beancatch->playerPosition = BC_CONVEYOR_RU;
                    bcCheckPlayerCatchBean();
                    beancatch->refreshScreen = true;
                }
                break;
            
            default:
                break;
        }
    }

    if(beancatch->clockHalfSecondsPulse && beancatch->halfStrike)
    {
        beancatch->refreshScreen = true;
    }

    if(beancatch->clockSecondsPulse)
    {
        if( !beancatch->eggyDevito && ((beancatch->clockSeconds % 10) > 5) )
        {
            beancatch->eggyDevito = true;
            beancatch->refreshScreen = true;
        } 
        else if( beancatch->eggyDevito && !((beancatch->clockSeconds % 10) > 5) )
        {
            beancatch->eggyDevito = false;
            beancatch->refreshScreen = true;
        } 
    }       

    bcDrawGame();
}

void bcUpdateMiss(void)
{
    beancatch->frameCounter++;
    
    if(beancatch->frameCounter < 0)
    {
        if(!(beancatch->frameCounter % 6))
        {
            beancatch->eggyDevito = !beancatch->eggyDevito;
            beancatch->refreshScreen = true;
            globalMidiPlayerPlaySong(&beancatch->sounds[beancatch->currentConveyor], MIDI_SFX);
        }
    } else if(beancatch->frameCounter == 0)
    {
        beancatch->refreshScreen = true;
    }

    if(beancatch->frameCounter > 60)
    {
        beancatch->droppedBeanLocation = BC_CONVEYOR_NULL;

        if(!beancatch->eggyDevitoed)
        {
            if(beancatch->halfStrike)
            {
                beancatch->halfStrike = false;
                beancatch->refreshScreen = true;
            }
            else
            {
                beancatch->strikes++;
                beancatch->halfStrike = true;
                beancatch->refreshScreen = true;
            }
        }
        else
        {
            if(beancatch->strikes >= 3 && beancatch->halfStrike)
            {
                beancatch->halfStrike = false;
                beancatch->refreshScreen = true;
            }
            else
            {
                beancatch->strikes++;
                beancatch->refreshScreen = true;
            }
        }

        if(beancatch->strikes >= 3 && !beancatch->halfStrike)
        {
            //Game Over
            bcChangeStateGameOver();
            return;
        }

        bcResumeTempState();
    }

    bcDrawGame();
}

void bcUpdateGameOver(void)
{
    beancatch->frameCounter++;

    if(beancatch->frameCounter < 60)
    {
        globalMidiPlayerPlaySong(&beancatch->sounds[beancatch->frameCounter & 0b11], MIDI_SFX);
    }

    if(beancatch->frameCounter > 18000) // Automatically return to clock after 5 minutes
    {
        beancatch->update = &bcUpdateMainMenu;
        return;
    }

    if ((beancatch->prevBtnState & PB_A) && !(beancatch->btnState & PB_A))
    {
        beancatch->update = &bcUpdateMainMenu;
        return;
    }
    
    if ((beancatch->prevBtnState & PB_B) && !(beancatch->btnState & PB_B))
    {
        beancatch->update = &bcUpdateMainMenu;
        return;
    }

    bcDrawGame();
}

void bcUpdatePause(void)
{
    if((beancatch->btnState & PB_START) && !(beancatch->prevBtnState & PB_START))
    {
        beancatch->refreshScreen = true;
        bcResumeTempState();
    }

    bcDrawGame();
}

void bcUpdateMainMenu(void)
{
    drawMenuZorldo(beancatch->menu, beancatch->menuRenderer, 16000);

    char scoreBuffer[13l];

    snprintf(scoreBuffer, sizeof(scoreBuffer) - 1, "Best: %5d", beancatch->highScoreGameB);

    drawText(beancatch->menuRenderer->menuFont, c000, scoreBuffer, 171, 61);
    drawText(beancatch->menuRenderer->menuFont, c000, scoreBuffer, 173, 61);
    drawText(beancatch->menuRenderer->menuFont, c000, scoreBuffer, 172, 60);
    drawText(beancatch->menuRenderer->menuFont, c000, scoreBuffer, 172, 62);
    drawText(beancatch->menuRenderer->menuFont, c555, scoreBuffer, 172, 61);

    snprintf(scoreBuffer, sizeof(scoreBuffer) - 1, "Best: %5d", beancatch->highScoreGameA);

    drawText(beancatch->menuRenderer->menuFont, c000, scoreBuffer, 171, 93);
    drawText(beancatch->menuRenderer->menuFont, c000, scoreBuffer, 173, 93);
    drawText(beancatch->menuRenderer->menuFont, c000, scoreBuffer, 172, 92);
    drawText(beancatch->menuRenderer->menuFont, c000, scoreBuffer, 172, 94);
    drawText(beancatch->menuRenderer->menuFont, c555, scoreBuffer, 172, 93);
}

void bcDrawGame(void)
{
    if(beancatch->refreshScreen)
    {
        //Unit outer bezel
        fillDisplayArea(0, 0, TFT_WIDTH, 21, c540);
        fillDisplayArea(0, 221, TFT_WIDTH, TFT_HEIGHT, c540);
        drawWsgSimple(&beancatch->wsgs[BC_WSG_TOP_LABEL], 0, 5);
        drawWsgSimple(&beancatch->wsgs[BC_WSG_BOTTOM_LBL], 0, 226);

        // Unit inner bezel
        fillDisplayArea(0, 21, TFT_WIDTH, 28, c510);
        fillDisplayArea(0, 212, TFT_WIDTH, 221, c510);

        // Reflector
        fillDisplayArea(0, 36, TFT_WIDTH, 205, c445);

        drawWsgSimple(&beancatch->wsgs[BC_WSG_BEAN_CATCH_BG], 0, 28);

        bc_LcdSegment_t segment;

        for (uint16_t i = 0; i < 5; i++)
        {
            if ((beancatch->beans[BC_CONVEYOR_LU] >> i) & 0b1)
            {
                segment = BC_LCD_SEGMENTS[BC_SEG_BEAN_UL_00 + i];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
            }

            if ((beancatch->beans[BC_CONVEYOR_LD] >> i) & 0b1)
            {
                segment = BC_LCD_SEGMENTS[BC_SEG_BEAN_DL_00 + i];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
            }

            if ((beancatch->beans[BC_CONVEYOR_RU] >> i) & 0b1)
            {
                segment = BC_LCD_SEGMENTS[BC_SEG_BEAN_UR_00 + i];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
            }

            if ((beancatch->beans[BC_CONVEYOR_RD] >> i) & 0b1)
            {
                segment = BC_LCD_SEGMENTS[BC_SEG_BEAN_DR_00 + i];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
            }
        }

        switch (beancatch->playerPosition)
        {
            case BC_CONVEYOR_LU:

                segment = BC_LCD_SEGMENTS[BC_SEG_PLAYER_UL];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                segment = BC_LCD_SEGMENTS[BC_SEG_BEANBERT_L];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                break;
            case BC_CONVEYOR_LD:

                segment = BC_LCD_SEGMENTS[BC_SEG_PLAYER_DL];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                segment = BC_LCD_SEGMENTS[BC_SEG_BEANBERT_L];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);

                break;
            case BC_CONVEYOR_RU:

                segment = BC_LCD_SEGMENTS[BC_SEG_PLAYER_UR];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                segment = BC_LCD_SEGMENTS[BC_SEG_BEANBERT_R];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);

                break;
            case BC_CONVEYOR_RD:

                segment = BC_LCD_SEGMENTS[BC_SEG_PLAYER_DR];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                segment = BC_LCD_SEGMENTS[BC_SEG_BEANBERT_R];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);

                break;
            default:
                break;
        }

        if (beancatch->strikes)
        {
            segment = BC_LCD_SEGMENTS[BC_SEG_STRIKE_LBL];
            drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
        }

        for (uint16_t i = 0; i < beancatch->strikes; i++)
        {
            if (beancatch->halfStrike && i == beancatch->strikes - 1 && (beancatch->usCounter > 500000))
            {
                break;
            }

            segment = BC_LCD_SEGMENTS[BC_SEG_STRIKE_ICON_00 + i];
            drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
        }

        switch(beancatch->state)
        {
            case BC_ST_GAME:
                segment = (beancatch->gameLevel == BC_GAME_LEVEL_A) ? BC_LCD_SEGMENTS[BC_SEG_GAME_A_LBL] : BC_LCD_SEGMENTS[BC_SEG_GAME_B_LBL];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);

                bcDrawScoreHud(beancatch->score);

                break;
            case BC_ST_GAME_OVER:
                segment = (beancatch->gameLevel == BC_GAME_LEVEL_A) ? BC_LCD_SEGMENTS[BC_SEG_GAME_A_LBL] : BC_LCD_SEGMENTS[BC_SEG_GAME_B_LBL];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);

                bcDrawScoreHud(beancatch->score);

                switch(beancatch->playerPosition)
                {
                    case BC_CONVEYOR_LU:
                    case BC_CONVEYOR_LD:
                    default:
                        segment = BC_LCD_SEGMENTS[BC_SEG_BUBBLE_PTR_L];
                        drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                        break;
                    case BC_CONVEYOR_RU:
                    case BC_CONVEYOR_RD:
                        segment = BC_LCD_SEGMENTS[BC_SEG_BUBBLE_PTR_R];
                        drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                        break;
                }
                segment = BC_LCD_SEGMENTS[BC_SEG_SWEARING];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                break;
            case BC_ST_MISS:
                segment = (beancatch->gameLevel == BC_GAME_LEVEL_A) ? BC_LCD_SEGMENTS[BC_SEG_GAME_A_LBL] : BC_LCD_SEGMENTS[BC_SEG_GAME_B_LBL];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);

                bcDrawScoreHud(beancatch->score);

                if(beancatch->frameCounter >= 0 && (beancatch->eggyDevitoed || beancatch->strikes >= 3))
                {
                    switch(beancatch->playerPosition)
                    {
                        case BC_CONVEYOR_LU:
                        case BC_CONVEYOR_LD:
                        default:
                            segment = BC_LCD_SEGMENTS[BC_SEG_BUBBLE_PTR_L];
                            drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                            break;
                        case BC_CONVEYOR_RU:
                        case BC_CONVEYOR_RD:
                            segment = BC_LCD_SEGMENTS[BC_SEG_BUBBLE_PTR_R];
                            drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                            break;
                    }
                    
                    segment = BC_LCD_SEGMENTS[BC_SEG_SWEARING];
                    drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                }

                break;
            case BC_ST_CLOCK:
                //Uncomment for classic UX
                /*if(beancatch->btnState & PB_A)
                {
                    segment = BC_LCD_SEGMENTS[BC_SEG_GAME_A_LBL];
                    drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);

                    bcDrawScoreHud(beancatch->highScoreGameA);
                }
                else if(beancatch->btnState & PB_B)
                {
                    segment = BC_LCD_SEGMENTS[BC_SEG_GAME_B_LBL];
                    drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                    
                    bcDrawScoreHud(beancatch->highScoreGameB);
                } 
                else*/
                {
                    //draw clock
                    segment = BC_LCD_SEGMENTS[BC_SEG_HOURS_SEP];
                    drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);

                    if(beancatch->clockHours < 12)
                    {
                        segment = BC_LCD_SEGMENTS[BC_SEG_AM_LABEL];
                        drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                    }

                    bcDrawScoreHud( (( (beancatch->clockHours == 0 || beancatch->clockHours == 12) ? 12 : beancatch->clockHours % 12)) * 100 + beancatch->clockMinutes );
                }
                break;
            case BC_ST_PAUSE:
                drawRectFilled(0, 96, TFT_WIDTH, 150, c131);
                drawText(beancatch->menuRenderer->titleFont, c000, "- PAUSE -", 52, 110);
                drawText(beancatch->menuRenderer->titleFont, c555, "- PAUSE -", 50, 107);
                break;
            default:
                break;
        }

        if (beancatch->eggyDevito)
        {
            segment = BC_LCD_SEGMENTS[BC_SEG_EGGY_DEVITO];
            drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
        }

        switch (beancatch->droppedBeanLocation)
        {
            case BC_CONVEYOR_NULL:
            default:
                break;
            case BC_CONVEYOR_LU:
            case BC_CONVEYOR_LD:
                segment = BC_LCD_SEGMENTS[BC_SEG_BEAN_DROP_L];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                break;
            case BC_CONVEYOR_RU:
            case BC_CONVEYOR_RD:
                segment = BC_LCD_SEGMENTS[BC_SEG_BEAN_DROP_R];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
                break;
        }

        beancatch->refreshScreen = false;
    }
}

void bcDrawScoreHud(uint16_t value)
{
    char scoreBuffer[7];
    char digitBuffer[3];

    snprintf(scoreBuffer, sizeof(scoreBuffer) - 1, "%4d", (value % 2000));

    if (value > 99)
    {
        digitBuffer[0] = scoreBuffer[0];
        digitBuffer[1] = scoreBuffer[1];
        digitBuffer[2] = '\0';
        drawText(&beancatch->lcdNumbersFont, c000, digitBuffer, 25, 53);
    }

    digitBuffer[0] = scoreBuffer[2];
    digitBuffer[1] = scoreBuffer[3];
    digitBuffer[2] = '\0';
    drawText(&beancatch->lcdNumbersFont, c000, digitBuffer, 68, 53);

    /*
    //Show game speed for testing
    snprintf(scoreBuffer, sizeof(scoreBuffer) - 1, "%4d", beancatch->waitLoopMax);
    drawText(&beancatch->lcdNumbersFont, c000, scoreBuffer, 16, 180);
    */
}

void bcClearBeans(void)
{
    for (uint16_t i = 0; i < 4; i++)
    {
        beancatch->beans[i] = 0;
    }

    beancatch->beanCount             = 0;
    beancatch->occupiedConveyorCount = 0;

    beancatch->refreshScreen = true;
}

bool bcIgnoreCurrentConveyorForGameA(void)
{
    if(beancatch->gameLevel != BC_GAME_LEVEL_A) 
    {
        return false;
    }

    return (beancatch->currentConveyor == ((beancatch->strikes + 2) % 4));
}

void bcScorePoint(void)
{
    uint16_t previousScore = beancatch->score;
    beancatch->score++;

    uint16_t previousScoreHundredsDigit = (previousScore / 100) % 10;
    uint16_t previousScoreTensDigit = (previousScore / 10) % 10;
    uint16_t scoreHundredsDigit = (beancatch->score / 100) % 10;
    uint16_t scoreTensDigit = (beancatch->score / 10) % 10;

    if(previousScoreHundredsDigit != scoreHundredsDigit && scoreHundredsDigit != 9)
    {
        beancatch->waitLoopMax += 7;
    } 
    else if (previousScoreTensDigit != scoreTensDigit)
    {
        beancatch->waitLoopMax--;
        if(beancatch->waitLoopMax < 6)
        {
            beancatch->waitLoopMax = 6;
        }
    }

    if(beancatch->score < 5)
    {
        beancatch->maxBeans = 1;
    } else {
        beancatch->maxBeans = BC_DIFFICULTY_MAX_BEANS[CLAMP(scoreHundredsDigit + scoreTensDigit, 0, BC_DIFFICULTY_MAX_BEANS_SIZE - 1)];
    }
    
    switch(beancatch->gameLevel)
    {
        case BC_GAME_LEVEL_A:
            if(beancatch->score > beancatch->highScoreGameA)
            {
                beancatch->highScoreGameA = beancatch->score;
            }
            break;
        case BC_GAME_LEVEL_B:
            if (beancatch->score > beancatch->highScoreGameB)
            {
                beancatch->highScoreGameB = beancatch->score;
            }
            break;
        default:
            break;
    }

    beancatch->refreshScreen = true;
}

void bcNewGame(void)
{
    bcClearBeans();
    beancatch->droppedBeanLocation = BC_CONVEYOR_NULL;
    beancatch->score = 0;
    beancatch->strikes = 0;
    beancatch->halfStrike = false;
    beancatch->maxBeans = 1;
    beancatch->currentConveyor = esp_random() % 4;
    beancatch->waitLoopCounter = 0;
    beancatch->beanInDanger = BC_CONVEYOR_NULL;
    beancatch->emptyLoopCountdown = 0;
    beancatch->eggyDevitoed = false;
    
    beancatch->refreshScreen = true;
}

void bcChangeStateGameA(void)
{
    beancatch->gameLevel = BC_GAME_LEVEL_A;
    beancatch->state = BC_ST_GAME;
    bcNewGame();
    beancatch->waitLoopMax = 37;
    beancatch->update = &bcUpdateGame;
}

void bcChangeStateGameB(void)
{
    beancatch->gameLevel = BC_GAME_LEVEL_B;
    beancatch->state = BC_ST_GAME;
    bcNewGame();
    beancatch->waitLoopMax = 31;
    beancatch->update = &bcUpdateGame;
}

void bcChangeStateClock(void)
{
    beancatch->gameLevel = BC_GAME_LEVEL_NULL;
    beancatch->strikes = 0;
    beancatch->state = BC_ST_CLOCK;
    bcNewGame();

    beancatch->maxBeans = 8;
    beancatch->refreshScreen = true;
    beancatch->update = &bcUpdateClock;
}

void bcChangeStateMiss(void)
{
    beancatch->tempState = beancatch->state;
    beancatch->state = BC_ST_MISS;

    beancatch->droppedBeanLocation = beancatch->beanInDanger;
    beancatch->beanInDanger = BC_CONVEYOR_NULL;
    beancatch->waitLoopCounter = 0;

    globalMidiPlayerPlaySong(&beancatch->sounds[BC_SOUND_STRIKE], MIDI_SFX);
    bcClearBeans();

    beancatch->eggyDevitoed = beancatch->eggyDevito;
    beancatch->frameCounter = (beancatch->eggyDevitoed && beancatch->strikes < 3) ? -60 : 0;

    beancatch->refreshScreen = true;
    beancatch->update = &bcUpdateMiss;
}

void bcChangeStateGameOver(void)
{
    beancatch->state = BC_ST_GAME_OVER;
    beancatch->frameCounter = 0;

    //Write personal best high score
    switch(beancatch->gameLevel)
    {
        case BC_GAME_LEVEL_A:
            writeNvsBlob(KEY_SCORE_PB_GAME_A, &(beancatch->highScoreGameA), sizeof(uint16_t));
            break;
        case BC_GAME_LEVEL_B:
            writeNvsBlob(KEY_SCORE_PB_GAME_B, &(beancatch->highScoreGameB), sizeof(uint16_t));
            break;
        default:
            break;
    }

    beancatch->refreshScreen = true;
    beancatch->update = &bcUpdateGameOver;
}

void bcChangeStatePause(void)
{
    beancatch->tempState = beancatch->state;
    beancatch->state = BC_ST_PAUSE;
    beancatch->btnState = 0;
    beancatch->prevBtnState = 0;
    beancatch->refreshScreen = true;
    beancatch->frameCounter = 0;
    beancatch->update = &bcUpdatePause;
}

void bcResumeTempState(void)
{
    beancatch->state = beancatch->tempState;
    beancatch->tempState = BC_ST_NULL;
    beancatch->update = (gameUpdateFuncton_t) gameStateToUpdateFunctions[beancatch->state];
}

void bcCheckPlayerCatchBean(void)
{
    if (beancatch->beanInDanger == beancatch->playerPosition)
    {
        // Catch bean
        beancatch->beanCount--;
        beancatch->beans[beancatch->playerPosition] = beancatch->beans[beancatch->playerPosition] & 0b011111;
        if (!beancatch->beans[beancatch->playerPosition])
        {
            beancatch->occupiedConveyorCount--;
        }
        beancatch->beanInDanger = BC_CONVEYOR_NULL;

        if(beancatch->state != BC_ST_CLOCK)
        {
            globalMidiPlayerPlaySong(&beancatch->sounds[BC_SOUND_SCORE_POINT], MIDI_BGM);
            bcScorePoint();
        }
           
    }
}
