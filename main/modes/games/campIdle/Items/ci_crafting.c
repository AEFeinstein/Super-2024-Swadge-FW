//==============================================================================
// Include
//==============================================================================

#include "ci_crafting.h"
#include "ci_container.h"
#include "ci_items.h"
#include "ci_helpers.h"

#include "linked_list.h"

//==============================================================================
// Defines
//==============================================================================

#define MAX_COLS        4
#define ICON_X_BUFFER   10
#define ICON_Y_BUFFER   10
#define ICON_Y_START    (18 + ICON_Y_BUFFER)
#define CRAFT_X_BUFFER  48
#define CRAFT_Y_CENTER  153
#define LINE_MIDDLE     ((TFT_HEIGHT * 3) / 4)
#define ARROW_START     (TFT_WIDTH - (ICON_WIDTH + CRAFT_X_BUFFER + 23))
#define ARROW_NOSE      15
#define DUAL_OFFSET     ((4 + ICON_HEIGHT) / 2)
#define WORKBENCH_SPACE 80
#define QTY_LEFT_SIZE   30

//==============================================================================
// Consts
//==============================================================================

static const char* const craftingText[] = {
    "Add to queue", "Crafting", "Owned", "Queue: ", "+", "Press A to add to queue", "Requires:",
};

const cnfsFileIdx_t workbenchImages[] = {
    CC_HEARTMAKER_WSG,     CCM_SPHERE_1_WSG,         CCM_SPHERE_2_WSG,      CCM_SPHERE_3_WSG,      CCM_WORKBENCH_WSG,
    CC_POLISHER_WSG,       CC_POLISHER_BOX_WSG,      CC_SMASHER_WSG,        CC_SMASHER_HAMMER_WSG, CC_SMELTER_WSG,
    CC_SMELTER_FIRE_1_WSG, CC_SMELTER_FIRE_2_WSG,    CC_SMELTER_FIRE_3_WSG, CC_SMELTER_WOOD_WSG,   CC_STONECUTTER_WSG,
    CC_TANNING_RACK_WSG,   CC_TANNING_RACK_PELT_WSG, CC_WEAVER_WSG,         CC_WORKBENCH_WSG,
};

//==============================================================================
// Function declarations
//==============================================================================

/**
 * @brief Attempts to make a workbench
 *
 * @param ccd Game Data
 * @param bench The workbench to make
 * @return true If workbench was successfully made
 * @return false If workbench failed
 */
static bool tryToCraftWorkbench(ciCampData_t* ccd, const ciWorkbench_t* bench);

/**
 * @brief Draws the crafting selection screen
 *
 * @param ccd Game data
 */
static void drawCraftSelection(ciCampData_t* ccd);

/**
 * @brief Draws the crafting screen
 *
 * @param ccd Game Data
 */
static void drawCraft(ciCampData_t* ccd);

/**
 * @brief Draws the arrow from start to craft
 *
 * @param dual If there's two items as part of the recipe
 */
static void drawArrow(bool dual);

/**
 * @brief Draws the progress along the arrow
 *
 * @param ccd Game Data
 * @param dual If there's two items as part of the recipe
 */
static void drawArrowProg(ciCampData_t* ccd, bool dual);

/**
 * @brief Draws the inventory qty over the required qty
 *
 * @param ccd Game Data
 * @param yPos Center Y position to start at
 * @param idx If asking about recipe item 0 or 1
 */
static void drawQueueQtys(ciCampData_t* ccd, int yPos, int idx);

/**
 * @brief Get the Wsg for a crafting station
 *
 * @param ccd Game Data
 * @param wb Workbench
 * @return wsg_t* Image found. NULL if not found
 */
static wsg_t* getWsg(ciCampData_t* ccd, ciWorkbenchEnum_t wb);

/**
 * @brief Draws the qty owned over qty required
 *
 * @param ccd Game Data
 * @param r Recipe
 * @param idx Which item slot
 * @param yPos Y start position
 */
static void drawQty(ciCampData_t* ccd, const ciRecipeProto_t* r, int idx, int yPos);

