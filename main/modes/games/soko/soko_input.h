#include "swadge2024.h"

// there is a way to set clever ints here such that we can super quickly convert to dx and dy with bit ops. I'll think
// it through eventually.
typedef enum
{
    SKD_UP,
    SKD_DOWN,
    SKD_RIGHT,
    SKD_LEFT,
    SKD_NONE
} sokoDirection_t;

typedef struct
{
    // input input data.
    uint16_t btnState; ///< The button state. Provided to input For PreProcess.

    // input meta data. Used by PreProcess.
    uint16_t prevBtnState;          ///< The button state from the previous frame.
    sokoDirection_t holdingDir;     ///< What direction we are holding down.
    sokoDirection_t prevHoldingDir; ///< What direction we are holding down.
    uint64_t timeHeldDirection;     ///< The amount of time we have been holding a single button down. Used for DAS.
    bool DASActive; ///< If DAS has begun. User may be holding before first DAS, this is false. After first, it becomes
                    ///< true.
    uint64_t dasTime;      ///< How many microseconds before DAS starts
    uint64_t firstDASTime; ///< how many microseconds after DAS has started before the next DAS

    // input output data. ie: usable Gameplay data.
    // todo: use Direction in input
    int playerInputDeltaX;
    int playerInputDeltaY;
    bool undo;
    bool restartLevel;
    bool exitToOverworld;

} sokoGameplayInput_t;

void sokoInitInput(sokoGameplayInput_t*);
void sokoPreProcessInput(sokoGameplayInput_t*, int64_t);
