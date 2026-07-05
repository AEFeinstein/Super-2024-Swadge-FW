//==============================================================================
// Includes
//==============================================================================

#include "ci_items.h"
#include "macros.h"
#include "swadge.h"

//==============================================================================
// Function Definitions
//==============================================================================

void ci_initInv(ci_inventory_t* inv)
{
    // Get size of all item arrays
    int invSize = ci_getArrayLength(CI_TYPE_ALL);
    printf("inv size:%d\n", invSize);
    
    // Create inventory of the appropriate size
    inv->inventory = (ci_item_t*)heap_caps_calloc(invSize, sizeof(ci_item_t), MALLOC_CAP_8BIT);

    // Load values into items
    int idx = 0;
    for (idx; idx < invSize; idx++)
    {
        
    }

    // Sort 

    // Load values into struct from NVS
    
}