//==============================================================================
// Functions
//==============================================================================

void ciInitWorkbenches(ciCampData_t* ccd)
{
    ccd->workbenchImages = (wsg_t*)heap_caps_calloc(ARRAY_SIZE(workbenchImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(workbenchImages); idx++)
    {
        loadWsg(workbenchImages[idx], &ccd->workbenchImages[idx], true);
    }
}

void ciFreeWorkbenches(ciCampData_t* ccd)
{
    for (int idx = 0; idx < ARRAY_SIZE(workbenchImages); idx++)
    {
        freeWsg(&ccd->workbenchImages[idx]);
    }
    free(ccd->workbenchImages);
}

void ciLoadCraftFromNVS(ciCampData_t* ccd)
{
    size_t len = 0;
    readNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_QUEUE], NULL, &len);
    int8_t toEnqueue[len];
    readNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_QUEUE], toEnqueue, &len);
    for (int idx = 0; idx < len; idx++)
    {
        intptr_t temp = toEnqueue[idx];
        push(&ccd->craftQueue, (intptr_t*)temp);
    }
}

void ciSaveCraftToNVS(ciCampData_t* ccd)
{
    if (ccd->craftQueue.length < 1)
    {
        return;
    }
    node_t* n = ccd->craftQueue.first;
    int8_t idxs[ccd->craftQueue.length];
    int idx = 0;
    while (n != NULL)
    {
        idxs[idx] = (intptr_t)n->val;
        idx++;
        n = n->next;
    }
    writeNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_QUEUE], idxs, ccd->craftQueue.length);
}

void ciInitCraftSelection(ciCampData_t* ccd)
{
    ccd->state     = CI_CRAFTING_PREP;
    ccd->selection = 0;
}

void ciInitCraft(ciCampData_t* ccd)
{
    ccd->state = CI_CRAFTING;
}

void ciRunCraftSelection(ciCampData_t* ccd)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            ccd->selection = ciMenu2DNavigate(&evt, ccd->selection, MAX_COLS, ciGetRecipeCount());
            if (evt.button & PB_A)
            {
                const ciRecipeProto_t* r = &recipeList[ccd->selection];
                bool ableToCraft
                    = (ccd->qtys[r->items[0].item] >= r->items[0].qty)
                      && (r->items[1].item == CI_NO_ITEM || (ccd->qtys[r->items[1].item] >= r->items[1].qty));
                ableToCraft = ableToCraft && CHECK_BIT(ccd->benches, r->craftingStation);
                if (ableToCraft)
                {
                    ciRemoveFromInv(ccd, r->items[0].item, r->items[0].qty);
                    ciRemoveFromInv(ccd, r->items[1].item, r->items[1].qty);
                    push(&ccd->craftQueue, (intptr_t*)ccd->selection);
                    // TODO: Add positive beep sound
                }
                else
                {
                    // TODO: Add negative beep sound
                }
            }
            else if (evt.button & PB_B)
            {
                ciInitCraft(ccd);
                // TODO: Add positive beep sound
            }
        }
    }
    drawCraftSelection(ccd);
}

bool ciRunCraft(ciCampData_t* ccd)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button & PB_A || evt.button & PB_LEFT)
            {
                ciInitCraftSelection(ccd);
                // TODO: Add positive beep sound
            }
            else if (evt.button & PB_B)
            {
                // TODO: Add positive beep sound
                return true;
            }
        }
    }
    drawCraft(ccd);
    return false;
}

void ciInitCraftTimer(ciCampData_t* ccd)
{
    // TODO:
    // Load previous time from NVS
    // Compare with RTC
    // Add units based on difference
}

void ciCraft(ciCampData_t* ccd)
{
    if (ccd->craftQueue.first == NULL)
    {
        return;
    }
    const ciRecipeProto_t* r = &recipeList[(intptr_t)ccd->craftQueue.first->val];
    if (ccd->timerUnits >= r->time)
    {
        ciAddToInv(ccd, r->result, 1);
        ccd->timerUnits -= r->time;
        shift(&ccd->craftQueue);
    }
}

