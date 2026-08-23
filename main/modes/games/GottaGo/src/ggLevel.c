//==============================================================================
// Includes
//==============================================================================

#include "ggLevel.h"
#include "ggScoring.h"

//==============================================================================
// Defines
//==============================================================================

// Timer
#define TIME_LIMIT_DENOM 27

// Urinal count
#define MIN_URINALS                 3
#define MAX_ADDITIONAL_URINALS      (MAX_URINALS - MIN_URINALS)
#define NUM_LEVELS_REQ_ADDTL_URINAL 4
#define MIN_URINAL_OPEN             2
#define GRAFFITI_COUNT              7

// Costs
#define NPC_COST           3
#define HALF_DIVIDER_COST  2
#define NO_DIVIDER_COST    (HALF_DIVIDER_COST + 1)
#define PUDDLE_COST        2
#define URINAL_SIZE_COST   2
#define URINAL_CHANGE_COST 2

// Chances
#define AUTO_FLUSHER_CHANCE 4
#define NPC_INIT_CHANCE     3
#define NPC_CHANGE_CHANCE   2
#define DIVIDER_CHANCE      3
#define PUDDLE_CHANCE       3
#define URINAL_CHANCE       2

// Urinal size
#define URINAL_DEFAULT_H 35

// NPCs
#define HEIGHT_OFFSET 7
#define SKIRT_CHANCE  1
#define SHORTS_CHANCE 4
#define PANTS_CHANCE  15

//==============================================================================
// Function Definitions
//==============================================================================

/**
 * @brief Initialize an NPC to defaults
 *
 * @param n NPC to initialize
 * @param active If they should be active or not
 */
static void ggInitNPC(ggNPC_t* n, bool active);

/**
 * @brief Randomize NPC
 *
 * @param n NPC to initialize
 */
static void ggInitRandomNPC(ggNPC_t* n);

//==============================================================================
// Functions
//==============================================================================

// Initialization
void ggLevelReset(ggData_t* ggd)
{
    // Reset
    ggClearUrinals(ggd);
    // Timer
    ggd->timer = 0;
    ggd->timeLimit -= ggd->timeLimit / TIME_LIMIT_DENOM;
    // Max number of toilets
    ggd->numActive = MIN_URINALS
                     + (esp_random() % (MIN(ggd->numLevels / NUM_LEVELS_REQ_ADDTL_URINAL, MAX_ADDITIONAL_URINALS) + 1));
    // Add points
    int points = ggd->numLevels;
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        if (esp_random() % AUTO_FLUSHER_CHANCE == 0)
        {
            points++;
            ggd->urinals[idx].autoFlush = 1;
        }
    }
    // Attempt to add NPC
    int maxNPCs = ggd->numActive - MIN_URINAL_OPEN;
    for (int idx = 0; idx < maxNPCs; idx++)
    {
        if (points < NPC_COST)
        {
            continue;
        }
        int chance = esp_random() % NPC_INIT_CHANCE; // 67/33 chance to appear
        if (chance != 0)
        {
            points -= NPC_COST;
            ggNPC_t* n = &ggd->urinals[esp_random() % ggd->numActive].npc;
            ggInitRandomNPC(n);
        }
        // Chance to change attributes
        if (points > 0 && esp_random() % NPC_CHANGE_CHANCE == 0)
        {
            points--;
            switch (esp_random() % 5)
            {
                case 0:
                {
                    ggd->urinals[chance].npc.shirt = 0;
                    break;
                }
                case 1:
                {
                    ggd->urinals[chance].npc.pants = GG_NO_PANTS;
                    break;
                }
                case 2:
                {
                    ggd->urinals[chance].npc.pants = GG_DOWN;
                    break;
                }
                case 3:
                {
                    ggd->urinals[chance].npc.pants = GG_UNDERWEAR;
                    break;
                }
                case 4:
                {
                    ggd->urinals[chance].npc.stink = 1;
                    break;
                }
            }
        }
    }
    // Attempt to modify partitions
    for (int idx = 0; idx < (ggd->numActive / 2); idx++)
    {
        if (points >= HALF_DIVIDER_COST && esp_random() % DIVIDER_CHANCE == 0)
        {
            int selection                   = esp_random() % ggd->numActive;
            ggd->urinals[selection].divider = 1 + (esp_random() % (GG_DIV_COUNT - 1));
            if (ggd->urinals[selection].divider == GG_DIV_NONE && points >= NO_DIVIDER_COST)
            {
                points -= NO_DIVIDER_COST;
            }
            else
            {
                points -= HALF_DIVIDER_COST;
            }
        }
    }
    // Attempt to make puddles
    for (int idx = 0; idx < (ggd->numActive / 2); idx++)
    {
        if (points >= PUDDLE_COST && esp_random() % PUDDLE_CHANCE == 0)
        {
            ggd->urinals[esp_random() % ggd->numActive].puddle = 1 + (esp_random() % (GG_PEE_COUNT - 1));
            points -= PUDDLE_COST;
        }
    }
    // Urinal size
    if (points >= URINAL_SIZE_COST * 2)
    {
        points -= URINAL_SIZE_COST;
        ggd->urinals[esp_random() % ggd->numActive].small = 1;
    }
    /* if (esp_random() % 100 == 0)
    {
        ggd->urinals[esp_random() % ggd->numActive].height += 15;
    } */
    // Adjust urinal
    while (points > 0)
    {
        if (points >= URINAL_CHANGE_COST && esp_random() % URINAL_CHANCE == 0)
        {
            points -= URINAL_CHANGE_COST;
            int selection = esp_random() % ggd->numActive;
            switch (esp_random() % GG_MAJOR_COUNT)
            {
                case GG_BROKEN_BOWL:
                {
                    ggd->urinals[selection].brokenBowl = 1;
                    break;
                }
                case GG_BROKEN_DRAIN:
                {
                    ggd->urinals[selection].brokenDrain = 1;
                    break;
                }
                case GG_PLUGGED:
                {
                    ggd->urinals[selection].pluggedDrain = 1;
                    break;
                }
                case GG_OOO:
                {
                    ggd->urinals[selection].outOfOrder = 1;
                    break;
                }
            }
        }
        else if (esp_random() % URINAL_CHANCE == 0)
        {
            int selection = esp_random() % ggd->numActive;
            int result = esp_random() % GRAFFITI_COUNT + GG_CRACK_COUNT + 1; // (Num graffiti + num cracks + waterleak)

            switch (result)
            {
                default:
                {
                    ggd->urinals[selection].graffiti = result;
                    break;
                }
                case GRAFFITI_COUNT:
                {
                    ggd->urinals[selection].waterLeak = 1;
                    break;
                }
                case GRAFFITI_COUNT + GG_CRACK_1:
                {
                    ggd->urinals[selection].cracks = GG_CRACK_1;
                    break;
                }
                case GRAFFITI_COUNT + GG_CRACK_2:
                {
                    ggd->urinals[selection].cracks = GG_CRACK_2;
                    break;
                }
                case GRAFFITI_COUNT + GG_CRACK_3:
                {
                    ggd->urinals[selection].cracks = GG_CRACK_3;
                    break;
                }
            }
        }
        else
        {
            points -= URINAL_CHANGE_COST;
        }
    }

    // Set selection to a valid option
    ggd->selection = ggd->numActive / 2;
    while (ggd->urinals[ggd->selection].npc.active)
    {
        ggd->selection++;
    }
    if (ggd->selection >= ggd->numActive)
    {
        ESP_LOGE("GG", "Selection >= active. May cause memory leaks");
    }
}

