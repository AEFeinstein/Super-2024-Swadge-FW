//==============================================================================
// Includes
//==============================================================================

#include "fairyCollection.h"

#include "fairyCollectionData.h"

//==============================================================================
// Defines
//==============================================================================

#define MAX_FAIRIES 64 // 64 * 13 = 832 bytes. Resize if required

//==============================================================================
// Consts
//==============================================================================

const char fcModeName[] = "Fairy Collection";

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    fcSPP_t userFairy;
    profileCard_t card;
    savedProfile_t spFairies[MAX_FAIRIES];
} fairyCollectionData_t;

//==============================================================================
// Functions declarations
//==============================================================================

static void fcEnterMode(void);
static void fcExitMode(void);
static void fcMainLoop(int64_t elapsedUs);
static void fcAddToSwadgePassPacket(struct swadgePassPacket* packet);

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
    .fnEnterMode             = fcMainLoop,
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
}