void ciLoadWorkbenches(ciCampData_t* ccd)
{
    int outVal = 0;
    readNamespaceNvs32(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_WORKBENCHES], &outVal);
    ccd->benches = outVal;
}

void ciAddWorkbench(ciCampData_t* ccd, ciCraftingStation_t wb)
{
    const ciWorkbench_t* w = &workbenchList[wb];
    if (CHECK_BIT(ccd->benches, wb))
    {
        return; // Already owned
    }
    if (tryToCraftWorkbench(ccd, w))
    {
        SET_BIT(ccd->benches, wb);
        writeNamespaceNvs32(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_WORKBENCHES], ccd->benches);
    }
}

void drawWorkbench(ciCampData_t* ccd, int x, int y, ciWorkbenchEnum_t wb, int scale, int stage)
{
    switch (wb)
    {
        case CI_CRAFT_CRYSTAL_POLISHER:
        {
            drawWsgSimpleScaled(&ccd->workbenchImages[CI_POLISHER], x, y, scale, scale);
            drawWsgSimpleScaled(&ccd->workbenchImages[CI_POLISHER_BOX], x + (scale * 20), y + (scale * 9), scale,
                                scale);
            break;
        }
        case CI_CRAFT_HEARTMAKER:
        {
            drawWsgSimpleScaled(&ccd->workbenchImages[CI_HEARTMAKER], x, y, scale, scale);
            break;
        }
        case CI_CRAFT_MAGIC_WORKBENCH:
        {
            drawWsgSimpleScaled(&ccd->workbenchImages[CI_M_WORKBENCH], x, y, scale, scale);
            switch (stage)
            {
                case 1:
                case 2:
                case 3:
                {
                    drawWsgSimpleScaled(&ccd->workbenchImages[CI_M_SPHERE_1 + stage - 1], x + (scale * 8),
                                        y + (scale * 7), scale, scale);
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
            drawWsgSimpleScaled(&ccd->workbenchImages[CI_SMASHER], x, y, scale, scale);
            drawWsgSimpleScaled(&ccd->workbenchImages[CI_SMASHER_HAMMER], x + (scale * 5), y + (scale * 3), scale,
                                scale);
            // TODO: Smashing animation
            break;
        }
        case CI_CRAFT_SMELTER:
        {
            drawWsgSimpleScaled(&ccd->workbenchImages[CI_SMELTER], x, y, scale, scale);
            drawWsgSimpleScaled(&ccd->workbenchImages[CI_SMELTER_WOOD], x + (scale * 15), y + (scale * 23), scale,
                                scale);
            switch (stage)
            {
                case 1:
                case 2:
                case 3:
                {
                    drawWsgSimpleScaled(&ccd->workbenchImages[CI_SMELTER + stage], x + (scale * 17), y + (scale * 16),
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
            drawWsgSimpleScaled(&ccd->workbenchImages[CI_STONECUTTER], x, y, scale, scale);
            break;
        }
        case CI_CRAFT_TANNING_RACK:
        {
            drawWsgSimpleScaled(&ccd->workbenchImages[CI_TANNING_RACK], x, y, scale, scale);
            if (stage > 0)
            {
                drawWsgSimpleScaled(&ccd->workbenchImages[CI_TANNING_RACK_PELT], x + (scale * 5), y + (scale * 4),
                                    scale, scale);
            }
            break;
        }
        case CI_CRAFT_WEAVER:
        {
            drawWsgSimpleScaled(&ccd->workbenchImages[CI_WEAVER], x, y, scale, scale);
            break;
        }
        case CI_CRAFT_WORKBENCH:
        {
            drawWsgSimpleScaled(&ccd->workbenchImages[CI_WORKBENCH], x, y, scale, scale);
            break;
        }
    }
}

//==============================================================================
// Static Functions
//==============================================================================

static bool tryToCraftWorkbench(ciCampData_t* ccd, const ciWorkbench_t* bench)
{
    bool items[3] = {false};
    items[0]      = ciRemoveFromInv(ccd, bench->items[0].item, bench->items[0].qty);
    items[1]      = ciRemoveFromInv(ccd, bench->items[1].item, bench->items[1].qty);
    items[2]      = ciRemoveFromInv(ccd, bench->items[2].item, bench->items[2].qty);
    if (items[0] && items[1] && items[2])
    {
        return true;
    }
    for (int idx = 0; idx < 3; idx++)
    {
        if (items[idx])
        {
            ciAddToInv(ccd, bench->items[idx].item, bench->items[idx].qty);
        }
    }
    return false;
}

static void drawCraftSelection(ciCampData_t* ccd)
{
    // Draw background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c010);
    // Draw title
    drawText(&ccd->largeText, c555, craftingText[0], (TFT_WIDTH - textWidth(&ccd->largeText, craftingText[0])) / 2, 4);
    // Draw recipes / selection
    for (int idx = 0; idx < ciGetRecipeCount(); idx++)
    {
        int x     = ICON_X_BUFFER + (ICON_X_BUFFER + ICON_WIDTH) * (idx % MAX_COLS);
        int y     = ICON_Y_START + (ICON_Y_BUFFER + ICON_HEIGHT) * (idx / MAX_COLS);
        node_t* n = ccd->craftQueue.first;
        int total = 0;
        while (n != NULL)
        {
            if (idx == (intptr_t)n->val)
            {
                total++;
            }
            n = n->next;
        }
        ciDrawItemIcon(ccd, recipeList[idx].result, x, y, total, (ccd->selection == idx), (total != 0));
    }
    // Draw small box
    int xStart   = TFT_WIDTH + 4 - WORKBENCH_SPACE;
    int yStart   = ICON_Y_START;
    int16_t xOff = xStart;
    int16_t yOff = yStart + 66;
    wsg_t* temp  = getWsg(ccd, recipeList[ccd->selection].craftingStation);
    fillDisplayArea(xStart - 2, yStart, TFT_WIDTH - 2, TFT_HEIGHT - 2, c111);
    drawText(&ccd->smallFont, c555, craftingText[6], xStart, yStart + 2);
    drawWorkbench(ccd, xStart + (WORKBENCH_SPACE - (4 + temp->w)) / 2,
                  yStart + 4 + ccd->smallFont.height + (50 - temp->h) / 2, recipeList[ccd->selection].craftingStation,
                  1, 1);
    drawTextWordWrapCentered(
        &ccd->smallFont, (CHECK_BIT(ccd->benches, recipeList[ccd->selection].craftingStation)) ? c040 : c400,
        workbenchList[recipeList[ccd->selection].craftingStation].title, &xOff, &yOff, TFT_WIDTH - 2, TFT_HEIGHT - 2);
    ciDrawItemIcon(ccd, recipeList[ccd->selection].items[0].item, xStart, yStart + 95, 0, false, false);
    drawQty(ccd, &recipeList[ccd->selection], 0, 150);
    if (recipeList[ccd->selection].items[1].item != CI_NO_ITEM)
    {
        ciDrawItemIcon(ccd, recipeList[ccd->selection].items[1].item, xStart, yStart + 150, 0, false, false);
        drawQty(ccd, &recipeList[ccd->selection], 1, 200);
    }
    drawRect(xStart - 2, yStart, TFT_WIDTH - 2, TFT_HEIGHT - 2, c000);
}

static void drawCraft(ciCampData_t* ccd)
{
    // Draw Background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT / 2, c100);
    // Draw Title
    drawText(&ccd->largeText, c555, craftingText[1], (TFT_WIDTH - textWidth(&ccd->largeText, craftingText[1])) / 2, 4);
    // Craft area
    fillDisplayArea(0, TFT_HEIGHT / 2, TFT_WIDTH, TFT_HEIGHT, c222);
    if (ccd->craftQueue.first == NULL)
    {
        return;
    }
    const ciRecipeProto_t* r = &recipeList[(intptr_t)ccd->craftQueue.first->val];
    if (r->items[1].item != CI_NO_ITEM)
    {
        ciDrawItemIcon(ccd, r->items[0].item, CRAFT_X_BUFFER, CRAFT_Y_CENTER - DUAL_OFFSET, 0, false, false);
        drawQueueQtys(ccd, LINE_MIDDLE - DUAL_OFFSET, 0);
        ciDrawItemIcon(ccd, r->items[1].item, CRAFT_X_BUFFER, CRAFT_Y_CENTER + DUAL_OFFSET, 0, false, false);
        drawQueueQtys(ccd, LINE_MIDDLE + DUAL_OFFSET, 1);
        // Combo Arrow
        drawArrow(true);
        drawArrowProg(ccd, true);
    }
    else
    {
        ciDrawItemIcon(ccd, r->items[0].item, CRAFT_X_BUFFER, CRAFT_Y_CENTER, 0, false, false);
        drawQueueQtys(ccd, LINE_MIDDLE, 0);
        // Arrow
        drawArrow(false);
        drawArrowProg(ccd, false);
    }
    ciDrawItemIcon(ccd, r->result, TFT_WIDTH - (ICON_WIDTH + CRAFT_X_BUFFER), CRAFT_Y_CENTER, ccd->qtys[r->result],
                   false, false);
    drawText(&ccd->smallFont, c555, craftingText[2],
             (TFT_WIDTH - CRAFT_X_BUFFER / 2) - (textWidth(&ccd->smallFont, craftingText[2]) / 2),
             LINE_MIDDLE - (ccd->smallFont.height + 2));
    char buffer[10];
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, ccd->qtys[r->result]);
    drawText(&ccd->smallFont, c555, buffer, (TFT_WIDTH - CRAFT_X_BUFFER / 2) - (textWidth(&ccd->smallFont, buffer) / 2),
             LINE_MIDDLE + 2);
    drawText(&ccd->smallFont, c555, craftingText[3], 2, TFT_HEIGHT / 2 - (2 + ccd->smallFont.height));
    node_t* node = ccd->craftQueue.first;
    int pos      = 0;
    while (node != NULL)
    {
        if (pos == 12)
        {
            node = NULL;
            drawText(&ccd->smallFont, c555, craftingText[4],
                     TFT_WIDTH - (textWidth(&ccd->smallFont, craftingText[4]) + 5),
                     TFT_HEIGHT / 2 - (ccd->smallFont.height + 5));
            continue;
        }
        const ciRecipeProto_t* rq = &recipeList[(intptr_t)node->val];
        drawWsgSimpleHalf(&ccd->itemImages[rq->result], 2 + textWidth(&ccd->smallFont, craftingText[3]) + pos * 18,
                          TFT_HEIGHT / 2 - 17);
        pos++;
        node = node->next;
    }
}

static void drawArrow(bool dual)
{
    int xStart = CRAFT_X_BUFFER * 2;
    if (dual)
    {
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 31, CRAFT_X_BUFFER * 3, LINE_MIDDLE - 31, c000);
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 30, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE - 30, c111);
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 29, CRAFT_X_BUFFER * 3 - 2, LINE_MIDDLE - 29, c111);
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 28, CRAFT_X_BUFFER * 3 - 3, LINE_MIDDLE - 28, c111);
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 27, CRAFT_X_BUFFER * 3 - 4, LINE_MIDDLE - 27, c000);
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 31, CRAFT_X_BUFFER * 3, LINE_MIDDLE + 31, c000);
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 30, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE + 30, c111);
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 29, CRAFT_X_BUFFER * 3 - 2, LINE_MIDDLE + 29, c111);
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 28, CRAFT_X_BUFFER * 3 - 3, LINE_MIDDLE + 28, c111);
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 27, CRAFT_X_BUFFER * 3 - 4, LINE_MIDDLE + 27, c000);
        drawLineFast(CRAFT_X_BUFFER * 3, LINE_MIDDLE - 31, CRAFT_X_BUFFER * 3, LINE_MIDDLE - 3, c000);
        drawLineFast(CRAFT_X_BUFFER * 3, LINE_MIDDLE + 31, CRAFT_X_BUFFER * 3, LINE_MIDDLE + 3, c000);
        drawLineFast(CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE - 30, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE + 30, c111);
        drawLineFast(CRAFT_X_BUFFER * 3 - 2, LINE_MIDDLE - 29, CRAFT_X_BUFFER * 3 - 2, LINE_MIDDLE + 29, c111);
        drawLineFast(CRAFT_X_BUFFER * 3 - 3, LINE_MIDDLE - 28, CRAFT_X_BUFFER * 3 - 3, LINE_MIDDLE + 28, c111);
        drawLineFast(CRAFT_X_BUFFER * 3 - 4, LINE_MIDDLE - 27, CRAFT_X_BUFFER * 3 - 4, LINE_MIDDLE + 27, c000);
        xStart = CRAFT_X_BUFFER * 3;
    }
    drawLineFast(xStart, LINE_MIDDLE - 2, ARROW_START + ARROW_NOSE, LINE_MIDDLE - 2, c000);
    drawLineFast(xStart, LINE_MIDDLE - 1, ARROW_START + ARROW_NOSE, LINE_MIDDLE - 1, c111);
    drawLineFast(xStart, LINE_MIDDLE, ARROW_START + ARROW_NOSE, LINE_MIDDLE, c111);
    drawLineFast(xStart, LINE_MIDDLE + 1, ARROW_START + ARROW_NOSE, LINE_MIDDLE + 1, c111);
    drawLineFast(xStart, LINE_MIDDLE + 2, ARROW_START + ARROW_NOSE, LINE_MIDDLE + 2, c000);
}

