//==============================================================================
// Includes
//==============================================================================

#include "fairyCreation.h"

// Fairy
#include "fairyCreationData.h"

// Swadge
#include "swadge.h"

//==============================================================================
// Consts
//==============================================================================



//==============================================================================
// Function Definitions
//==============================================================================

// Helper
static void structToArray(fcCreationData_t* fcdd);
static void arrayToStruct(fcCreationData_t* fcdd);
static void saveFairyToNVS(fcCreationData_t* fcdd);

//==============================================================================
// Functions
//==============================================================================

void fcInitCreation(fcCreationData_t* fcdd, fairy_t* fairy, profileCard_t* card)
{
    fcdd->fairy = fairy;
    fcdd->card  = card;
    structToArray(fcdd);
}

bool fcRunCreation(fcCreationData_t* fcdd)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button & PB_UP)
            {
                fcdd->optionSelection--;
                if (fcdd->optionSelection < 0)
                {
                    fcdd->optionSelection = FC_NUM_OPTIONS - 1;
                }
            }
            else if (evt.button & PB_DOWN)
            {
                fcdd->optionSelection++;
                if (fcdd->optionSelection > FC_NUM_OPTIONS)
                {
                    fcdd->optionSelection = 0;
                }
            }
            else if (evt.button & PB_RIGHT)
            {
                fcdd->options[fcdd->optionSelection]--;
                if (fcdd->options[fcdd->optionSelection] < 0)
                {
                    fcdd->options[fcdd->optionSelection] = fairyMaxList[fcdd->optionSelection] - 1;
                }
            }
            else if (evt.button & PB_LEFT)
            {
                fcdd->options[fcdd->optionSelection]++;
                if (fcdd->options[fcdd->optionSelection] >= fairyMaxList[fcdd->optionSelection])
                {
                    fcdd->options[fcdd->optionSelection] = 0;
                }
            }
            else if (evt.button & PB_A)
            {
                saveFairyToNVS(fcdd);
            }
            else if (evt.button & PB_B)
            {
                return true;
            }
        }
    }
    return false;
}

void fcDrawCreation(fcCreationData_t* fcdd, font_t* font)
{
    fillDisplayArea(0,0,280,240,c000);
    
    drawText(font, c550, "Press A to save", 12, TFT_HEIGHT - 20);
}

//==============================================================================
// Static Functions
//==============================================================================

static void structToArray(fcCreationData_t* fcdd)
{
    fcdd->options[0]  = fcdd->fairy->shape;
    fcdd->options[1]  = fcdd->fairy->charm;
    fcdd->options[2]  = fcdd->fairy->filling;
    fcdd->options[3]  = fcdd->fairy->pedestal;
    fcdd->options[4]  = fcdd->fairy->color;
    fcdd->options[5]  = fcdd->fairy->wing;
    fcdd->options[6]  = fcdd->fairy->ball;
    fcdd->options[7]  = fcdd->fairy->aura;
    fcdd->options[8]  = fcdd->card->years;
    fcdd->options[9]  = fcdd->card->attendeeType;
    fcdd->options[10] = fcdd->card->cardBG;
    fcdd->options[11] = fcdd->card->team;
}

static void arrayToStruct(fcCreationData_t* fcdd)
{
    fcdd->fairy->shape       = fcdd->options[0];
    fcdd->fairy->charm       = fcdd->options[1];
    fcdd->fairy->filling     = fcdd->options[2];
    fcdd->fairy->pedestal    = fcdd->options[3];
    fcdd->fairy->color       = fcdd->options[4];
    fcdd->fairy->wing        = fcdd->options[5];
    fcdd->fairy->ball        = fcdd->options[6];
    fcdd->fairy->aura        = fcdd->options[7];
    fcdd->card->years        = fcdd->options[8];
    fcdd->card->attendeeType = fcdd->options[9];
    fcdd->card->cardBG       = fcdd->options[10];
    fcdd->card->team         = fcdd->options[11];
}

static void saveFairyToNVS(fcCreationData_t* fcdd)
{
    arrayToStruct(fcdd);
    size_t size = sizeof(fairy_t);
    writeNamespaceNvsBlob(nvsStrs[FC_NAMESPACE], nvsStrs[FC_USER_FAIRY], fcdd->fairy, size);
    size = sizeof(profileCard_t);
    writeNamespaceNvsBlob(nvsStrs[FC_NAMESPACE], nvsStrs[FC_USER_CARD], fcdd->card, size);
}