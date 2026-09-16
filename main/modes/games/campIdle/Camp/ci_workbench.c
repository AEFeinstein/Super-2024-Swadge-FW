//==============================================================================
// Include
//==============================================================================

// Main
#include "ci_workbench.h"

// Camp
#include "ci_helpers.h"
#include "ci_nvs.h"

//==============================================================================
// Const
//==============================================================================

const cnfsFileIdx_t workbenchImages[] = {
    CC_HEARTMAKER_WSG,     CCM_SPHERE_1_WSG,         CCM_SPHERE_2_WSG,      CCM_SPHERE_3_WSG,      CCM_WORKBENCH_WSG,
    CC_POLISHER_WSG,       CC_POLISHER_BOX_WSG,      CC_SMASHER_WSG,        CC_SMASHER_HAMMER_WSG, CC_SMELTER_WSG,
    CC_SMELTER_FIRE_1_WSG, CC_SMELTER_FIRE_2_WSG,    CC_SMELTER_FIRE_3_WSG, CC_SMELTER_WOOD_WSG,   CC_STONECUTTER_WSG,
    CC_TANNING_RACK_WSG,   CC_TANNING_RACK_PELT_WSG, CC_WEAVER_WSG,         CC_WORKBENCH_WSG,
};

const ciWorkbench_t workbenchList[] = {
    {
        .title         = "Polisher",
        .desc          = "Allows you to polish rocks and crystals",
        .items[0].item = CI_CUT_ROCK,
        .items[0].qty  = 20,
        .items[1].item = CI_DIAMOND_POWDER,
        .items[1].qty  = 10,
        .items[2].item = CI_NO_ITEM,
        .items[2].qty  = 0,
    },
    {
        .title         = "Heartmaker",
        .desc          = "Pulls magic out of the air to heal you",
        .items[0].item = CI_POLISHED_CRYSTAL,
        .items[0].qty  = 10,
        .items[1].item = CI_CLOTH,
        .items[1].qty  = 10,
        .items[2].item = CI_ROPE,
        .items[2].qty  = 3,
    },
    {
        .title         = "Magic Workbench",
        .desc          = "Allows you to craft magical items",
        .items[0].item = CI_POLISHED_CRYSTAL,
        .items[0].qty  = 4,
        .items[1].item = CI_LOG,
        .items[1].qty  = 4,
        .items[2].item = CI_NO_ITEM,
        .items[2].qty  = 0,
    },
    {
        .title         = "Smasher",
        .desc          = "Smashes things to bits",
        .items[0].item = CI_ROCKS,
        .items[0].qty  = 10,
        .items[1].item = CI_STICK,
        .items[1].qty  = 10,
        .items[2].item = CI_GEAR,
        .items[2].qty  = 5,
    },
    {
        .title         = "Smelter",
        .desc          = "Turn your ore into Iron",
        .items[0].item = CI_ROCKS,
        .items[0].qty  = 20,
        .items[1].item = CI_COAL,
        .items[1].qty  = 10,
        .items[2].item = CI_NO_ITEM,
        .items[2].qty  = 0,
    },
    {
        .title         = "Stone Cutter",
        .desc          = "Allows you to cut stones and make them square",
        .items[0].item = CI_LOG,
        .items[0].qty  = 4,
        .items[1].item = CI_ROCKS,
        .items[1].qty  = 10,
        .items[2].item = CI_NO_ITEM,
        .items[2].qty  = 0,
    },
    {
        .title         = "Tanning rack",
        .desc          = "The hides may be pleather, but you still need to tan them",
        .items[0].item = CI_STICK,
        .items[0].qty  = 10,
        .items[1].item = CI_ROPE,
        .items[1].qty  = 4,
        .items[2].item = CI_NO_ITEM,
        .items[2].qty  = 0,
    },
    {
        .title         = "Weaver",
        .desc          = "Allows for the creation of cloth",
        .items[0].item = CI_STICK,
        .items[0].qty  = 20,
        .items[1].item = CI_GEAR,
        .items[1].qty  = 4,
        .items[2].item = CI_IRON_NAIL,
        .items[2].qty  = 10,
    },
    {
        .title         = "Workbench",
        .desc          = "A simple table for building more complex things",
        .items[0].item = CI_LOG,
        .items[0].qty  = 2,
        .items[1].item = CI_STICK,
        .items[1].qty  = 10,
        .items[2].item = CI_RESIN,
        .items[2].qty  = 2,
    },
};

//==============================================================================
// Function Declarations
//==============================================================================

/**
 * @brief Attempts to make a workbench
 *
 * @param wbd Game Data
 * @param bench The workbench to make
 * @return true If workbench was successfully made
 * @return false If workbench failed
 */
static bool tryToCraftWorkbench(ciWorkbenchData_t* wbd, ciInventory_t* inv, const ciWorkbench_t* bench);

//==============================================================================
// Functions
//==============================================================================

void ciInitWorkbenchImages(ciWorkbenchData_t* wbd)
{
    wbd->benchImages = (wsg_t*)heap_caps_calloc(ARRAY_SIZE(workbenchImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(workbenchImages); idx++)
    {
        loadWsg(workbenchImages[idx], &wbd->benchImages[idx], true);
    }
}

void ciFreeWorkbenchImages(ciWorkbenchData_t* wbd)
{
    for (int idx = 0; idx < ARRAY_SIZE(workbenchImages); idx++)
    {
        freeWsg(&wbd->benchImages[idx]);
    }
    free(wbd->benchImages);
}

void ciLoadWorkbenchFromNVS(ciWorkbenchData_t* wbd)
{
    int outVal = 0;
    readNamespaceNvs32(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_WORKBENCHES], &outVal);
    wbd->benches = outVal;
}

