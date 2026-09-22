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
#include "esp_log.h"
#include "macros.h"

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
    int16_t frameCounter;

    bc_gameStateEnum_t state;
    gameUpdateFuncton_t update;
    bool refreshScreen;

    font_t lcdNumbersFont;
    wsg_t wsgs[BC_WSG_SIZE];
    midiFile_t sounds[BC_SOUND_INDEX_MAX];

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
static void bcUpdateGame(void);
static void bcUpdateGameOver(void);

static void bcDrawGame(void);
static void bcDrawScoreHud(uint16_t value);

static void bcClearBeans(void);
static bool bcIgnoreCurrentConveyorForGameA(void);
static void bcScorePoint(void);
static void bcNewGame(void);
static void bcChangeStateGameA(void);
static void bcChangeStateGameB(void);
static void bcChangeStateClock(void);
static void bcCheckPlayerCatchBean(void);

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


    for (uint16_t i = 0; i < BC_SOUND_INDEX_MAX; i++)
    {
        loadMidiFile(BC_SOUND_MAP[i], &beancatch->sounds[i], false);
    }

    loadFont(LCD_NUMBERS_FONT, &beancatch->lcdNumbersFont, false);

    beancatch->refreshScreen = true;
    beancatch->update = &bcUpdateAcl;
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
            //if(esp_random() % 2){ //testing segments for now...
                segment = BC_LCD_SEGMENTS[i];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
            //}
        }

        char buffer[5];

        snprintf(buffer, sizeof(buffer) - 1, "%2d", 18 /*esp_random() % 18*/);
        drawText(&beancatch->lcdNumbersFont, c000, buffer, 25, 53);

        snprintf(buffer, sizeof(buffer) - 1, "%02d", 88 /*esp_random() % 18*/);
        drawText(&beancatch->lcdNumbersFont, c000, buffer, 68, 53);

        beancatch->refreshScreen = false;
    }

    beancatch->frameCounter++;
    if(beancatch->frameCounter > 59)
    {
        beancatch->frameCounter = 0;
        bcChangeStateClock();
    }
}

void bcUpdateClock(void)
{
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

    bcDrawGame();
}

void bcUpdateGame(void)
{
    if(!beancatch->emptyLoopCountdown)
    {
        beancatch->waitLoopCounter++;

        if(beancatch->waitLoopCounter > beancatch->waitLoopMax)
        {
            if(beancatch->beanInDanger > -1)
            {
                //Cleanup after strike
                beancatch->droppedBeanLocation = beancatch->beanInDanger;
                beancatch->beanInDanger = BC_CONVEYOR_NULL;
                beancatch->waitLoopCounter = 0;

                globalMidiPlayerPlaySong(&beancatch->sounds[BC_SOUND_STRIKE], MIDI_SFX);
                bcClearBeans();

                beancatch->eggyDevitoed = beancatch->eggyDevito;

                beancatch->emptyLoopCountdown = 60;
            }
            else
            {
                //handle beans
                beancatch->idleConveyorCounter=0;

                do {
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
                        beancatch->beans[beancatch->currentConveyor]++;
                        beancatch->beanCount++;
                        beancatch->refreshScreen = true;
                    }

                    if (beancatch->beans[beancatch->currentConveyor] > 0)
                    {
                        beancatch->idleConveyorCounter=0;
                        bcCheckPlayerCatchBean();
                        globalMidiPlayerPlaySong(&beancatch->sounds[beancatch->currentConveyor], MIDI_SFX);
                        beancatch->waitLoopCounter=0;
                        beancatch->refreshScreen = true;
                    }
                    else
                    {
                        beancatch->idleConveyorCounter++;
                    }

                    beancatch->currentConveyor = (beancatch->currentConveyor + 1) % 4;
                } while (beancatch->idleConveyorCounter > 0 && beancatch->idleConveyorCounter < 3);
            }
        }
        
    } 
    else
    {
        beancatch->emptyLoopCountdown--;

        if(!beancatch->emptyLoopCountdown)
        {
            if(beancatch->droppedBeanLocation != BC_CONVEYOR_NULL)
            {
                beancatch->droppedBeanLocation = BC_CONVEYOR_NULL;

                if(beancatch->eggyDevitoed)
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
                    beancatch->emptyLoopCountdown = 240;
                }
            }
            else if(beancatch->strikes >= 3 && !beancatch->halfStrike)
            {
                beancatch->strikes = 0;
                bcChangeStateClock();
            }
        }
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

    bcDrawGame();
}