static void drawArrowProg(ciCampData_t* ccd, bool dual)
{
    int xStart               = CRAFT_X_BUFFER * 2;
    int len                  = ARROW_START + ARROW_NOSE - xStart;
    const ciRecipeProto_t* r = &recipeList[(intptr_t)ccd->craftQueue.first->val];
    int unit                 = len / r->time;
    int curr                 = unit * ccd->timerUnits + (unit * ccd->timerUs) / UNIT;
    if (dual)
    {
        if (xStart + curr >= CRAFT_X_BUFFER * 3)
        {
            drawLineFast(CRAFT_X_BUFFER * 3, LINE_MIDDLE - 1, xStart + curr, LINE_MIDDLE - 1, c040);
            drawLineFast(CRAFT_X_BUFFER * 3, LINE_MIDDLE, xStart + curr, LINE_MIDDLE, c040);
            drawLineFast(CRAFT_X_BUFFER * 3, LINE_MIDDLE + 1, xStart + curr, LINE_MIDDLE + 1, c040);
            drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 30, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE - 30, c040);
            drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 29, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE - 29, c040);
            drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 28, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE - 28, c040);
            drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 30, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE + 30, c040);
            drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 29, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE + 29, c040);
            drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 28, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE + 28, c040);
        }
        else if (xStart + curr < CRAFT_X_BUFFER * 3)
        {
            drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 30, xStart + curr, LINE_MIDDLE - 30, c040);
            drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 29, xStart + curr, LINE_MIDDLE - 29, c040);
            drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 28, xStart + curr, LINE_MIDDLE - 28, c040);
            drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 30, xStart + curr, LINE_MIDDLE + 30, c040);
            drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 29, xStart + curr, LINE_MIDDLE + 29, c040);
            drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 28, xStart + curr, LINE_MIDDLE + 28, c040);
        }
        if (xStart + curr > CRAFT_X_BUFFER * 3 - 4)
        {
            drawLineFast(CRAFT_X_BUFFER * 3 - 3, LINE_MIDDLE - 30, CRAFT_X_BUFFER * 3 - 3, LINE_MIDDLE + 30, c040);
        }
        if (xStart + curr > CRAFT_X_BUFFER * 3 - 3)
        {
            drawLineFast(CRAFT_X_BUFFER * 3 - 2, LINE_MIDDLE - 30, CRAFT_X_BUFFER * 3 - 2, LINE_MIDDLE + 30, c040);
        }
        if (xStart + curr > CRAFT_X_BUFFER * 3 - 2)
        {
            drawLineFast(CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE - 30, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE + 30, c040);
        }
    }
    else
    {
        drawLineFast(xStart, LINE_MIDDLE - 1, xStart + curr, LINE_MIDDLE - 1, c040);
        drawLineFast(xStart, LINE_MIDDLE, xStart + curr, LINE_MIDDLE, c040);
        drawLineFast(xStart, LINE_MIDDLE + 1, xStart + curr, LINE_MIDDLE + 1, c040);
    }
}

