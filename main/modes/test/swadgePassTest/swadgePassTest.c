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
    menu_t* menu;
    menuMegaRenderer_t* renderer;
    list_t swadgePasses;
    swadgePassData_t* currSpd;
} swadgePassTest_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void swadgePassTestMainLoop(int64_t elapsedUs);
static void swadgePassTestEnterMode(void);
static void swadgePassTestExitMode(void);
bool swadgePassTestMenuCb(const char* label, bool selected, uint32_t value);

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

    // Get SwadgePasses
    getSwadgePasses(&spt->swadgePasses, &swadgePassTestMode, true);

    // Initialize menu
    spt->menu = initMenu(swadgePassTestName, swadgePassTestMenuCb);

    // Add all keys to the menu
    node_t* passNode = spt->swadgePasses.first;
    while (passNode)
    {
        swadgePassData_t* spd = (swadgePassData_t*)passNode->val;
        addSingleItemToMenu(spt->menu, spd->key);
        passNode = passNode->next;
    }

    // Initialize renderer
    spt->renderer = initMenuMegaRenderer(NULL, NULL, NULL);
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void swadgePassTestExitMode(void)
{
    // Free the menu
    deinitMenu(spt->menu);
    deinitMenuMegaRenderer(spt->renderer);

    // Free the swadgePasses
    freeSwadgePasses(&spt->swadgePasses);

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
        if (NULL == spt->currSpd)
        {
            // Not showing individual data, show the menu
            spt->menu = menuButton(spt->menu, evt);
        }
        else
        {
            // Showing individual data
            if (evt.down)
            {
                if (PB_A == evt.button)
                {
                    // Toggle is Used
                    bool isUsed = isPacketUsedByMode(spt->currSpd, &swadgePassTestMode);
                    setPacketUsedByMode(spt->currSpd, &swadgePassTestMode, !isUsed);
                }
                else if (PB_B == evt.button)
                {
                    // return to menu
                    spt->currSpd = NULL;
                }
            }
        }
    }

    if (NULL == spt->currSpd)
    {
        // Draw menu
        drawMenuMega(spt->menu, spt->renderer, elapsedUs);
    }
    else
    {
        font_t* font = spt->renderer->menuFont;
        int16_t xOff = 20;
        int16_t yOff = 20;

        // Show key
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c111);
        char buf[1024];
        sprintf(buf, "MAC: %s", spt->currSpd->key);
        drawText(font, c555, buf, xOff, yOff);
        yOff += font->height + 2;

        // Show if it's used or not
        bool isUsed = isPacketUsedByMode(spt->currSpd, &swadgePassTestMode);
        sprintf(buf, "%sUsed", isUsed ? "" : "Not ");
        drawText(font, c555, buf, xOff, yOff);
        yOff += font->height + 2;

        // Show raw data
        memset(buf, 0, sizeof(buf));
        uint8_t* rawData = (uint8_t*)&spt->currSpd->data.packet;
        for (int16_t idx = 0; idx < sizeof(swadgePassPacket_t); idx++)
        {
            char tmp[16];
            sprintf(tmp, "%02X ", rawData[idx]);
            strcat(buf, tmp);
        }
        drawTextWordWrap(font, c555, buf, &xOff, &yOff, TFT_WIDTH - 20, TFT_HEIGHT);
    }
}

/**
 * @brief A callback which is called when a menu changes or items are selected
 * @param label A pointer to the label which was selected or scrolled to
 * @param selected true if the item was selected with the A button, false if it was scrolled to
 * @param value If a settings item was selected or scrolled, this is the new value for the setting
 * @return true to go up a menu level, false to remain here
 */
bool swadgePassTestMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        // Iterate through all the nodes
        node_t* passNode = spt->swadgePasses.first;
        while (passNode)
        {
            // Check if the label matches
            swadgePassData_t* spd = (swadgePassData_t*)passNode->val;
            if (!strcmp(label, spd->key))
            {
                spt->currSpd = spd;
                return false;
            }
            else
            {
                // Iterate to the next
                passNode = passNode->next;
            }
        }
    }
    return false;
}