void bcDrawGame(void)
{
    if(beancatch->refreshScreen)
    {
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c445);
        drawWsgSimple(&beancatch->wsgs[BC_WSG_BEAN_CATCH_BG], 0, 28);

        bc_LcdSegment_t segment;
 
        for (uint16_t i = 0; i < 5; i++)
        {
            if( (beancatch->beans[BC_CONVEYOR_LU] >> i) & 0b1)
            {
                segment = BC_LCD_SEGMENTS[BC_SEG_BEAN_UL_00 + i];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
            }

            if( (beancatch->beans[BC_CONVEYOR_LD] >> i) & 0b1)
            {
                segment = BC_LCD_SEGMENTS[BC_SEG_BEAN_DL_00 + i];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
            }

            if( (beancatch->beans[BC_CONVEYOR_RU] >> i) & 0b1)
            {
                segment = BC_LCD_SEGMENTS[BC_SEG_BEAN_UR_00 + i];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
            }

            if( (beancatch->beans[BC_CONVEYOR_RD] >> i) & 0b1)
            {
                segment = BC_LCD_SEGMENTS[BC_SEG_BEAN_DR_00 + i];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
            }
        }

        switch(beancatch->playerPosition)
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

        if(beancatch->strikes)
        {
            segment = BC_LCD_SEGMENTS[BC_SEG_STRIKE_LBL];
            drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
        }

        for (uint16_t i = 0; i < beancatch->strikes; i++)
        {
            if(beancatch->halfStrike && i == beancatch->strikes-1 && (beancatch->frameCounter > 29) )
            {
                break;
            }
            
            segment = BC_LCD_SEGMENTS[BC_SEG_STRIKE_ICON_00];
            drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
        }

        switch(beancatch->state)
        {
            case BC_ST_GAME_A:
                segment = BC_LCD_SEGMENTS[BC_SEG_GAME_A_LBL];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);

                bcDrawScoreHud(beancatch->score);

                break;
            case BC_ST_GAME_B:
                segment = BC_LCD_SEGMENTS[BC_SEG_GAME_B_LBL];
                drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);

                bcDrawScoreHud(beancatch->score);

                break;
            case BC_ST_CLOCK:
                if(beancatch->btnState & PB_A)
                {
                    bcDrawScoreHud(beancatch->highScoreGameA);
                }
                else if(beancatch->btnState & PB_B)
                {
                    bcDrawScoreHud(beancatch->highScoreGameB);
                } 
                else
                {
                    //draw clock
                }
            default:
                break;
        }

        if(beancatch->eggyDevito)
        {
            segment = BC_LCD_SEGMENTS[BC_SEG_EGGY_DEVITO];
            drawWsgSimple(&beancatch->wsgs[segment.wsgIndex], segment.x, segment.y);
        }

        switch(beancatch->droppedBeanLocation)
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
                
    if(value > 99){
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
    for(uint16_t i = 0; i < 4; i++)
    {
        beancatch->beans[i] = 0;
    }

    beancatch->beanCount = 0;

    beancatch->refreshScreen = true;
}

bool bcIgnoreCurrentConveyorForGameA(void)
{
    if(beancatch->state != BC_ST_GAME_A) 
    {
        return false;
    }

    return (beancatch->currentConveyor ==((beancatch->strikes+2) % 4));
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
        beancatch->maxBeans = BC_DIFFICULTY_MAX_BEANS[CLAMP(scoreHundredsDigit + scoreTensDigit, 0, BC_DIFFICULTY_MAX_BEANS_SIZE)];
    }
    
    switch(beancatch->state)
    {
        case BC_ST_GAME_A:
            if(beancatch->score > beancatch->highScoreGameA)
            {
                beancatch->highScoreGameA = beancatch->score;
            }
            break;
        case BC_ST_GAME_B:
            if(beancatch->score > beancatch->highScoreGameB)
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
    beancatch->state = BC_ST_GAME_A;
    bcNewGame();
    beancatch->waitLoopMax = 31;
    beancatch->update = &bcUpdateGame;
}

void bcChangeStateGameB(void)
{
    beancatch->state = BC_ST_GAME_B;
    bcNewGame();
    beancatch->waitLoopMax = 25;
    beancatch->update = &bcUpdateGame;
}

void bcChangeStateClock(void)
{
    beancatch->state = BC_ST_CLOCK;
    bcNewGame();
    beancatch->refreshScreen = true;
    beancatch->update = &bcUpdateClock;
}

void bcCheckPlayerCatchBean(void)
{
    if(beancatch->beanInDanger == beancatch->playerPosition)
    {
        //Catch bean
        beancatch->beanCount--;
        beancatch->beans[beancatch->playerPosition] = beancatch->beans[beancatch->playerPosition] & 0b011111;
        beancatch->beanInDanger = BC_CONVEYOR_NULL;
        globalMidiPlayerPlaySong(&beancatch->sounds[BC_SOUND_SCORE_POINT], MIDI_BGM);
        bcScorePoint();
    }
}
