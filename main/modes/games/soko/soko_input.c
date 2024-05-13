#include "soko_input.h"

/**
 * @brief Initialize Input. Does this for every puzzle start, to reset button state.
 * Also where config like dastime is set.
 *
 * @param input
 */
void sokoInitInput(sokoGameplayInput_t* input)
{
    input->dasTime           = 100000;
    input->firstDASTime      = 500000;
    input->DASActive         = false;
    input->prevHoldingDir    = SKD_NONE;
    input->prevBtnState      = 0;
    input->playerInputDeltaX = 0;
    input->playerInputDeltaY = 0;
    input->restartLevel      = false;
    input->exitToOverworld   = false;
    input->undo              = false;
}
/**
 * @brief Input preprocessing turns btnstate into game-logic usable data.
 * Input variables only set on press, as appropriate.
 * Handles DAS, settings, etc.
 * Called once a frame before game loop.
 *
 * @param input
 */
void sokoPreProcessInput(sokoGameplayInput_t* input, int64_t elapsedUs)
{
    uint16_t btn = input->btnState;
    // reset output data.
    input->playerInputDeltaY = 0;
    input->playerInputDeltaX = 0;

    // Non directional buttons
    if ((btn & PB_B) && !(input->prevBtnState & PB_B))
    {
        input->restartLevel = true;
    }else{
        input->restartLevel = false;
    }

    if((btn & PB_A) && !(input->prevBtnState & PB_A))
    {
        input->undo = true;
    }else{
        input->undo = false;
    }

    if ((btn & PB_START) && !(input->prevBtnState & PB_START))
    {
        input->exitToOverworld = true;
    }else{
        input->exitToOverworld = false;
    }

    // update holding direction
    if ((btn & PB_UP) && !(btn & 0b1110))
    {
        input->holdingDir = SKD_UP;
    }
    else if ((btn & PB_DOWN) && !(btn & 0b1101))
    {
        input->holdingDir = SKD_DOWN;
    }
    else if ((btn & PB_LEFT) && !(btn & 0b1011))
    {
        input->holdingDir = SKD_LEFT;
    }
    else if ((btn & PB_RIGHT) && !(btn & 0b0111))
    {
        input->holdingDir = SKD_RIGHT;
    }
    else
    {
        input->holdingDir        = SKD_NONE;
        input->DASActive         = false;
        input->timeHeldDirection = 0; // reset when buttons change or multiple buttons.
    }

    // going from one button to another without letting go could cheese DAS.
    if (input->holdingDir != input->prevHoldingDir)
    {
        input->DASActive         = false;
        input->timeHeldDirection = 0;
    }

    // increment DAS time.
    if (input->holdingDir != SKD_NONE)
    {
        input->timeHeldDirection += elapsedUs;
    }

    // two cases when DAS gets triggered: initial and every one after the initial.
    bool triggerDAS = false;
    if (input->DASActive == false && input->timeHeldDirection > input->firstDASTime)
    {
        triggerDAS       = true;
        input->DASActive = true;
    }

    if (input->DASActive == true && input->timeHeldDirection > input->dasTime)
    {
        triggerDAS = true;
    }

    if (triggerDAS)
    {
        // reset timer
        input->timeHeldDirection = 0;

        // trigger movement
        // todo: in sokogame i had to write delta to direction. This is basically directionenum to delta, which could be
        // extracted too.
        switch (input->holdingDir)
        {
            case SKD_RIGHT:
                input->playerInputDeltaX = 1;
                break;
            case SKD_LEFT:
                input->playerInputDeltaX = -1;
                break;
            case SKD_UP:
                input->playerInputDeltaY = -1;
                break;
            case SKD_DOWN:
                input->playerInputDeltaY = 1;
                break;
            case SKD_NONE:
            default:
                break;
        }
    }
    else
    { // if !trigger DAS

        // holdingDir is ONLY holding one button. So we use normal buttonstate for taps so we can tap button two before
        // releasing button one.
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

    } // end !triggerDAS

    // do this last
    input->prevBtnState   = btn;
    input->prevHoldingDir = input->holdingDir;
}
