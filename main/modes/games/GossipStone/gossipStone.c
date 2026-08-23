//==============================================================================
// Includes
//==============================================================================

#include "gossipStone.h"
#include "mainMenu.h"
#include "gs_entity.h"
#include "gs_entityManager.h"
#include "hdw-ch32v003.h"
#include "gs_gossip.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Consts
//==============================================================================

// It's good practice to declare immutable strings as const so they get placed in ROM, not RAM
const char gs_ModeName[]                = "Gossip Stone";
const char gs_trophyNVS[]               = "GossipTroph";
static const char gs_GossipStr[]        = "Gossip";
static const char gs_AskMeAnythingStr[] = "Ask Me Anything";

//==============================================================================
// Function Prototypes
//==============================================================================

static void gs_enterMode(void);
static void gs_exitMode(void);
static void gs_mainLoop(int64_t elapsedUs);
static void gs_loadAssets(void);
static void gs_initializeGame(void);
static void gs_freeAssets(void);
bool gs_menuCb(const char* label, bool selected, uint32_t value);
static void gs_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void dn_freeAssets(void);

//==============================================================================
// Variables
//==============================================================================

const trophyData_t gs_trophies[] = {
    {
        // 0
        .title       = "The Prophecy",
        .description = "Listen to all the gossip.",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = GOSSIP_COUNT - 1, // Because the first message is tutorial text.
    },
};

// Individual mode settings
trophySettings_t gs_trophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 5,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = gs_trophyNVS,
};

// This is passed to the swadgeMode_t
trophyDataList_t trophyData = {.settings = &gs_trophySettings, .list = gs_trophies, .length = ARRAY_SIZE(gs_trophies)};

swadgeMode_t gossipStoneMode = {.modeName = gs_ModeName, // Assign the name we created here
                                .wifiMode = NO_WIFI, // If we want WiFi. WiFi is expensive computationally/battery-wise,
                                                     // so disable it if you're not going to use it.
                                .overrideUsb = false, // Overrides the default USB behavior. This is helpful for the
                                                      // game controller mode but unlikely to be useful for your game.
                                .usesAccelerometer = false, // If we're using motion controls
                                .usesThermometer   = false, // If we're using the internal thermometer
                                .overrideSelectBtn = false, // The select/Menu button has a default behavior. If you
                                                            // want to override it, you can set this to true but you'll
                                                            // need to re-implement the 'return to main menu' behavior.
                                .fnEnterMode              = gs_enterMode, // The enter mode function
                                .fnExitMode               = gs_exitMode,  // The exit mode function
                                .fnMainLoop               = gs_mainLoop,  // The loop function
                                .fnAudioCallback          = NULL,         // If the mode uses the microphone
                                .fnBackgroundDrawCallback = gs_BackgroundDrawCallback, // Draws a section of the display
                                .fnEspNowRecvCb           = NULL, // If using Wifi, add the receive function here
                                .fnEspNowSendCb           = NULL, // If using Wifi, add the send function here
                                .fnAdvancedUSB            = NULL, // If using advanced USB things_.
                                .trophyData               = &trophyData};

/// @brief A heatshrink decoder to use for all WSG loads rather than allocate a new one for each WSG
/// This helps to prevent memory fragmentation in SPIRAM.
/// Note, this is outside the dn_t struct for easy access to loading fuctions without dn_t references
heatshrink_decoder* gs_hsd;
/// @brief A temporary decode space to use for all WSG loads
uint8_t* gs_decodeSpace;

gs_gameData_t* gameData;

