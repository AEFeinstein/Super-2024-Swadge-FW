/**
 * @file mode_2048.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
 * @version 1.5.0
 * @date 2024-06-28
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "mode_2048.h"
#include "2048_game.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static void t48EnterMode(void);
static void t48ExitMode(void);
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

static const char* tileSpriteNames[] = {
    "Tile-Blue-Diamond.wsg", "Tile-Blue-Square.wsg",  "Tile-Cyan-Legs.wsg",      "Tile-Green-Diamond.wsg",
    "Tile-Green-Octo.wsg",   "Tile-Green-Square.wsg", "Tile-Mauve-Legs.wsg",     "Tile-Orange-Legs.wsg",
    "Tile-Pink-Diamond.wsg", "Tile-Pink-Octo.wsg",    "Tile-Pink-Square.wsg",    "Tile-Purple-Legs.wsg",
    "Tile-Red-Octo.wsg",     "Tile-Red-Square.wsg",   "Tile-Yellow-Diamond.wsg", "Tile-Yellow-Octo.wsg",
};

static const char* sparkleSpriteNames[] = {
    "Sparkle_Blue.wsg", "Sparkle_Cyan.wsg",   "Sparkle_Green.wsg", "Sparkle_Orange.wsg",
    "Sparkle_Pink.wsg", "Sparkle_Purple.wsg", "Sparkle_Red.wsg",   "Sparkle_Yellow.wsg",
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter 2048 mode and set everything up
 */
static void t48EnterMode(void)
{
    setFrameRateUs(16667); // 60 FPS

    // Init Mode & resources
    t48 = calloc(sizeof(t48_t), 1);

    // Load fonts
    loadFont("ibm_vga8.font", &t48->font, false);
    loadFont("sonic.font", &t48->titleFont, false);
    makeOutlineFont(&t48->titleFont, &t48->titleFontOutline, false);

    // Load images
    t48->tiles = calloc(ARRAY_SIZE(tileSpriteNames), sizeof(wsg_t));
    for (int32_t tIdx = 0; tIdx < ARRAY_SIZE(tileSpriteNames); tIdx++)
    {
        loadWsg(tileSpriteNames[tIdx], &t48->tiles[tIdx], true);
    }

    t48->sparkleSprites = calloc(ARRAY_SIZE(sparkleSpriteNames), sizeof(wsg_t));
    for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(sparkleSpriteNames); sIdx++)
    {
        loadWsg(sparkleSpriteNames[sIdx], &t48->sparkleSprites[sIdx], true);
    }

    // Load sounds
    loadMidiFile("Follinesque.mid", &t48->bgm, true);
    loadMidiFile("sndBounce.mid", &t48->click, true);

    // Initialize the game
    t48_gameInit(t48);

    // TODO reimplement main & high score menus
}

/**
 * @brief Exit 2048 mode and free all resources
 */
static void t48ExitMode(void)
{
    freeFont(&t48->titleFontOutline);
    freeFont(&t48->titleFont);
    freeFont(&t48->font);

    for (uint8_t i = 0; i < ARRAY_SIZE(sparkleSpriteNames); i++)
    {
        freeWsg(&t48->sparkleSprites[i]);
    }
    free(t48->sparkleSprites);

    for (uint8_t i = 0; i < ARRAY_SIZE(tileSpriteNames); i++)
    {
        freeWsg(&t48->tiles[i]);
    }
    free(t48->tiles);

    soundStop(true);
    unloadMidiFile(&t48->bgm);
    unloadMidiFile(&t48->click);

    free(t48);
}

/**
 * @brief The main game loop, responsible for input handling, game logic, and animation
 *
 * @param elapsedUs The time since this was last called, for animation
 */
static void t48MainLoop(int64_t elapsedUs)
{
    // Get inputs
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            // Process inputs
            t48_gameInput(t48, evt.button);
        }
    }

    // Loop the game
    t48_gameLoop(t48, elapsedUs);
}