void ciSetWorkbench(ciWorkbenchData_t* wbd, ciInventory_t* inv, ciWorkbenchCraft_t wb)
{
    const ciWorkbench_t* w = &workbenchList[wb];
    if (CHECK_BIT(wbd->benches, wb))
    {
        return; // Already owned
    }
    if (tryToCraftWorkbench(wbd, inv, w))
    {
        SET_BIT(wbd->benches, wb);
        writeNamespaceNvs32(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_WORKBENCHES], wbd->benches);
    }
}

void drawWorkbench(ciWorkbenchData_t* wbd, int x, int y, ciWorkbenchCraft_t wb, int scale, int stage)
{
    switch (wb)
    {
        case CI_CRAFT_CRYSTAL_POLISHER:
        {
            drawWsgSimpleScaled(&wbd->benchImages[CI_POLISHER], x, y, scale, scale);
            drawWsgSimpleScaled(&wbd->benchImages[CI_POLISHER_BOX], x + (scale * 20), y + (scale * 9), scale, scale);
            break;
        }
        case CI_CRAFT_HEARTMAKER:
        {
            drawWsgSimpleScaled(&wbd->benchImages[CI_HEARTMAKER], x, y, scale, scale);
            break;
        }
        case CI_CRAFT_MAGIC_WORKBENCH:
        {
            drawWsgSimpleScaled(&wbd->benchImages[CI_M_WORKBENCH], x, y, scale, scale);
            switch (stage)
            {
                case 1:
                case 2:
                case 3:
                {
                    drawWsgSimpleScaled(&wbd->benchImages[CI_M_SPHERE_1 + stage - 1], x + (scale * 8), y + (scale * 7),
                                        scale, scale);
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        case CI_CRAFT_SMASHER:
        {
            drawWsgSimpleScaled(&wbd->benchImages[CI_SMASHER], x, y, scale, scale);
            drawWsgSimpleScaled(&wbd->benchImages[CI_SMASHER_HAMMER], x + (scale * 5), y + (scale * 3), scale, scale);
            // TODO: Smashing animation
            break;
        }
        case CI_CRAFT_SMELTER:
        {
            drawWsgSimpleScaled(&wbd->benchImages[CI_SMELTER], x, y, scale, scale);
            drawWsgSimpleScaled(&wbd->benchImages[CI_SMELTER_WOOD], x + (scale * 15), y + (scale * 23), scale, scale);
            switch (stage)
            {
                case 1:
                case 2:
                case 3:
                {
                    drawWsgSimpleScaled(&wbd->benchImages[CI_SMELTER + stage], x + (scale * 17), y + (scale * 16),
                                        scale, scale);
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        case CI_CRAFT_STONE_CUTTER:
        {
            drawWsgSimpleScaled(&wbd->benchImages[CI_STONECUTTER], x, y, scale, scale);
            break;
        }
        case CI_CRAFT_TANNING_RACK:
        {
            drawWsgSimpleScaled(&wbd->benchImages[CI_TANNING_RACK], x, y, scale, scale);
            if (stage > 0)
            {
                drawWsgSimpleScaled(&wbd->benchImages[CI_TANNING_RACK_PELT], x + (scale * 5), y + (scale * 4), scale,
                                    scale);
            }
            break;
        }
        case CI_CRAFT_WEAVER:
        {
            drawWsgSimpleScaled(&wbd->benchImages[CI_WEAVER], x, y, scale, scale);
            break;
        }
        case CI_CRAFT_WORKBENCH:
        {
            drawWsgSimpleScaled(&wbd->benchImages[CI_WORKBENCH], x, y, scale, scale);
            break;
        }
    }
}

wsg_t* getWsg(ciWorkbenchData_t* wbd, ciWorkbenchCraft_t wb)
{
    switch (wb)
    {
        case CI_CRAFT_CRYSTAL_POLISHER:
        {
            return &wbd->benchImages[CI_POLISHER];
        }
        case CI_CRAFT_HEARTMAKER:
        {
            return &wbd->benchImages[CI_HEARTMAKER];
        }
        case CI_CRAFT_MAGIC_WORKBENCH:
        {
            return &wbd->benchImages[CI_M_WORKBENCH];
        }
        case CI_CRAFT_SMASHER:
        {
            return &wbd->benchImages[CI_SMASHER];
        }
        case CI_CRAFT_SMELTER:
        {
            return &wbd->benchImages[CI_SMELTER];
        }
        case CI_CRAFT_STONE_CUTTER:
        {
            return &wbd->benchImages[CI_STONECUTTER];
        }
        case CI_CRAFT_TANNING_RACK:
        {
            return &wbd->benchImages[CI_TANNING_RACK];
        }
        case CI_CRAFT_WEAVER:
        {
            return &wbd->benchImages[CI_WEAVER];
        }
        case CI_CRAFT_WORKBENCH:
        {
            return &wbd->benchImages[CI_WORKBENCH];
        }
    }
    return NULL;
}

//==============================================================================
// Static Functions
//==============================================================================

static bool tryToCraftWorkbench(ciWorkbenchData_t* wbd, ciInventory_t* inv, const ciWorkbench_t* bench)
{
    bool items[3] = {false};
    items[0]      = ciRemoveFromInv(inv, bench->items[0].item, bench->items[0].qty);
    items[1]      = ciRemoveFromInv(inv, bench->items[1].item, bench->items[1].qty);
    items[2]      = ciRemoveFromInv(inv, bench->items[2].item, bench->items[2].qty);
    if (items[0] && items[1] && items[2])
    {
        return true;
    }
    for (int idx = 0; idx < 3; idx++)
    {
        if (items[idx])
        {
            ciAddToInv(inv, bench->items[idx].item, bench->items[idx].qty);
        }
    }
    return false;
}