static void gs_enterMode(void)
{
    setFrameRateUs(16666);
    gameData = (gs_gameData_t*)heap_caps_calloc(1, sizeof(gs_gameData_t), MALLOC_CAP_8BIT);

    gameData->trophyData = &gs_trophies;

    // set the camera to the center of positive ints
    gameData->camera.pos
        = (vec_t){0xFFFF - (TFT_WIDTH << (GS_DECIMAL_BITS - 1)), 0xFFFF - (TFT_HEIGHT << (GS_DECIMAL_BITS - 1))};
    gs_initializeEntityManager(&gameData->entityManager, gameData);
    gs_setAssetMetaData();

    // Allocate WSG loading helpers
    gs_hsd = heatshrink_decoder_alloc(256, 8, 4);
    // The largest image is bb_menu2.png, decodes to 99124 bytes
    // 99328 is 1024 * 97
    gs_decodeSpace
        = heap_caps_malloc_tag(99328, MALLOC_CAP_SPIRAM, "decodeSpace"); // TODO change the size to the largest sprite
    loadFont(IBM_VGA_8_FONT, &gameData->font_gossip, true);

    // Initialize a menu renderer
    gameData->menuRenderer = initMenuMegaRenderer(NULL, NULL, NULL);

    // Initialize the main menu
    gameData->ui   = UI_MENU;
    gameData->menu = initMenu(gs_ModeName, gs_menuCb);
    addSingleItemToMenu(gameData->menu, gs_GossipStr);
    addSingleItemToMenu(gameData->menu, gs_AskMeAnythingStr);

    gs_loadAssets();

    gs_initializeGame();
}

void gs_setAssetMetaData(void)
{
}

static void gs_exitMode(void)
{
    gs_freeAssets();
    gs_freeEntityManager(&gameData->entityManager);
    // Free the fonts
    freeFont(&gameData->font_gossip);
    // unloadMidiFile(&gameData->songMidi);
    heap_caps_free(gameData);
}

