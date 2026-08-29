//==============================================================================
// Includes
//==============================================================================

#include "ci_items.h"
#include "macros.h"
#include "swadge.h"

//==============================================================================
// Defines
//==============================================================================

// Drawing
#define ITEM_ICON_WIDTH  40
#define ITEM_ICON_HEIGHT 54

//==============================================================================
// Function Definitions
//==============================================================================

//==============================================================================
// Functions
//==============================================================================

// Initialization

void ciInitInventory(ciCampData_t* ccd)
{
    // Load static data
    size_t blobSize = sizeof(ccd->qtys);
    if (!readNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_INVENTORY], &ccd->qtys, &blobSize))
    {
        // First run initialization
        // NOTE: Should be all 0s
        writeNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_INVENTORY], &ccd->qtys, blobSize);
    }
    ccd->itemImages = heap_caps_calloc(ciGetArrayLength(), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ciGetArrayLength(); idx++)
    {
        loadWsg(ciItemData[idx].image, &ccd->itemImages[idx], true);
    }
}

void ciFreeInventory(ciCampData_t* ccd)
{
    for (int idx = 0; idx < ciGetArrayLength(); idx++)
    {
        freeWsg(&ccd->itemImages[idx]);
    }
    free(ccd->itemImages);
}

void ciDrawItem(ciCampData_t* ccd)
{
    // Draw shadowbox

    // Draw Icon

    // Draw title

    // Draw description

    // Draw info (locations, weight, etc.)
}

void ciDrawItemIcon(ciCampData_t* ccd, int idx, int xStart, int yStart)
{
    drawRectFilled(xStart, yStart, xStart + ITEM_ICON_WIDTH, yStart + ITEM_ICON_HEIGHT, c111);
    drawWsgSimple(&ccd->itemImages[idx], xStart + 4, yStart + 4);
    //  Draw qty
}

//==============================================================================
// Static Function Definitions
//==============================================================================

