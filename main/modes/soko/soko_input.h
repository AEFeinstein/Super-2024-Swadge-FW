#include "swadge2024.h"

typedef struct
{
    //player input
    uint16_t btnState;      ///< The button state
    uint16_t prevBtnState;

    //we moved. 
    int playerInputDeltaX;
    int playerInputDeltaY;

} sokoGameplayInput_t;

void sokoPreProcessInput(sokoGameplayInput_t*);