static void gs_mainLoop(int64_t elapsedUs)
{
    // Handle inputs
    gameData->btnDownState = 0;
    buttonEvt_t evt        = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        gameData->btnState = evt.state;
        switch (gameData->ui)
        {
            case UI_MENU:
            {
                gameData->menu = menuButton(gameData->menu, evt);
                break;
            }
            case UI_GAME:
            {
                if (evt.down)
                {
                    // store the down presses for this frame
                    gameData->btnDownState += evt.button;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

    switch (gameData->ui)
    {
        case UI_MENU:
        {
            drawMenuMega(gameData->menu, gameData->menuRenderer, elapsedUs);
            break;
        }
        case UI_GAME:
        {
            if (gameData->btnDownState & PB_B)
            {
                gameData->ui = UI_MENU;
                break;
            }
            gameData->elapsedUs = elapsedUs;
            // update the whole engine via entity management
            gs_updateEntities(&gameData->entityManager);
            gs_drawEntities(&gameData->entityManager);
            break;
        }
        default:
        {
            break;
        }
    }
}

static void gs_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Fill the flat background color
    paletteColor_t* frameBuf = getPxTftFramebuffer();
    memset(&frameBuf[(y * TFT_WIDTH) + x], c011, sizeof(paletteColor_t) * w * h);
}

static void gs_loadAssets(void)
{
    gs_loadAsset(SKY_GRADIENT_WSG, 1, &gameData->assets[GS_SKY_GRADIENT_ASSET]);
    gs_loadAsset(HILL_WSG, 1, &gameData->assets[GS_HILL_ASSET]);
    gameData->assets[GS_HILL_ASSET].originX = 0;
    gs_loadAsset(MOON_WSG, 1, &gameData->assets[GS_MOON_ASSET]);
    gs_loadAsset(GOSSIP_STONE_0_WSG, 3, &gameData->assets[GS_GOSSIP_STONE_ASSET]);
}

static void gs_initializeGame(void)
{
    gs_entity_t* skyGradient
        = gs_createEntity(&gameData->entityManager, 1, GS_NO_ANIMATION, true, GS_SKY_GRADIENT_ASSET, 1,
                          (vec_t){0xFFFF - (0 << GS_DECIMAL_BITS), 0xFFFF + (25 << GS_DECIMAL_BITS)}, gameData);
    skyGradient->drawFunction = gs_drawSkyGradient;
    gs_createEntity(&gameData->entityManager, 1, GS_NO_ANIMATION, true, GS_HILL_ASSET, 1,
                    (vec_t){0xFFFF - (TFT_WIDTH << (GS_DECIMAL_BITS - 1)), 0xFFFF + (102 << GS_DECIMAL_BITS)},
                    gameData);
    gs_createEntity(&gameData->entityManager, 1, GS_NO_ANIMATION, true, GS_MOON_ASSET, 1,
                    (vec_t){0xFFFF - (90 << GS_DECIMAL_BITS), 0xFFFF - (0 << GS_DECIMAL_BITS)}, gameData);
    gs_entity_t* gossipStone
        = gs_createEntity(&gameData->entityManager, 3, GS_LOOPING_ANIMATION, false, GS_GOSSIP_STONE_ASSET, 5,
                          (vec_t){0xFFFF + (11 << GS_DECIMAL_BITS), 0xFFFF + (82 << GS_DECIMAL_BITS)}, gameData);
    gs_entity_t* gossip = gs_createEntity(&gameData->entityManager, 0, GS_NO_ANIMATION, true, GS_NO_ASSET, 0,
                                          (vec_t){0xffff, 0xffff}, gameData);
    gossip->data        = heap_caps_calloc(1, sizeof(gs_gossip_t), MALLOC_CAP_SPIRAM);
    if (gossip->data != NULL)
    {
        gossip->dataType                          = GS_GOSSIP_DATA;
        gossip->updateFunction                    = gs_updateGossip;
        gossip->drawFunction                      = gs_drawGossip;
        ((gs_gossip_t*)gossip->data)->gossipStone = gossipStone;
    }
    else
    {
        // Exit to the main menu
        switchToSwadgeMode(&mainMenuMode);
    }
}

static void gs_freeAssets(void)
{
    for (int i = 0; i < NUM_ASSETS; i++)
    {
        if (gameData->assets[i].allocated)
        {
            for (int frameIdx = 0; frameIdx < gameData->assets[i].numFrames; frameIdx++)
            {
                freeWsg(&gameData->assets[i].frames[frameIdx]);
            }
            heap_caps_free(gameData->assets[i].frames);
            gameData->assets[i].allocated = false;
        }
    }
}

/**
 * @brief Callback for when a Gossip Stone menu item is selected
 *
 * @param label The string label of the menu item selected
 * @param selected true if this was selected, false if it was moved to
 * @param value The value for settings, unused.
 */
bool gs_menuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (gs_GossipStr == label)
        {
            gameData->ui        = UI_GAME;
            gs_entity_t* gossip = gs_findLastEntityOfType(gameData->entityManager.entities->first->val, GS_GOSSIP_DATA);
            ((gs_gossip_t*)gossip->data)->messageList    = gossipList;
            ((gs_gossip_t*)gossip->data)->arr_size       = GOSSIP_COUNT;
            ((gs_gossip_t*)gossip->data)->gossipTracking = true;
            // reset a few things because the player may have exited and entered.
            ((gs_gossip_t*)gossip->data)->index    = 0;
            ((gs_gossip_t*)gossip->data)->progress = 0;
        }
        else if (gs_AskMeAnythingStr == label)
        {
            gameData->ui        = UI_GAME;
            gs_entity_t* gossip = gs_findLastEntityOfType(gameData->entityManager.entities->first->val, GS_GOSSIP_DATA);
            ((gs_gossip_t*)gossip->data)->messageList    = AMAList;
            ((gs_gossip_t*)gossip->data)->arr_size       = AMA_COUNT;
            ((gs_gossip_t*)gossip->data)->gossipTracking = false;
            // reset a few things because the player may have exited and entered.
            ((gs_gossip_t*)gossip->data)->index    = 0;
            ((gs_gossip_t*)gossip->data)->progress = 0;
        }
    }
    return false;
}
