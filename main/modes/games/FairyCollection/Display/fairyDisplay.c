//==============================================================================
// Includes
//==============================================================================

#include "fairyDisplay.h"
#include "fairyDraw.h"
#include "swadge.h"

//==============================================================================
// Defines
//==============================================================================

#define NUM_COLS 8

#define BOX_WIDTH 24
#define BOX_X_OFFSET 23
#define BOX_Y_OFFSET 3
#define X_SPACING 6
#define Y_SPACING 6

//==============================================================================
// Function declarations
//==============================================================================

static int menu2DNavigate(buttonEvt_t* evt, int selection, int colLim, int maxVal);

//==============================================================================
// Functions
//==============================================================================

bool fcRunSPField(fcspField_t* fcspf)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        // Selection
        if (evt.down)
        {
            fcspf->fairyIndex = menu2DNavigate(&evt, fcspf->fairyIndex, NUM_COLS, FC_MAX_NUM_FAIRIES);
            if (fcspf->drawingFairy)
            {
                fcspf->drawingFairy = false;
            }
            else if (evt.button & PB_A)
            {
                fcspf->drawingFairy = true;
            }
            else if (evt.button & PB_B)
            {
                return true;
            }
        }
    }
    return false;
}

void fcDrawSPField(fcspField_t* fcspf, font_t* font, int64_t elapsedUs)
{
    if (fcspf->drawingFairy)
    {
        nameData_t nd = {0};
        setUsernameFrom32(&nd, fcspf->spFairies[fcspf->fairyIndex].packedName);
        fcDrawFairyCard(&fcspf->spFairies[fcspf->fairyIndex].pCard, &fcspf->spFairies[fcspf->fairyIndex].fairy, &nd, font);
        return;
    }
    // Draw field
    // - Background
    // - Fairies in a grid

    // FIXME: Test code for MAGCon
    fillDisplayArea(0, 0, 280, 240, c110);
    for (int idx = 0; idx < FC_MAX_NUM_FAIRIES; idx++)
    {
        paletteColor_t col = c050;
        if (idx == fcspf->fairyIndex)
        {
            col = c550;
        } else if (!fcspf->spFairies[idx].pCard.initialized)
        {
            col = c500;
        }
        int xOff = BOX_X_OFFSET + ((idx % NUM_COLS) * (BOX_WIDTH + X_SPACING));
        int yOff = BOX_Y_OFFSET + ((idx / NUM_COLS) * (BOX_WIDTH + Y_SPACING));
        drawRectFilled(xOff, yOff, xOff + BOX_WIDTH, yOff + BOX_WIDTH, col);
    }
}

//==============================================================================
// Static functions
//==============================================================================

static int menu2DNavigate(buttonEvt_t* evt, int selection, int colLim, int maxVal)
{
    int outVal = selection;
    if (evt->button & PB_RIGHT)
    {
        outVal++;
        if (outVal % colLim == 0)
        {
            outVal -= colLim;
        }
        if (outVal >= maxVal)
        {
            outVal = (maxVal / colLim) * colLim;
        }
    }
    else if (evt->button & PB_LEFT)
    {
        outVal--;
        if (outVal == -1 || outVal % colLim == colLim - 1)
        {
            outVal += colLim;
        }
        if (outVal >= maxVal)
        {
            outVal = maxVal - 1;
        }
    }
    else if (evt->button & PB_DOWN)
    {
        outVal += colLim;
        if (outVal >= maxVal)
        {
            outVal %= maxVal;
            outVal += maxVal % colLim;
            if (outVal > colLim - 1)
            {
                outVal -= colLim;
            }
        }
    }
    else if (evt->button & PB_UP)
    {
        outVal -= colLim;
        if (outVal < 0)
        {
            outVal += maxVal;
            outVal -= maxVal % colLim;
            if (outVal + colLim < maxVal)
            {
                outVal += colLim;
            }
        }
    }
    return outVal;
}