static void drawQueueQtys(ciCampData_t* ccd, int yPos, int idx)
{
    char buffer[10];
    const ciRecipeProto_t* r = &recipeList[(intptr_t)ccd->craftQueue.first->val];
    paletteColor_t col       = (ccd->qtys[r->items[idx].item] >= r->items[idx].qty) ? c040 : c400;
    int total                = ccd->qtys[r->items[idx].item];
    node_t* n                = ccd->craftQueue.first;
    while (n != NULL)
    {
        const ciRecipeProto_t* re = &recipeList[(intptr_t)n->val];
        if (re->items[idx].item == r->items[idx].item)
        {
            total += re->items[idx].qty;
        }
        n = n->next;
    }
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, total);
    drawText(&ccd->smallFont, col, buffer, (CRAFT_X_BUFFER - textWidth(&ccd->smallFont, buffer)) / 2,
             yPos - (2 + ccd->smallFont.height));
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, r->items[idx].qty);
    drawText(&ccd->smallFont, col, buffer, (CRAFT_X_BUFFER - textWidth(&ccd->smallFont, buffer)) / 2, yPos + 2);
    drawLineFast(CRAFT_X_BUFFER / 3, yPos, (CRAFT_X_BUFFER * 2) / 3, yPos, col);
}

static void drawQty(ciCampData_t* ccd, const ciRecipeProto_t* r, int idx, int yPos)
{
    char buffer[10];
    paletteColor_t col = (ccd->qtys[r->items[idx].item] >= r->items[idx].qty) ? c040 : c400;
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, ccd->qtys[r->items[idx].item]);
    drawText(&ccd->smallFont, col, buffer, TFT_WIDTH - (QTY_LEFT_SIZE + textWidth(&ccd->smallFont, buffer)) / 2,
             yPos - (2 + ccd->smallFont.height));
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, r->items[idx].qty);
    drawText(&ccd->smallFont, col, buffer, TFT_WIDTH - (QTY_LEFT_SIZE + textWidth(&ccd->smallFont, buffer)) / 2,
             yPos + 2);
    drawLineFast(TFT_WIDTH - QTY_LEFT_SIZE / 3, yPos, TFT_WIDTH - (QTY_LEFT_SIZE * 2) / 3, yPos, col);
}

