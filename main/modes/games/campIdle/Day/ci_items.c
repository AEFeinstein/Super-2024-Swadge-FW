//==============================================================================
// Includes
//==============================================================================

#include "ci_items.h"
#include "macros.h"
#include "swadge.h"

static int loadInvFrom(ci_inventory_t* inv, const ci_item_t* array, int idx_start, int count);

//==============================================================================
// Function Definitions
//==============================================================================

void ci_initInv(ci_inventory_t* inv)
{
    // Get size of all item arrays
    int invSize = ci_getArrayLength(CI_TYPE_ALL);

    // Create inventory of the appropriate size
    inv->inventory = (ci_invItem_t*)heap_caps_calloc(invSize, sizeof(ci_invItem_t), MALLOC_CAP_8BIT);

    // Load values into items
    int start = 0;
    start     = loadInvFrom(inv, ci_badFoodData, start, ci_getArrayLength(CI_BAD_FOOD));
    start     = loadInvFrom(inv, ci_craftedData, start, ci_getArrayLength(CI_CRAFTED));
    start     = loadInvFrom(inv, ci_foodData, start, ci_getArrayLength(CI_FOOD));
    start     = loadInvFrom(inv, ci_foragedData, start, ci_getArrayLength(CI_FORAGED));
    start     = loadInvFrom(inv, ci_healingData, start, ci_getArrayLength(CI_HEALING));

    inv->testImage = heap_caps_calloc(invSize, sizeof(wsg_t), MALLOC_CAP_8BIT);

    for (int i = 0; i < invSize; i++)
    {
        loadWsg(inv->inventory[i].item->image, &inv->testImage[i], true);
        drawWsgSimple(&inv->testImage[i], (i%8) * 32, (i/8) * 32);
    }

    // Sort
}

static int loadInvFrom(ci_inventory_t* inv, const ci_item_t* array, int idx_start, int count)
{
    int retVal = 0;
    for (int idx = idx_start; idx < (idx_start + count); idx++)
    {
        inv->inventory[idx].item       = &array[idx - idx_start];
        inv->inventory[idx].qty        = 1;    // TODO: Load from NVS
        inv->inventory[idx].discovered = true; // TODO: Load from NVS
        retVal                         = idx;
    }
    return retVal;
}