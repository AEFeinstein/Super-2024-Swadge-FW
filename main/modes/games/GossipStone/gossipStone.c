//==============================================================================
// Includes
//==============================================================================

#include "gossipStone.h"
#include "mainMenu.h"
#include "gs_entity.h"
#include "gs_entityManager.h"
#include "hdw-ch32v003.h"
#include "gs_gossip.h"
#include "gs_utility.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Consts
//==============================================================================

// It's good practice to declare immutable strings as const so they get placed in ROM, not RAM
const char gs_ModeName[]                = "Talky Rocky";
const char gs_trophyNVS[]               = "OreatorTroph";
static const char gs_GossipStr[]        = "Gossip";
static const char gs_AskMeAnythingStr[] = "Ask Me Anything";
static const char gs_ProphecyStr[]      = "The Prophecy";
static const char gs_MoonStr[]          = "The Moon";
static const char gs_ExitStr[]          = "Exit";

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
    gameData->submode = GS_MENU_SUBMODE;
    gameData->menu    = initMenu(gs_ModeName, gs_menuCb);

    gs_populateMenu();

    gs_loadAssets();

    gs_initializeGame();

    // read NVS
    gameData->gossipProgress = heap_caps_calloc((GOSSIP_COUNT / 32) + 1, sizeof(int32_t), MALLOC_CAP_SPIRAM);
    for (int i = 0; i < (GOSSIP_COUNT / 32) + 1; i++)
    {
        char nvsKey[20];
        sprintf(nvsKey, "gossipProgress%d", i);
        readNvs32(nvsKey, &gameData->gossipProgress[i]);
    }
}