static wsg_t* getWsg(ciCampData_t* ccd, ciWorkbenchEnum_t wb)
{
    switch (wb)
    {
        case CI_CRAFT_CRYSTAL_POLISHER:
        {
            return &ccd->workbenchImages[CI_POLISHER];
        }
        case CI_CRAFT_HEARTMAKER:
        {
            return &ccd->workbenchImages[CI_HEARTMAKER];
        }
        case CI_CRAFT_MAGIC_WORKBENCH:
        {
            return &ccd->workbenchImages[CI_M_WORKBENCH];
        }
        case CI_CRAFT_SMASHER:
        {
            return &ccd->workbenchImages[CI_SMASHER];
        }
        case CI_CRAFT_SMELTER:
        {
            return &ccd->workbenchImages[CI_SMELTER];
        }
        case CI_CRAFT_STONE_CUTTER:
        {
            return &ccd->workbenchImages[CI_STONECUTTER];
        }
        case CI_CRAFT_TANNING_RACK:
        {
            return &ccd->workbenchImages[CI_TANNING_RACK];
        }
        case CI_CRAFT_WEAVER:
        {
            return &ccd->workbenchImages[CI_WEAVER];
        }
        case CI_CRAFT_WORKBENCH:
        {
            return &ccd->workbenchImages[CI_WORKBENCH];
        }
    }
    return NULL;
}