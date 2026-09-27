//==============================================================================
// Includes
//==============================================================================

#include "fairyCollection.h"

#include "fairyCollectionData.h"
#include "fairyCreation.h"

#include "menu.h"

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

static const char* const fcMenuText[] = {
    "Create-a-fairy",
    "Collection",
    "Background: ",
};
static const char* const fcBGOptions[] = {
    "Greenhouse",
    "Forest",
    "Bookcase",
    "Lab",
};
static const int fcBGOptionVals[] = {
    0,
    1,
    2,
    3,
};
const settingParam_t opts = {
    .min = 0,
    .max = ARRAY_SIZE(fcBGOptionVals),
    .def = 0,
    .key = "fc-bg-key",
};

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

    // Menu
    menu_t* menu;
    menuZorldoRenderer_t* zr;
    int background;

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
static bool fcMenuCb(const char* label, bool selected, uint32_t settingVal);

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

    // Init menu
    fcd->menu = initMenu(fcModeName, fcMenuCb);
    addSingleItemToMenu(fcd->menu, fcMenuText[0]);
    addSingleItemToMenu(fcd->menu, fcMenuText[1]);
    addSettingsOptionsItemToMenu(fcd->menu, fcMenuText[2], fcBGOptions, fcBGOptionVals, ARRAY_SIZE(fcBGOptions), &opts,
                                 fcd->background);
    fcd->zr = initMenuZorldoRenderer(NULL, NULL);
    
    // TEST
    fcd->state = FC_CREATOR;
}

static void fcExitMode(void)
{
    deinitMenuZorldoRenderer(fcd->zr);
    deinitMenu(fcd->menu);
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
            buttonEvt_t evt;
            while (checkButtonQueueWrapper(&evt))
            {
                fcd->menu = menuButton(fcd->menu, evt);
            }
            drawMenuZorldo(fcd->menu, fcd->zr, elapsedUs);
            break;
        }
        case FC_SP:
        {
            // TODO: Handle SP field
            buttonEvt_t evt;
            while (checkButtonQueueWrapper(&evt))
            {
                // Allows backing out
            }
            break;
        }
        case FC_CREATOR:
        {
            if (fcRunCreation(fcd->fcdd))
            {
                fcd->state = FC_MENU;
            }
            fcDrawCreation(fcd->fcdd, &fcd->font);
            // TODO: Draw user's fairy here
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
        card.initialized = false;
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

static bool fcMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == fcMenuText[0])
        {
            fcd->state = FC_CREATOR;
        }
        else if (label == fcMenuText[1])
        {
            fcd->state = FC_SP;
        }
    }
    return false;
}