void ggClearUrinals(ggData_t* ggd)
{
    for (int idx = 0; idx < MAX_URINALS; idx++)
    {
        ggUrinal_t* u   = &ggd->urinals[idx];
        u->autoFlush    = 0;
        u->graffiti     = 0;
        u->cracks       = 0;
        u->waterLeak    = 0;
        u->brokenDrain  = 0;
        u->outOfOrder   = 0;
        u->brokenBowl   = 0;
        u->pluggedDrain = 0;
        u->divider      = 0;
        u->small        = 0;
        u->height       = URINAL_DEFAULT_H;
        u->puddle       = GG_PEE_NONE;
        ggInitNPC(&ggd->urinals[idx].npc, 0);
    }
}

// End
bool ggEndLevel(ggData_t* ggd)
{
    // Bail if this is a loss
    if (ggd->hasLost)
    {
        return false;
    }
    // Determine if the player gets another use of the stall
    if (ggd->numLevels > ggd->stallReq)
    {
        ggd->stallUses++;
        ggd->stallReq *= 2;
        ggd->stallReq += ggd->stallReq / 2;
    }
    // Move on
    return true;
}

//==============================================================================
// Static Functions
//==============================================================================

static void ggInitNPC(ggNPC_t* n, bool active)
{
    n->active     = active;
    n->small      = 0;
    n->shirt      = 1;
    n->pants      = GG_PANTS;
    n->shoes      = 1;
    n->stink      = 0;
    n->skinColor  = 0;
    n->shirtColor = 0;
    n->pantsColor = 0;
    n->shoeColor  = 0;
    n->randOffset = 0;
}

static void ggInitRandomNPC(ggNPC_t* n)
{
    n->active     = 1;
    n->skinColor  = esp_random() % 7;
    n->shirtColor = esp_random() % 7;
    n->pantsColor = esp_random() % 7;
    n->shoeColor  = esp_random() % 4;
    n->randOffset = (HEIGHT_OFFSET + 1) - (esp_random() % ((HEIGHT_OFFSET * 2) + 1));
    switch (esp_random() % (SKIRT_CHANCE + SHORTS_CHANCE + PANTS_CHANCE))
    {
        case 0:
        {
            n->pants = GG_SKIRT;
            break;
        }
        case 1:
        case 2:
        case 3:
        case 4:
        {
            n->pants = GG_SHORTS;
        }
        default:
        {
            break;
        }
    }
}
