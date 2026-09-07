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

// Select Craft
#define MAX_COLS       4
#define ICON_X_BUFFER  10
#define ICON_Y_BUFFER  10
#define ICON_Y_START   (18 + ICON_Y_BUFFER)
#define CRAFT_X_BUFFER 48
#define CRAFT_Y_CENTER 153
#define LINE_MIDDLE    ((TFT_HEIGHT * 3) / 4)
#define ARROW_START    (TFT_WIDTH - (ICON_WIDTH + CRAFT_X_BUFFER + 23))
#define ARROW_NOSE     15
#define DUAL_OFFSET    ((4 + ICON_HEIGHT) / 2)

//==============================================================================
// Consts
//==============================================================================

static const char* const craftingText[] = {
    "Add to queue", "Crafting", "Owned", "Queue: ", "+", "Press A to add to queue",
};

//==============================================================================
// Function declarations
//==============================================================================

/**
 * @brief Attempts to make a workbench
 *
 * @param ccd Game Data
 * @param bench The workbench to make
 * @return true If workbech was ssuccesfully made
 * @return false If workbech failed
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
 * @param yPos Center Y position toi start at
 * @param idx If asking about recipe item 0 or 1
 */
static void drawQtys(ciCampData_t* ccd, int yPos, int idx);

//==============================================================================
// Functions
//==============================================================================

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

void ciSaveCraftFromNVS(ciCampData_t* ccd)
{
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
                ciSaveCraftFromNVS(ccd);
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
    // TODO: Add in workbenches
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
        drawQtys(ccd, LINE_MIDDLE - DUAL_OFFSET, 0);
        ciDrawItemIcon(ccd, r->items[1].item, CRAFT_X_BUFFER, CRAFT_Y_CENTER + DUAL_OFFSET, 0, false, false);
        drawQtys(ccd, LINE_MIDDLE + DUAL_OFFSET, 1);
        // Combo Arrow
        drawArrow(true);
        drawArrowProg(ccd, true);
    }
    else
    {
        ciDrawItemIcon(ccd, r->items[0].item, CRAFT_X_BUFFER, CRAFT_Y_CENTER, 0, false, false);
        drawQtys(ccd, LINE_MIDDLE, 0);
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

static void drawQtys(ciCampData_t* ccd, int yPos, int idx)
{
    char buffer[10];
    const ciRecipeProto_t* r = &recipeList[(intptr_t)ccd->craftQueue.first->val];
    paletteColor_t col       = (ccd->qtys[r->items[idx].item] >= r->items[idx].qty) ? c040 : c400;
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, ccd->qtys[r->items[idx].item]);
    drawText(&ccd->smallFont, col, buffer, (CRAFT_X_BUFFER - textWidth(&ccd->smallFont, buffer)) / 2,
             yPos - (2 + ccd->smallFont.height));
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, r->items[idx].qty);
    drawText(&ccd->smallFont, col, buffer, (CRAFT_X_BUFFER - textWidth(&ccd->smallFont, buffer)) / 2, yPos + 2);
    drawLineFast(CRAFT_X_BUFFER / 3, yPos, (CRAFT_X_BUFFER * 2) / 3, yPos, col);
}