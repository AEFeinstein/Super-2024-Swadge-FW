//==============================================================================
// Includes
//==============================================================================

#include "artillery_help.h"

//==============================================================================
// Static Const Variables
//==============================================================================

static const helpPage_t helpPages[] = {
    {
        .title = "Page 1",
        .text  = "This is the first page of help text! Welcome!",
    },
    {
        .title = "Page 2",
        .text  = "This is the second page of help text. It probably has more details or something.",
    },
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 * @param ad
 */
void artilleryHelpInit(artilleryData_t* ad)
{
    ad->help = initHelpScreen(ad->blankMenu, ad->mRenderer, helpPages, ARRAY_SIZE(helpPages));
}

/**
 * @brief TODO doc
 *
 * @param ad
 */
void artilleryHelpDeinit(artilleryData_t* ad)
{
    deinitHelpScreen(ad->help);
}

/**
 * @brief TODO doc
 *
 * @param ad
 * @param evt
 */
void artilleryHelpInput(artilleryData_t* ad, buttonEvt_t* evt)
{
    if (buttonHelp(ad->help, evt))
    {
        // Exit the help menu
        ad->mState = AMS_MENU;
    }
}

/**
 * @brief TODO doc
 *
 * @param ad
 * @param elapsedUs
 */
void artilleryHelpLoop(artilleryData_t* ad, uint32_t elapsedUs)
{
    drawHelp(ad->help, elapsedUs);
}
