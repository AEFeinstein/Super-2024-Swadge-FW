//==============================================================================
// Includes
//==============================================================================

#include "fairyCollection.h"

#include "fairyCollectionData.h"

//==============================================================================
// Defines
//==============================================================================

/* #define FAIRY_GRID_SIZE   60
#define FAIRY_GRID_WIDTH  (FAIRY_GRID_SIZE * 8)
#define FAIRY_GRID_HEIGHT (FAIRY_GRID_SIZE * (MAX_NUM_SWADGE_PASSES / 8 + 1)) */

//==============================================================================
// Consts
//==============================================================================

const char fcModeName[] = "Fairy Collection";

static const char* const nvsStrs[] = {
    "fairy-col", "user-fairy", "user-card",
    //"spp-saved",
};

//==============================================================================
// Enum
//==============================================================================

typedef enum
{
    FC_NAMESPACE,
    FC_USER_FAIRY,
    FC_USER_CARD,
    // FC_SPP_SAVED,
} fcNvsKeys_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    fcSPP_t userFairy;
    profileCard_t card;
    savedProfile_t spFairies[MAX_NUM_SWADGE_PASSES];
} fairyCollectionData_t;

//==============================================================================
// Functions declarations
//==============================================================================

static void fcEnterMode(void);
static void fcExitMode(void);
static void fcMainLoop(int64_t elapsedUs);

// Swadgepass
static void fcAddToSwadgePassPacket(struct swadgePassPacket* packet);
static void fcLoadFromSwadgePass(void);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t fairyCollectionMode = {
    .modeName                = fcModeName,
    .wifiMode                = NO_WIFI,
    .overrideUsb             = false,
    .usesAccelerometer       = false,
    .usesThermometer         = false,
    .fnEnterMode             = fcEnterMode,
    .fnExitMode              = fcExitMode,
    .fnMainLoop              = fcMainLoop,
    .overrideSelectBtn       = false,
    .fnAddToSwadgePassPacket = &fcAddToSwadgePassPacket,
};

fairyCollectionData_t* fcd;

//==============================================================================
// Functions
//==============================================================================

static void fcEnterMode(void)
{
    fcd = (fairyCollectionData_t*)heap_caps_calloc(1, sizeof(fairyCollectionData_t), MALLOC_CAP_8BIT);
    fcLoadFromSwadgePass();
}

static void fcExitMode(void)
{
    free(fcd);
}

static void fcMainLoop(int64_t elapsedUs)
{
}

static void fcAddToSwadgePassPacket(struct swadgePassPacket* packet)
{
    // Load from NVS
    profileCard_t card;
    fcSPP_t fairy;
    size_t sCard  = sizeof(profileCard_t);
    size_t sFairy = sizeof(fcSPP_t);
    if (!readNamespaceNvsBlob(nvsStrs[FC_NAMESPACE], nvsStrs[FC_USER_FAIRY], &fairy, &sCard)
        || !readNamespaceNvsBlob(nvsStrs[FC_NAMESPACE], nvsStrs[FC_USER_CARD], &card, &sFairy))
    {
        // Uninitialized
        card.unInitialized = true;
    }
    packet->fairyCol.fairy = fairy;
    packet->fairyCol.card  = card;
}

static void fcLoadFromSwadgePass(void)
{
    // Load SPP data into array
    int16_t currIdx = 0;
    list_t spList   = {0};
    getSwadgePasses(&spList, &fairyCollectionMode, true);
    node_t* spNode = spList.first;
    while (spNode)
    {
        // Not marking it as used since we're loading from SPP each time
        swadgePassData_t* spd         = (swadgePassData_t*)spNode->val;
        fcd->spFairies[currIdx].fairy = spd->data.packet.fairyCol.fairy;
        fcd->spFairies[currIdx].pCard = spd->data.packet.fairyCol.card;
        currIdx++;
        spNode = spNode->next;
    }
}