void gs_populateMenu(void)
{
    removeAllItemsFromMenu(gameData->menu);
    // Gossip
    addSingleItemToMenu(gameData->menu, gs_GossipStr);
    // Ask Me Anything
    addSingleItemToMenu(gameData->menu, gs_AskMeAnythingStr);
    // The Prophecy
    if (trophyGetSavedValue(&gs_trophies[0]) == GOSSIP_COUNT - 1)
    {
        addSingleItemToMenu(gameData->menu, gs_ProphecyStr);
    }
    // The Moon

    // Exit
    addSingleItemToMenu(gameData->menu, gs_ExitStr);
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
        switch (gameData->submode)
        {
            case GS_MENU_SUBMODE:
            {
                gameData->menu = menuButton(gameData->menu, evt);
                break;
            }
            case GS_GOSSIP_SUBMODE:
            case GS_AMA_SUBMODE:
            case GS_PROPHECY_SUBMODE:
            case GS_MOON_SUBMODE:
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

    getTouchLinear(gameData->touchState, ARRAY_SIZE(gameData->touchState));

    // Update and Draw depending on the submode
    switch (gameData->submode)
    {
        case GS_MENU_SUBMODE:
        {
            drawMenuMega(gameData->menu, gameData->menuRenderer, elapsedUs);
            break;
        }
        case GS_GOSSIP_SUBMODE:
        case GS_AMA_SUBMODE:
        case GS_PROPHECY_SUBMODE:
        case GS_MOON_SUBMODE:
        {
            if (gameData->btnDownState & PB_B)
            {
                gameData->submode = GS_MENU_SUBMODE;
                gs_populateMenu();
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
    paletteColor_t col = c011;
    if(gameData->entityManager.camera.pos.y < 0xFFFF - (370 << DECIMAL_BITS)){
        col = c000;
    }
    memset(&frameBuf[(y * TFT_WIDTH) + x], col, sizeof(paletteColor_t) * w * h);
}

static void gs_loadAssets(void)
{
    gs_loadAsset(SKY_GRADIENT_WSG, 1, &gameData->assets[GS_SKY_GRADIENT_ASSET]);
    gs_loadAsset(HILL_WSG, 1, &gameData->assets[GS_HILL_ASSET]);
    gs_loadAsset(MOON_WSG, 1, &gameData->assets[GS_MOON_ASSET]);
    gs_loadAsset(GOSSIP_STONE_0_WSG, 3, &gameData->assets[GS_GOSSIP_STONE_ASSET]);
    gs_loadAsset(FLAME_0_WSG, 6, &gameData->assets[GS_FLAME_ASSET]);
    gs_loadAsset(MOON_TILE_0_WSG, 15, &gameData->assets[GS_MOON_TILE_ASSET]);
    gs_loadAsset(STAR_0_WSG, 14, &gameData->assets[GS_STAR_ASSET]); // anim sequences 0-5, 6, 7-12, 13, 10
    gs_loadAsset(SPACE_GRADIENT_WSG, 1, &gameData->assets[GS_SPACE_GRADIENT_ASSET]);
    gs_loadAsset(HI_RES_MOON_WSG, 1, &gameData->assets[GS_HI_RES_MOON_ASSET]);
}

static void gs_initializeGame(void)
{
    gs_entity_t* skyGradient
        = gs_createEntity(&gameData->entityManager, 1, GS_NO_ANIMATION, false, GS_SKY_GRADIENT_ASSET, 1,
                          (vec_t){0xFFFF - (0 << DECIMAL_BITS), 0xFFFF + (25 << DECIMAL_BITS)}, gameData);
    skyGradient->drawFunction = gs_drawSkyGradient;

    gs_entity_t* spaceGradient
        = gs_createEntity(&gameData->entityManager, 1, GS_NO_ANIMATION, false, GS_SPACE_GRADIENT_ASSET, 1,
                          (vec_t){0xFFFF - (0 << DECIMAL_BITS), 0xFFFF - (250 << DECIMAL_BITS)}, gameData);
    spaceGradient->drawFunction = gs_drawSkyGradient;

    for (int i = 0; i < 20; i++)
    {
        gs_entity_t* star
            = gs_createEntity(&gameData->entityManager, 0, GS_NO_ANIMATION, false, GS_STAR_ASSET, 0,
                              (vec_t){0xFFFF + (gs_randomInt(-(TFT_WIDTH >> 1), TFT_WIDTH >> 1) << DECIMAL_BITS),
                                      0xFFFF + (gs_randomInt(0, 90) << DECIMAL_BITS)},
                              gameData);
        star->data = heap_caps_calloc(1, sizeof(gs_star_t), MALLOC_CAP_SPIRAM);
        gs_randomizeStarData(star);
        star->updateFarFunction = gs_updateFarStar;
        star->drawFunction      = gs_drawStar;
    }

    gs_createEntity(&gameData->entityManager, 1, GS_NO_ANIMATION, false, GS_HI_RES_MOON_ASSET, 1,
                    (vec_t){0xFFFF, 0xFFFF - (1000 << DECIMAL_BITS)}, gameData);

    gs_createEntity(&gameData->entityManager, 1, GS_NO_ANIMATION, false, GS_HILL_ASSET, 1,
                    (vec_t){0xFFFF, 0xFFFF + (102 << DECIMAL_BITS)}, gameData);
    gs_createEntity(&gameData->entityManager, 1, GS_NO_ANIMATION, false, GS_MOON_ASSET, 1,
                    (vec_t){0xFFFF - (90 << DECIMAL_BITS), 0xFFFF - (0 << DECIMAL_BITS)}, gameData);
    gs_entity_t* gossipStone
        = gs_createEntity(&gameData->entityManager, 3, GS_LOOPING_ANIMATION, false, GS_GOSSIP_STONE_ASSET, 5,
                          (vec_t){0xFFFF + (11 << DECIMAL_BITS), 0xFFFF + (82 << DECIMAL_BITS)}, gameData);
    gossipStone->data                                = heap_caps_calloc(1, sizeof(gs_gossipStone_t), MALLOC_CAP_SPIRAM);
    gossipStone->dataType                            = GS_GOSSIP_STONE_DATA;
    ((gs_gossipStone_t*)gossipStone->data)->grounded = true;
    gs_entity_t* gossip = gs_createEntity(&gameData->entityManager, 0, GS_NO_ANIMATION, false, GS_NO_ASSET, 0,
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
        // Failed
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
        if (label == gs_GossipStr)
        {
            gameData->submode   = GS_GOSSIP_SUBMODE;
            gs_entity_t* gossip = gs_findLastEntityOfType(gameData->entityManager.entities->first->val, GS_GOSSIP_DATA);
            gs_gossip_t* gData = (gs_gossip_t*)gossip->data;
            gData->messageList = gossipList;
            gData->arr_size    = GOSSIP_COUNT;
            // reset a few things because the player may have exited and entered.
            gData->index    = 0;
            ((gs_gossip_t*)gossip->data)->progress = 0;
        }
        else if (label == gs_AskMeAnythingStr)
        {
            gameData->submode   = GS_AMA_SUBMODE;
            gs_entity_t* gossip = gs_findLastEntityOfType(gameData->entityManager.entities->first->val, GS_GOSSIP_DATA);
            gs_gossip_t* gData = (gs_gossip_t*)gossip->data;
            gData->messageList = AMAList;
            gData->arr_size    = AMA_COUNT;
            // reset a few things because the player may have exited and entered.
            gData->index    = 0;
            gData->progress = 0;
        }
        else if (label == gs_ProphecyStr)
        {
            gameData->submode   = GS_PROPHECY_SUBMODE;
            gs_entity_t* gossip = gs_findLastEntityOfType(gameData->entityManager.entities->first->val, GS_GOSSIP_DATA);
            gs_gossip_t* gData = (gs_gossip_t*)gossip->data;
            gData->messageList = ProphecyList;
            gData->arr_size    = PROPHECY_COUNT;
            gData->onDialogueFinished = gs_enableFlightControls;
            // reset a few things because the player may have exited and entered.
            gData->index    = 0;
            gData->progress = 0;

            if (gs_findLastEntityOfType(gameData->entityManager.entities->first->val, GS_FLAME_DATA) == NULL)
            {
                gs_entity_t* flame = gs_createEntity(&gameData->entityManager, 6, GS_NO_ANIMATION, false,
                                                     GS_FLAME_ASSET, 0, (vec_t){0xffff, 0xffff}, gameData);
                flame->data        = heap_caps_calloc(1, sizeof(gs_flame_t), MALLOC_CAP_SPIRAM);
                gs_entity_t* gossipStone
                    = gs_findLastEntityOfType(gameData->entityManager.entities->first->val, GS_GOSSIP_STONE_DATA);
                gossipStone->updateFunction                       = gs_updateGossipStone;
                gossipStone->drawFunction                         = gs_drawGossipStone;
                ((gs_gossipStone_t*)gossipStone->data)->flame     = flame;
                ((gs_gossipStone_t*)gossipStone->data)->rotateDeg = (360 - 45) << DECIMAL_BITS;
                if (gossip->data != NULL)
                {
                    flame->dataType     = GS_FLAME_DATA;
                    flame->drawFunction = gs_drawFlame;
                }
                else
                {
                    // Failed
                    switchToSwadgeMode(&mainMenuMode);
                }
            }
        }
        else if (label == gs_MoonStr)
        {
            if (gameData->entityManager.tilemap == NULL)
            {
                gameData->entityManager.tilemap
                    = gs_createEntity(&gameData->entityManager, 15, GS_NO_ANIMATION, true, GS_MOON_TILE_ASSET, 0,
                                      (vec_t){0xffff, 0xffff}, gameData);
                gameData->entityManager.tilemap->dataType = GS_TILEMAP_DATA;
                // calloc the columns in layers separately to avoid a big alloc
                for (int32_t w = 0; w < TILE_FIELD_WIDTH; w++)
                {
                    ((gs_tilemap_t*)gameData->entityManager.tilemap->data)->tiles[w]
                        = heap_caps_calloc_tag(TILE_FIELD_HEIGHT, sizeof(gs_tileInfo_t), MALLOC_CAP_SPIRAM, "fgTile");
                }
            }
        }
        else if (label == gs_ExitStr)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    return false;
}
