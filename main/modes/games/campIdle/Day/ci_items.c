//==============================================================================
// Includes
//==============================================================================

#include "ci_items.h"
#include "macros.h"
#include "swadge.h"

//==============================================================================
// Static Functions
//==============================================================================

static int loadInvFrom(ci_inventory_t* inv, const ci_item_t* array, int idx_start, int count);
// static int sort(const void *a, const void *b);

//==============================================================================
// Function Definitions
//==============================================================================

void ci_initInv(ci_inventory_t* inv)
{
    // Get size of all item arrays
    int invSize = ci_getArrayLength(CI_TYPE_ALL);

    // Create inventory of the appropriate size
    inv->itemList = (ci_invItem_t*)heap_caps_calloc(invSize, sizeof(ci_invItem_t), MALLOC_CAP_8BIT);

    // Load values into items
    int start = 0;
    start     = loadInvFrom(inv, ci_badFoodData, start, ci_getArrayLength(CI_BAD_FOOD));
    start     = loadInvFrom(inv, ci_craftedData, start, ci_getArrayLength(CI_CRAFTED));
    start     = loadInvFrom(inv, ci_foodData, start, ci_getArrayLength(CI_FOOD));
    start     = loadInvFrom(inv, ci_foragedData, start, ci_getArrayLength(CI_FORAGED));
    start     = loadInvFrom(inv, ci_healingData, start, ci_getArrayLength(CI_HEALING));

    // Sort
    // qsort(inv->itemList, invSize, sizeof(inv->itemList[0]), sort);
}

void ci_deInitInv(ci_inventory_t* inv)
{
    // TODO: Save to NVS before freeing
    free(inv->itemList);
}

void ci_loadInv(ci_inventory_t* inv)
{
    
}

void ci_saveInv(ci_inventory_t* inv)
{

}

//==============================================================================
// Static Function Definitions
//==============================================================================

static int loadInvFrom(ci_inventory_t* inv, const ci_item_t* array, int idx_start, int count)
{
    int retVal = 0;
    for (int idx = idx_start; idx < (idx_start + count); idx++)
    {
        inv->itemList[idx].item       = &array[idx - idx_start];
        inv->itemList[idx].qty        = 1;    // TODO: Load from NVS
        inv->itemList[idx].discovered = true; // TODO: Load from NVS
        retVal                        = idx + 1;
    }
    return retVal;
}

/* static int sort(const void *a, const void *b)
{
    const char* aa = ((ci_item_t*)a)->title;
    const char* bb = ((ci_item_t*)b)->title;
    return strcmp(aa, bb);
} */