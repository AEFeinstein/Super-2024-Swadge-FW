//==============================================================================
// Includes
//==============================================================================

#include "ci_draw.h"
#include "ci_items.h"

//==============================================================================
// Functions
//==============================================================================

void ciDrawSplash(ciCampData_t* ccd)
{
    /* for (int x = 0; x < 7; x++)
    {
        for (int y = 0; y < 4; y++)
        {
            ciDrawItemIcon(ccd, (x*7) + y, 40 * x, 54 * y, false);
        }
    } */
    ciDrawItemPanel(ccd, ccd->selection);
}

void ciDrawMenu(ciCampData_t* ccd)
{
}