/**
 * @file mode_2048.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
 * @version 1.1.4
 * @date 2024-06-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "mode_2048.h"

#include "T48_leds.h"
#include "T48_logic.h"
#include "T48_menus.h"
#include "T48_draw.h"

//==============================================================================
// Function Prototypes
//==============================================================================

// Swadge functions

/**
 * @brief Mode setup
 *
 */
static void t48EnterMode(void);

/**
 * @brief Mode teardown
 *
 */
static void t48ExitMode(void);

/**
 * @brief Main loop of the code
 *
 * @param elapsedUs
 */
static void t48MainLoop(int64_t elapsedUs);

/**
 * @brief Restart audio when song ends
 *
 */
static void t48BgmCb(void);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t t48Mode = {
    .modeName                 = modeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = t48EnterMode,
    .fnExitMode               = t48ExitMode,
    .fnMainLoop               = t48MainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

t48_t* t48;

//==============================================================================
// Functions
//==============================================================================

// Swadge functions

static void t48EnterMode(void)
{
    // Init Mode & resources
    setFrameRateUs(T48_US_PER_FRAME);
    t48 = calloc(sizeof(t48_t), 1);

    // Load fonts
    loadFont("ibm_vga8.font", &t48->font, false);
    loadFont("sonic.font", &t48->titleFont, false);
    makeOutlineFont(&t48->titleFont, &t48->titleFontOutline, false);

    // Load images
    loadWsg("Tile-Blue-Diamond.wsg", &t48->tiles[0], true);
    loadWsg("Tile-Blue-Square.wsg", &t48->tiles[1], true);
    loadWsg("Tile-Cyan-Legs.wsg", &t48->tiles[2], true);
    loadWsg("Tile-Green-Diamond.wsg", &t48->tiles[3], true);
    loadWsg("Tile-Green-Octo.wsg", &t48->tiles[4], true);
    loadWsg("Tile-Green-Square.wsg", &t48->tiles[5], true);
    loadWsg("Tile-Mauve-Legs.wsg", &t48->tiles[6], true);
    loadWsg("Tile-Orange-Legs.wsg", &t48->tiles[7], true);
    loadWsg("Tile-Pink-Diamond.wsg", &t48->tiles[8], true);
    loadWsg("Tile-Pink-Octo.wsg", &t48->tiles[9], true);
    loadWsg("Tile-Pink-Square.wsg", &t48->tiles[10], true);
    loadWsg("Tile-Purple-Legs.wsg", &t48->tiles[11], true);
    loadWsg("Tile-Red-Octo.wsg", &t48->tiles[12], true);
    loadWsg("Tile-Red-Square.wsg", &t48->tiles[13], true);
    loadWsg("Tile-Yellow-Diamond.wsg", &t48->tiles[14], true);
    loadWsg("Tile-Yellow-Octo.wsg", &t48->tiles[15], true);
    loadWsg("Sparkle_Blue.wsg", &t48->sparkleSprites[0], true);
    loadWsg("Sparkle_Cyan.wsg", &t48->sparkleSprites[1], true);
    loadWsg("Sparkle_Green.wsg", &t48->sparkleSprites[2], true);
    loadWsg("Sparkle_Orange.wsg", &t48->sparkleSprites[3], true);
    loadWsg("Sparkle_Pink.wsg", &t48->sparkleSprites[4], true);
    loadWsg("Sparkle_Purple.wsg", &t48->sparkleSprites[5], true);
    loadWsg("Sparkle_Red.wsg", &t48->sparkleSprites[6], true);
    loadWsg("Sparkle_Yellow.wsg", &t48->sparkleSprites[7], true);

    // Load sounds
    loadMidiFile("Follinesque.mid", &t48->bgm, true);
    loadMidiFile("sndBounce.mid", &t48->click, true);

    // Init Text Entry
    textEntryInit(&t48->font, 4, t48->playerInitials);
    textEntrySetBGColor(c001);
    textEntrySetEmphasisColor(c500);
    textEntrySetNewCapsStyle(true);
    textEntrySetNewEnterStyle(true);

    // Init High scores
    for (int8_t i = 0; i < T48_HS_COUNT; i++)
    {
        if (!readNvs32(highScoreKey[i], &t48->highScore[i]))
        {
            switch (i)
            {
                case 0:
                    t48->highScore[i] = 100000;
                    break;
                case 1:
                    t48->highScore[i] = 50000;
                    break;
                case 2:
                    t48->highScore[i] = 25000;
                    break;
                case 3:
                    t48->highScore[i] = 10000;
                    break;
                case 4:
                    t48->highScore[i] = 5000;
                    break;
            }
            writeNvs32(highScoreKey[i], t48->highScore[i]);
        }
        size_t len = 4;
        if (!readNvsBlob(highScoreInitialsKey[i], &t48->hsInitials[i], &len))
        {
            static char buff[5];
            switch (i)
            {
                case 0:
                    strcpy(buff, "JW");
                    break;
                case 1:
                    strcpy(buff, "Pan");
                    break;
                case 2:
                    strcpy(buff, "Pix");
                    break;
                case 3:
                    strcpy(buff, "Poe");
                    break;
                case 4:
                    strcpy(buff, "DRG");
                    break;
            }
            strcpy(t48->hsInitials[i], buff);
            writeNvsBlob(highScoreInitialsKey[i], &t48->hsInitials[i], len);
        }
    }
    
    // Init game
    for (int i = 0; i < T48_BOARD_SIZE; i++)
    {
        t48->board[i].x = i/4;
        t48->board[i].y = i%4;
    }
    t48->ds = GAMESTART;
}

static void t48ExitMode(void)
{
    freeFont(&t48->titleFontOutline);
    freeFont(&t48->titleFont);
    freeFont(&t48->font);

    for (uint8_t i = 0; i < T48_SPARKLE_COUNT; i++)
    {
        freeWsg(&t48->sparkleSprites[i]);
    }
    for (uint8_t i = 0; i < T48_TILE_COUNT; i++)
    {
        freeWsg(&t48->tiles[i]);
    }

    soundStop(true);
    unloadMidiFile(&t48->bgm);
    unloadMidiFile(&t48->click);

    free(t48);
}

static void t48MainLoop(int64_t elapsedUs)
{
    // Dim LEDs
    t48DimLEDs(t48);
    // Play BGM if it's not playing
    if (!t48->bgmIsPlaying)
    {
        //soundPlayBgmCb(&t48->bgm, MIDI_BGM, t48BgmCb);
        t48->bgmIsPlaying = true;
    }
    // Get inputs
    buttonEvt_t evt;
    switch (t48->ds)
    {
        case GAMESTART:
            // Check any button is pressed
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48StartGame(t48);
                    t48->ds = GAME;
                    for (int i = 0; i < T48_BOARD_SIZE; i++)
                    {
                        if (true)
                        {
                            t48->board[i].val = 2;
                        }
                    }
                }
            }
            // Draw
            t48StartScreen(t48, t48Rainbow(t48));
            t48RandLEDs(t48);
            break;
        case GAME:
            // Input
            while (checkButtonQueueWrapper(&evt))
            {
                // Move blocks down, up, right or left
                if (evt.down && evt.button & PB_DOWN)
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48SlideDown(t48);
                }
                else if (evt.down && evt.button & PB_UP)
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48SlideUp(t48);
                }
                else if (evt.down && evt.button & PB_LEFT)
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48SlideLeft(t48);
                }
                else if (evt.down && evt.button & PB_RIGHT)
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48SlideRight(t48);
                }
                // Restart game if you hit start
                else if (evt.down && evt.button & PB_START)
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48->ds = CONFIRM;
                }
            }
            // Check game is done or "done"
            if (t48CheckWin(t48))
            {
                t48->ds = WIN;
            }
            if (t48CheckOver(t48))
            {
                for (int8_t i = 0; i < T48_HS_COUNT; i++)
                {
                    if (t48->highScore[i] <= t48->score)
                    {
                        t48->newHS = true;
                    }
                }
                if (t48->newHS)
                {
                    t48->ds = WRITE;
                }
                else
                {
                    t48->ds = GAMEOVER;
                }
            }
            // Draw
            t48Draw(t48);
            break;
        case GAMEOVER:
            // Check any button is pressed
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && (evt.button & PB_A || evt.button & PB_B))
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48->ds = GAMESTART;
                }
            }
            // Draw
            t48DrawGameOverScreen(t48, t48->score, t48Rainbow(t48));
            t48RandLEDs(t48);
            break;
        case WIN:
            // Check any button is pressed
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && (evt.button & PB_A || evt.button & PB_B))
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48->ds = GAME;
                }
            }
            // Draw
            t48DrawWinScreen(t48);
            break;
        case CONFIRM:
            // Input
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && evt.button & PB_B)
                {
                    t48->ds = GAMESTART;
                }
                else if (evt.down && evt.button & PB_A)
                {
                    t48->ds = GAME;
                }
            }
            t48DrawConfirm(t48);
            t48RandLEDs(t48);
            break;
        case WRITE:
            while (checkButtonQueueWrapper(&evt))
            {
                t48->textEntryDone = !textEntryInput(evt.down, evt.button);
            }
            if (t48->textEntryDone)
            {
                t48SortHighScores(t48);
                textEntrySoftReset();
                t48->ds = GAMEOVER;
            }
            textEntryDraw(elapsedUs);
            break;
        default:
            break;
    }
}

static void t48BgmCb()
{
    t48->bgmIsPlaying = false;
}