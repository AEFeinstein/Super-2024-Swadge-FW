//==============================================================================
// Includes
//==============================================================================

#include "fairyCollection.h"

#include "fairyCollectionData.h"
#include "fairyCreation.h"

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

/*
Trophies:
- OLDHEAD: Find someone with 15+ years
- Gets around: FInd one of each attendancer type
*/

//==============================================================================
// Enum
//==============================================================================

typedef enum
{
    FC_MENU,
    FC_SP,
    FC_CREATOR,
} fcState_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Resources
    font_t font;

    // Data
    fairy_t userFairy;
    profileCard_t userCard;
    savedProfile_t spFairies[FC_MAX_NUM_FAIRIES];

    // State
    fcState_t state;
    fcCreationData_t* fcdd;
} fairyCollectionData_t;

//==============================================================================
// Functions declarations
//==============================================================================

static void fcEnterMode(void);
static void fcExitMode(void);
static void fcMainLoop(int64_t elapsedUs);

// Swadgepass
static void fcAddToSwadgePassPacket(struct swadgePassPacket* packet);
static void loadUserFairy(void);
static void loadFromSwadgePass(void);

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
    fcd       = (fairyCollectionData_t*)heap_caps_calloc(1, sizeof(fairyCollectionData_t), MALLOC_CAP_8BIT);
    fcd->fcdd = (fcCreationData_t*)heap_caps_calloc(1, sizeof(fcCreationData_t), MALLOC_CAP_8BIT);
    loadFont(IBM_VGA_8_FONT, &fcd->font, true);
    loadUserFairy();
    fcInitCreation(fcd->fcdd, &fcd->userFairy, &fcd->userCard);
    loadFromSwadgePass();

    // TEST
    fcd->state = FC_CREATOR;
}

static void fcExitMode(void)
{
    freeFont(&fcd->font);
    free(fcd->fcdd);
    free(fcd);
}

static void fcMainLoop(int64_t elapsedUs)
{
    switch (fcd->state)
    {
        case FC_MENU:
        {
            // TODO: Handle Menu
            break;
        }
        case FC_SP:
        {
            // TODO: Handle SP field
            break;
        }
        case FC_CREATOR:
        {
            if (fcRunCreation(fcd->fcdd))
            {
                // TODO: Change mode
            }
            fcDrawCreation(fcd->fcdd, &fcd->font);
            break;
        }
        default:
        {
            buttonEvt_t evt;
            while (checkButtonQueueWrapper(&evt))
            {
                // Allows backing out
            }
            break;
        }
    }
}

static void fcAddToSwadgePassPacket(struct swadgePassPacket* packet)
{
    // Load from NVS
    profileCard_t card;
    fairy_t fairy;
    size_t sCard  = sizeof(profileCard_t);
    size_t sFairy = sizeof(fairy_t);
    if (!readNamespaceNvsBlob(nvsStrs[FC_NAMESPACE], nvsStrs[FC_USER_FAIRY], &fairy, &sCard)
        || !readNamespaceNvsBlob(nvsStrs[FC_NAMESPACE], nvsStrs[FC_USER_CARD], &card, &sFairy))
    {
        // Uninitialized
        card.unInitialized = true;
    }
    packet->fairyCol.fairy = fairy;
    packet->fairyCol.card  = card;
}

static void loadUserFairy(void)
{
    size_t size = sizeof(fairy_t);
    readNamespaceNvsBlob(nvsStrs[FC_NAMESPACE], nvsStrs[FC_USER_FAIRY], &fcd->userFairy, &size);
    size = sizeof(profileCard_t);
    readNamespaceNvsBlob(nvsStrs[FC_NAMESPACE], nvsStrs[FC_USER_FAIRY], &fcd->userCard, &size);
}

static void loadFromSwadgePass(void)
{
    // Load SPP data into array
    switchToSpeaker();
    dacStop();
    int32_t currIdx = 0;
    readNamespaceNvs32(nvsStrs[FC_NAMESPACE], nvsStrs[FC_SPP_NEXT_IDX], &currIdx);
    size_t size = sizeof(fcd->spFairies);
    readNamespaceNvsBlob(nvsStrs[FC_NAMESPACE], nvsStrs[FC_SPP_SAVED], &fcd->spFairies, &size);
    list_t spList = {0};
    getSwadgePasses(&spList, &fairyCollectionMode, true);
    node_t* spNode = spList.first;
    while (spNode)
    {
        swadgePassData_t* spd = (swadgePassData_t*)spNode->val;
        if (!isPacketUsedByMode(spd, &fairyCollectionMode))
        {
            fcd->spFairies[currIdx].fairy = spd->data.packet.fairyCol.fairy;
            fcd->spFairies[currIdx].pCard = spd->data.packet.fairyCol.card;
            setPacketUsedByMode(spd, &fairyCollectionMode, true);
            currIdx++;
            if (currIdx >= FC_MAX_NUM_FAIRIES)
            {
                currIdx = 0;
            }
        }
        spNode = spNode->next;
    }
    dacStart();
    // Save to NVS
    writeNamespaceNvs32(nvsStrs[FC_NAMESPACE], nvsStrs[FC_SPP_NEXT_IDX], currIdx);
    writeNamespaceNvsBlob(nvsStrs[FC_NAMESPACE], nvsStrs[FC_SPP_SAVED], fcd->spFairies, sizeof(fcd->spFairies));
}