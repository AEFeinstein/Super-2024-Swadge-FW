//==============================================================================
// Includes
//==============================================================================

#include "fairyCreation.h"

// Fairy
#include "fairyCreationData.h"
#include "fairyDraw.h"

// Swadge
#include "swadge.h"
#include "nameList.h"

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
            if (fcdd->displayFairy)
            {
                fcdd->displayFairy = false;
            }
            else if (evt.button & PB_UP)
            {
                fcdd->optionSelection--;
                if (fcdd->optionSelection < 0)
                {
                    fcdd->optionSelection = FC_OPT_COUNT - 1;
                }
            }
            else if (evt.button & PB_DOWN)
            {
                fcdd->optionSelection++;
                if (fcdd->optionSelection >= FC_OPT_COUNT)
                {
                    fcdd->optionSelection = 0;
                }
            }
            else if (evt.button & PB_LEFT)
            {
                fcdd->options[fcdd->optionSelection]--;
                if (fcdd->options[fcdd->optionSelection] < 0)
                {
                    fcdd->options[fcdd->optionSelection] = fairyMaxList[fcdd->optionSelection] - 1;
                }
            }
            else if (evt.button & PB_RIGHT)
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
                fcdd->displayFairy = true;
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
    if (fcdd->displayFairy)
    {
        nameData_t* nd = getSystemUsername();
        fcDrawFairyCard(fcdd->card, fcdd->fairy, nd, font);
        return;
    }
    fillDisplayArea(0, 0, 280, 240, c000);
    drawText(font, c444, fcOptionTypes[fcdd->optionSelection], 12, 32);
    switch (fcdd->optionSelection)
    {
        case FC_OPT_YEAR:
        {
            char buffer[32];
            if (fcdd->options[fcdd->optionSelection] == 15)
            {
                snprintf(buffer, sizeof(buffer) - 1, "15+");
            }
            else
            {
                snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, fcdd->options[fcdd->optionSelection] + 1);
            }
            drawText(font, c555, buffer, 12, 64);
            break;
        }
        case FC_OPT_ATTENDEE:
        {
            drawText(font, c555, fcAttendeeText[fcdd->options[fcdd->optionSelection]], 12, 64);
            break;
        }
        case FC_OPT_CARD:
        {
            drawText(font, c555, fcCardText[fcdd->options[fcdd->optionSelection]], 12, 64);
            break;
        }
        case FC_OPT_TEAM:
        {
            drawText(font, c555, fcTeamText[fcdd->options[fcdd->optionSelection]], 12, 64);
            break;
        }
        case FC_OPT_SHAPE:
        {
            drawText(font, c555, fcShapeText[fcdd->options[fcdd->optionSelection]], 12, 64);
            break;
        }
        case FC_OPT_CHARM:
        {
            drawText(font, c555, fcCharmText[fcdd->options[fcdd->optionSelection]], 12, 64);
            break;
        }
        case FC_OPT_FILLING:
        {
            drawText(font, c555, fcFillingText[fcdd->options[fcdd->optionSelection]], 12, 64);
            break;
        }
        case FC_OPT_PEDESTAL:
        {
            drawText(font, c555, fcPedestalText[fcdd->options[fcdd->optionSelection]], 12, 64);
            break;
        }
        case FC_OPT_COLOR:
        {
            drawText(font, c555, fcColorsText[fcdd->options[fcdd->optionSelection]], 12, 64);
            break;
        }
        case FC_OPT_WING:
        {
            drawText(font, c555, fcWingText[fcdd->options[fcdd->optionSelection]], 12, 64);
            break;
        }
        case FC_OPT_BALL:
        {
            drawText(font, c555, fcBallText[fcdd->options[fcdd->optionSelection]], 12, 64);
            break;
        }
        case FC_OPT_AURA:
        {
            drawText(font, c555, fcAuraText[fcdd->options[fcdd->optionSelection]], 12, 64);
            break;
        }
        default:
        {
            break;
        }
    }
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Option Idx: %" PRId16, fcdd->optionSelection);
    drawText(font, c055, buffer, 12, TFT_HEIGHT - 60);
    snprintf(buffer, sizeof(buffer) - 1, "Option Setting: %" PRId16, fcdd->options[fcdd->optionSelection]);
    drawText(font, c055, buffer, 12, TFT_HEIGHT - 40);
    drawText(font, c550, "Press A to save", 12, TFT_HEIGHT - 20);
}

//==============================================================================
// Static Functions
//==============================================================================

static void structToArray(fcCreationData_t* fcdd)
{
    fcdd->options[FC_OPT_YEAR]     = fcdd->card->years;
    fcdd->options[FC_OPT_ATTENDEE] = fcdd->card->attendeeType;
    fcdd->options[FC_OPT_CARD]     = fcdd->card->cardBG;
    fcdd->options[FC_OPT_TEAM]     = fcdd->card->team;
    fcdd->options[FC_OPT_SHAPE]    = fcdd->fairy->shape;
    fcdd->options[FC_OPT_CHARM]    = fcdd->fairy->charm;
    fcdd->options[FC_OPT_FILLING]  = fcdd->fairy->filling;
    fcdd->options[FC_OPT_PEDESTAL] = fcdd->fairy->pedestal;
    fcdd->options[FC_OPT_COLOR]    = fcdd->fairy->color;
    fcdd->options[FC_OPT_WING]     = fcdd->fairy->wing;
    fcdd->options[FC_OPT_BALL]     = fcdd->fairy->ball;
    fcdd->options[FC_OPT_AURA]     = fcdd->fairy->aura;
}

static void arrayToStruct(fcCreationData_t* fcdd)
{
    fcdd->card->years        = fcdd->options[FC_OPT_YEAR];
    fcdd->card->attendeeType = fcdd->options[FC_OPT_ATTENDEE];
    fcdd->card->cardBG       = fcdd->options[FC_OPT_CARD];
    fcdd->card->team         = fcdd->options[FC_OPT_TEAM];
    fcdd->fairy->shape       = fcdd->options[FC_OPT_SHAPE];
    fcdd->fairy->charm       = fcdd->options[FC_OPT_CHARM];
    fcdd->fairy->filling     = fcdd->options[FC_OPT_FILLING];
    fcdd->fairy->pedestal    = fcdd->options[FC_OPT_PEDESTAL];
    fcdd->fairy->color       = fcdd->options[FC_OPT_COLOR];
    fcdd->fairy->wing        = fcdd->options[FC_OPT_WING];
    fcdd->fairy->ball        = fcdd->options[FC_OPT_BALL];
    fcdd->fairy->aura        = fcdd->options[FC_OPT_AURA];
}

static void saveFairyToNVS(fcCreationData_t* fcdd)
{
    fcdd->card->initialized = true;
    arrayToStruct(fcdd);
    size_t size = sizeof(fairy_t);
    writeNamespaceNvsBlob(nvsStrs[FC_NAMESPACE], nvsStrs[FC_USER_FAIRY], fcdd->fairy, size);
    size = sizeof(profileCard_t);
    writeNamespaceNvsBlob(nvsStrs[FC_NAMESPACE], nvsStrs[FC_USER_CARD], fcdd->card, size);
}