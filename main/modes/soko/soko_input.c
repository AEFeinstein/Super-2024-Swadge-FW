#include "soko_input.h"

/**
 * @brief Input preprocessing turns btnstate into game-logic usable data.
 * Input variables only set on press, as appropriate.
 * Handles DAS, settings, etc.
 * Called once a frame before game loop.
 * 
 * @param input 
 */
void sokoPreProcessInput(sokoGameplayInput_t* input)
{
    input->playerInputDeltaY = 0;
    input->playerInputDeltaX = 0;

    if (input->btnState & PB_UP && !(input->prevBtnState & PB_UP))
    {
        input->playerInputDeltaY = -1;
    }
    else if (input->btnState & PB_DOWN && !(input->prevBtnState & PB_DOWN))
    {
        input->playerInputDeltaY = 1;
    }
    else if (input->btnState & PB_LEFT && !(input->prevBtnState & PB_LEFT))
    {
        input->playerInputDeltaX = -1;
    }
    else if (input->btnState & PB_RIGHT && !(input->prevBtnState & PB_RIGHT))
    {
        input->playerInputDeltaX = 1;
    }


    //do this last
    input->prevBtnState = input->btnState;
}
