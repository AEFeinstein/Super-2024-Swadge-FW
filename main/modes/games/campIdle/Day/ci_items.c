//==============================================================================
// Includes
//==============================================================================

#include "ci_items.h"
#include "macros.h"

//==============================================================================
// Function Definitions
//==============================================================================

void ci_initInv(ci_inventory_t* inv)
{
    // Get size of all item arrays
    int invSize = ARRAY_SIZE(ci_badFoodData) + ARRAY_SIZE(ci_craftedData) + ARRAY_SIZE(ci_foodData)
                  + ARRAY_SIZE(ci_foragedData) + ARRAY_SIZE(ci_healingData);
    int x = 10;
    x += invSize;
    // Create inventory of the appropriate size
    // Load values into struct from NVS
}