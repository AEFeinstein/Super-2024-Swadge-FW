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
    fillDisplayArea(0,0,TFT_WIDTH, TFT_HEIGHT, c000);
    drawText(&ccd->largeText, c555, "Splash screen", 32, 32);
}

void ciDrawMenu(ciCampData_t* ccd)
{
    fillDisplayArea(0,0,TFT_WIDTH, TFT_HEIGHT, c000);
    drawText(&ccd->largeText, c555, "Menu screen", 32, 32);
    // Draw entries
    
}