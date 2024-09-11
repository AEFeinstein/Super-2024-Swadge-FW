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
#include "2048_game.h"

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

//==============================================================================
// Variables
//==============================================================================

const char modeName[] = "2048";

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

    t48_gameInit(t48);
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
    // Get inputs
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            t48_gameInput(t48, evt.button);
        }
    }

    t48_gameDraw(t48, elapsedUs);
}
