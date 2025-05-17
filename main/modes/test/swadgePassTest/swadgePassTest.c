//==============================================================================
// Includes
//==============================================================================

#include "swadgePassTest.h"

//==============================================================================
// Structs
//==============================================================================

/// @brief The struct that holds all the state for the SwadgePass test mode
typedef struct
{
    font_t ibm; ///< The font used to display text
    menu_t* menu;
    menuManiaRenderer_t* renderer;
    list_t swadgePassKeys;
} swadgePassTest_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void swadgePassTestMainLoop(int64_t elapsedUs);
static void swadgePassTestEnterMode(void);
static void swadgePassTestExitMode(void);
void swadgePassTestMenuCb(const char* label, bool selected, uint32_t value);

//==============================================================================
// Strings
//==============================================================================

static const char swadgePassTestName[] = "SwadgePass Test";

//==============================================================================
// Variables
//==============================================================================

/// The Swadge mode for swadgePassTest
swadgeMode_t swadgePassTestMode = {
    .modeName                 = swadgePassTestName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = swadgePassTestEnterMode,
    .fnExitMode               = swadgePassTestExitMode,
    .fnMainLoop               = swadgePassTestMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

/// All state information for the SwadgePass Test mode.
swadgePassTest_t* spt = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter SwadgePass Test mode, allocate required memory, and initialize required variables
 *
 */
static void swadgePassTestEnterMode(void)
{
    spt = heap_caps_calloc(1, sizeof(swadgePassTest_t), MALLOC_CAP_8BIT);

    // Load a font
    loadFont(IBM_VGA_8_FONT, &spt->ibm, false);

    // Get SwadgePass keys
    getSwadgePassKeys(&spt->swadgePassKeys);

    // Initialize menu
    spt->menu = initMenu(swadgePassTestName, swadgePassTestMenuCb);

    // Add all keys to the menu
    node_t* keyNode = spt->swadgePassKeys.first;
    while (keyNode)
    {
        addSingleItemToMenu(spt->menu, keyNode->val);
        keyNode = keyNode->next;
    }

    // Initialize renderer
    spt->renderer = initMenuManiaRenderer(NULL, NULL, NULL);
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void swadgePassTestExitMode(void)
{
    // Free the font
    freeFont(&spt->ibm);

    // Free the menu
    deinitMenu(spt->menu);
    deinitMenuManiaRenderer(spt->renderer);

    // Free the keys
    while (spt->swadgePassKeys.length)
    {
        heap_caps_free(pop(&spt->swadgePassKeys));
    }

    // Free the mode
    heap_caps_free(spt);
}

/**
 * @brief This function is called periodically and frequently. It will both read inputs and draw the screen.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void swadgePassTestMainLoop(int64_t elapsedUs)
{
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        spt->menu = menuButton(spt->menu, evt);
    }

    // Draw menu
    drawMenuMania(spt->menu, spt->renderer, elapsedUs);
}

/**
 * @brief A callback which is called when a menu changes or items are selected
 * @param label A pointer to the label which was selected or scrolled to
 * @param selected true if the item was selected with the A button, false if it was scrolled to
 * @param value If a settings item was selected or scrolled, this is the new value for the setting
 */
void swadgePassTestMenuCb(const char* label, bool selected, uint32_t value)
{
    // TODO show more data from the packet
}