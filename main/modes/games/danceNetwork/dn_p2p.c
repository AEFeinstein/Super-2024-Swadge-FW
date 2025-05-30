#include "dn_p2p.h"

//==============================================================================
// Variables
//==============================================================================

// static const char* conStartedStrs[] = {
//     "Connection Started.",
//     "Hold two Swadges",
//     "close together.",
// };

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Handle input when showing the connection UI
 *
 * @param data The entire game state
 * @param evt The button event
 */
void dn_HandleConnectingInput(dn_gameData_t* data, buttonEvt_t* evt)
{
    if (evt->down && PB_B == evt->button)
    {
        // Cancel the connection and return to the main menu
        p2pDeinit(&data->p2p);
        dn_ShowUi(UI_MENU);
    }
}