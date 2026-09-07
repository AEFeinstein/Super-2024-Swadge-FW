//==============================================================================
// Include
//==============================================================================

#include "ci_helpers.h"

//==============================================================================
// Functions
//==============================================================================

int ciMenu2DNavigate(buttonEvt_t* evt, int selection, int colLim, int maxVal)
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