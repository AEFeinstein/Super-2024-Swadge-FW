//==============================================================================
// Includes
//==============================================================================

#include "fairyDraw.h"
#include "fill.h"
#include "nameList.h"

#include <stdio.h>

//==============================================================================
// Function
//==============================================================================

void fcDrawFairy(fairy_t* fairy, int x, int y)
{
    // TODO: Layer in all the images in the correct order:
    // - Pedestal
    // - Filling
    // - Wings
    // - Ball
    // - Aura
    // - Jar
    // - Charm
    // Handle if uninitialized
}

void fcDrawFairyCard(profileCard_t* card, fairy_t* fairy, font_t* font)
{
    // TODO: 
    // Don't draw bg, use whatever is left in buffer
    // Draw card
    // Draw Fairy in top corner
    // Draw text
    // - Color team text appropriately
    // - Years attended
    // - Attendee type
    // - Username
    // If uninitialized, only draw a jar

    // FIXME: Just drawing all of the text until graphics are around
    // Card
    fillDisplayArea(0, 0, 280, 240, c111);
    char buffer[64];
    // Name
    nameData_t* nd = getSystemUsername();
    snprintf(buffer, sizeof(buffer) - 1, "Username: %s", nd->nameBuffer);
    drawText(font, c555, buffer, 12, 12);
    // Years attended
    if (card->years == 15)
    {
        snprintf(buffer, sizeof(buffer) - 1, "%s %s", fcOptionTypes[FC_OPT_YEAR], "15+");
    }
    else
    {
        snprintf(buffer, sizeof(buffer) - 1, "%s %" PRId16, fcOptionTypes[FC_OPT_YEAR], card->years + 1);
    }
    drawText(font, c555, buffer, 12, 24);
    // Attendee
    snprintf(buffer, sizeof(buffer) - 1, "%s %s", fcOptionTypes[FC_OPT_ATTENDEE], fcAttendeeText[card->attendeeType]);
    drawText(font, c555, buffer, 12, 36);
    // Card
    snprintf(buffer, sizeof(buffer) - 1, "%s %s", fcOptionTypes[FC_OPT_CARD], fcCardText[card->cardBG]);
    drawText(font, c555, buffer, 12, 48);
    // Team
    snprintf(buffer, sizeof(buffer) - 1, "%s %s", fcOptionTypes[FC_OPT_TEAM], fcTeamText[card->team]);
    paletteColor_t col = c555;
    switch (card->team)
    {
        case FC_RED:
        {
            col = c500;
            break;
        }
        case FC_YELLOW:
        {
            col = c550;
            break;
        }
        case FC_BLUE:
        {
            col = c005;
            break;
        }
    }
    drawText(font, col, buffer, 12, 60);
    // Shape
    snprintf(buffer, sizeof(buffer) - 1, "%s %s", fcOptionTypes[FC_OPT_SHAPE], fcShapeText[fairy->shape]);
    drawText(font, c555, buffer, 12, 72);
    // Charm
    snprintf(buffer, sizeof(buffer) - 1, "%s %s", fcOptionTypes[FC_OPT_CHARM], fcCharmText[fairy->charm]);
    drawText(font, c555, buffer, 12, 84);
    // Filling
    snprintf(buffer, sizeof(buffer) - 1, "%s %s", fcOptionTypes[FC_OPT_FILLING], fcFillingText[fairy->filling]);
    drawText(font, c555, buffer, 12, 96);
    // Pedestal
    snprintf(buffer, sizeof(buffer) - 1, "%s %s", fcOptionTypes[FC_OPT_PEDESTAL], fcPedestalText[fairy->pedestal]);
    drawText(font, c555, buffer, 12, 108);
    // Colors
    snprintf(buffer, sizeof(buffer) - 1, "%s %s", fcOptionTypes[FC_OPT_COLOR], fcColorsText[fairy->color]);
    drawText(font, c555, buffer, 12, 120);
    // Wing
    snprintf(buffer, sizeof(buffer) - 1, "%s %s", fcOptionTypes[FC_OPT_WING], fcWingText[fairy->wing]);
    drawText(font, c555, buffer, 12, 132);
    // Ball
    snprintf(buffer, sizeof(buffer) - 1, "%s %s", fcOptionTypes[FC_OPT_BALL], fcBallText[fairy->ball]);
    drawText(font, c555, buffer, 12, 144);
    // Aura
    snprintf(buffer, sizeof(buffer) - 1, "%s %s", fcOptionTypes[FC_OPT_AURA], fcAuraText[fairy->aura]);
    drawText(font, c555, buffer, 12, 156);
    // If Uninitialized, note it
}