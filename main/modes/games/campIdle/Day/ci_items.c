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
    int invSize = getArrayLength(CI_TYPE_ALL);
    printf("inv size:%d\n", invSize);
    
    // Create inventory of the appropriate size
    // Load values into struct from NVS
}