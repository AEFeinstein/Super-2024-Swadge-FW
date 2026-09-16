//==============================================================================
// Include
//==============================================================================

// Main
#include "ci_crafting.h"

// C
#include <stdio.h>
#include <inttypes.h>

// Camp
#include "ci_helpers.h"
#include "ci_items.h"
#include "ci_nvs.h"
#include "ci_recipeData.h"

//==============================================================================
// Defines
//==============================================================================

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

//==============================================================================
// Function declarations
//==============================================================================

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
static void drawArrowProg(ciCrafting_t* cft, bool dual, int timeUnits, int64_t timerUs);

/**
 * @brief Draws the inventory qty over the required qty
 *
 * @param ccd Game Data
 * @param yPos Center Y position to start at
 * @param idx If asking about recipe item 0 or 1
 */
static void drawQueueQtys(ciCrafting_t* cft, ciInventory_t* inv, font_t* font, int yPos, int idx);

/**
 * @brief Draws the qty owned over qty required
 *
 * @param ccd Game Data
 * @param r Recipe
 * @param idx Which item slot
 * @param yPos Y start position
 */
static void drawQty(ciInventory_t* inv, font_t* font, int topVal, int botVal, int x, int y, int width);

//==============================================================================
// Functions
//==============================================================================

void ciLoadCraftFromNVS(ciCrafting_t* cft)
{
    size_t len = 0;
    readNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_QUEUE], NULL, &len);
    int8_t toEnqueue[len];
    readNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_QUEUE], toEnqueue, &len);
    for (int idx = 0; idx < len; idx++)
    {
        intptr_t temp = toEnqueue[idx];
        push(&cft->craftQueue, (intptr_t*)temp);
    }
}

void ciSaveCraftToNVS(ciCrafting_t* cft)
{
    if (cft->craftQueue.length < 1)
    {
        return;
    }
    node_t* n = cft->craftQueue.first;
    int8_t idxs[cft->craftQueue.length];
    int idx = 0;
    while (n != NULL)
    {
        idxs[idx] = (intptr_t)n->val;
        idx++;
        n = n->next;
    }
    writeNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_QUEUE], idxs, cft->craftQueue.length);
}

void ciCraft(ciCrafting_t* cft, ciInventory_t* inv, int64_t* timeUnits)
{
    if (cft->craftQueue.first == NULL)
    {
        return;
    }
    const ciRecipeProto_t* r = &recipeList[(intptr_t)cft->craftQueue.first->val];
    if (*timeUnits >= r->time)
    {
        ciAddToInv(inv, r->result, 1);
        *timeUnits -= r->time;
        shift(&cft->craftQueue);
    }
}

/* void ciInitCraftTimer(ciCampData_t* ccd)
{
    // TODO:
    // Load previous time from NVS
    // Compare with RTC
    // Add units based on difference
} */

void drawCraft(ciCrafting_t* cft, ciInventory_t* inv, font_t* lFont, font_t* sFont, int timeUnits, int64_t timerUs)
{
    // Draw Background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT / 2, c100);
    // Draw Title
    drawText(lFont, c555, craftingText[1], (TFT_WIDTH - textWidth(lFont, craftingText[1])) / 2, 4);
    // Craft area
    fillDisplayArea(0, TFT_HEIGHT / 2, TFT_WIDTH, TFT_HEIGHT, c222);
    if (cft->craftQueue.first == NULL)
    {
        return;
    }
    const ciRecipeProto_t* r = &recipeList[(intptr_t)cft->craftQueue.first->val];
    if (r->items[1].item != CI_NO_ITEM)
    {
        ciDrawItemIcon(inv, sFont, r->items[0].item, CRAFT_X_BUFFER, CRAFT_Y_CENTER - DUAL_OFFSET, 0, false, false);
        drawQueueQtys(cft, inv, sFont, LINE_MIDDLE - DUAL_OFFSET, 0);
        ciDrawItemIcon(inv, sFont, r->items[1].item, CRAFT_X_BUFFER, CRAFT_Y_CENTER + DUAL_OFFSET, 0, false, false);
        drawQueueQtys(cft, inv, sFont, LINE_MIDDLE + DUAL_OFFSET, 1);
        // Combo Arrow
        drawArrow(true);
        drawArrowProg(cft, true, timeUnits, timerUs);
    }
    else
    {
        ciDrawItemIcon(inv, sFont, r->items[0].item, CRAFT_X_BUFFER, CRAFT_Y_CENTER, 0, false, false);
        drawQueueQtys(cft, inv, sFont, LINE_MIDDLE, 0);
        // Arrow
        drawArrow(false);
        drawArrowProg(cft, false, timeUnits, timerUs);
    }
    ciDrawItemIcon(inv, sFont, r->result, TFT_WIDTH - (ICON_WIDTH + CRAFT_X_BUFFER), CRAFT_Y_CENTER,
                   inv->qtys[r->result], false, false);
    drawText(sFont, c555, craftingText[2], (TFT_WIDTH - CRAFT_X_BUFFER / 2) - (textWidth(sFont, craftingText[2]) / 2),
             LINE_MIDDLE - (sFont->height + 2));
    char buffer[10];
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, inv->qtys[r->result]);
    drawText(sFont, c555, buffer, (TFT_WIDTH - CRAFT_X_BUFFER / 2) - (textWidth(sFont, buffer) / 2), LINE_MIDDLE + 2);
    drawText(sFont, c555, craftingText[3], 2, TFT_HEIGHT / 2 - (2 + sFont->height));
    node_t* node = cft->craftQueue.first;
    int pos      = 0;
    while (node != NULL)
    {
        if (pos == 12)
        {
            node = NULL;
            drawText(sFont, c555, craftingText[4], TFT_WIDTH - (textWidth(sFont, craftingText[4]) + 5),
                     TFT_HEIGHT / 2 - (sFont->height + 5));
            continue;
        }
        const ciRecipeProto_t* rq = &recipeList[(intptr_t)node->val];
        drawWsgSimpleHalf(&inv->itemImages[rq->result], 2 + textWidth(sFont, craftingText[3]) + pos * 18,
                          TFT_HEIGHT / 2 - 17);
        pos++;
        node = node->next;
    }
}

void drawCraftSelection(ciCrafting_t* cft, ciInventory_t* inv, ciWorkbenchData_t* wbd, font_t* lFont, font_t* sFont,
                        int selection)
{
    // Draw background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c010);
    // Draw title
    drawText(lFont, c555, craftingText[0], (TFT_WIDTH - textWidth(lFont, craftingText[0])) / 2, 4);
    // Draw recipes / selection
    for (int idx = 0; idx < ciGetRecipeCount(); idx++)
    {
        int x     = ICON_X_BUFFER + (ICON_X_BUFFER + ICON_WIDTH) * (idx % MAX_COLS);
        int y     = ICON_Y_START + (ICON_Y_BUFFER + ICON_HEIGHT) * (idx / MAX_COLS);
        node_t* n = cft->craftQueue.first;
        int total = 0;
        while (n != NULL)
        {
            if (idx == (intptr_t)n->val)
            {
                total++;
            }
            n = n->next;
        }
        ciDrawItemIcon(inv, sFont, recipeList[idx].result, x, y, total, (selection == idx), (total != 0));
    }
    // Draw small box
    int xStart               = TFT_WIDTH + 4 - WORKBENCH_SPACE;
    int yStart               = ICON_Y_START;
    int16_t xOff             = xStart;
    int16_t yOff             = yStart + 66;
    const ciRecipeProto_t* r = &recipeList[selection];
    wsg_t* temp              = getWsg(wbd, r->craftingStation);
    fillDisplayArea(xStart - 2, yStart, TFT_WIDTH - 2, TFT_HEIGHT - 2, c111);
    drawText(sFont, c555, craftingText[6], xStart, yStart + 2);
    drawWorkbench(wbd, xStart + (WORKBENCH_SPACE - (4 + temp->w)) / 2, yStart + 4 + sFont->height + (50 - temp->h) / 2,
                  r->craftingStation, 1, 1);
    drawTextWordWrapCentered(sFont, (CHECK_BIT(wbd->benches, r->craftingStation)) ? c040 : c400,
                             workbenchList[r->craftingStation].title, &xOff, &yOff, TFT_WIDTH - 2, TFT_HEIGHT - 2);
    ciDrawItemIcon(inv, sFont, r->items[0].item, xStart, yStart + 95, 0, false, false);
    drawQty(inv, sFont, inv->qtys[r->items[0].item], r->items[0].qty, TFT_WIDTH, 150, QTY_LEFT_SIZE);
    if (r->items[1].item != CI_NO_ITEM)
    {
        ciDrawItemIcon(inv, sFont, r->items[1].item, xStart, yStart + 150, 0, false, false);
        drawQty(inv, sFont, inv->qtys[r->items[1].item], r->items[1].qty, TFT_WIDTH, 210, QTY_LEFT_SIZE);
    }
    drawRect(xStart - 2, yStart, TFT_WIDTH - 2, TFT_HEIGHT - 2, c000);
}

//==============================================================================
// Static Functions
//==============================================================================

static void drawArrow(bool dual)
{
    int xStart = CRAFT_X_BUFFER * 2;
    if (dual)
    {
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 31, CRAFT_X_BUFFER * 3, LINE_MIDDLE - 31, c000);
        drawRect(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 30, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE - 28, c111);
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 27, CRAFT_X_BUFFER * 3 - 4, LINE_MIDDLE - 27, c000);
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 31, CRAFT_X_BUFFER * 3, LINE_MIDDLE + 31, c000);
        drawRect(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 30, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE + 28, c111);
        drawLineFast(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 27, CRAFT_X_BUFFER * 3 - 4, LINE_MIDDLE + 27, c000);
        drawLineFast(CRAFT_X_BUFFER * 3, LINE_MIDDLE - 31, CRAFT_X_BUFFER * 3, LINE_MIDDLE - 3, c000);
        drawLineFast(CRAFT_X_BUFFER * 3, LINE_MIDDLE + 31, CRAFT_X_BUFFER * 3, LINE_MIDDLE + 3, c000);
        drawRect(CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE - 30, CRAFT_X_BUFFER * 3 - 4, LINE_MIDDLE + 27, c111);
        drawLineFast(CRAFT_X_BUFFER * 3 - 4, LINE_MIDDLE - 27, CRAFT_X_BUFFER * 3 - 4, LINE_MIDDLE + 27, c000);
        xStart = CRAFT_X_BUFFER * 3;
    }
    drawLineFast(xStart, LINE_MIDDLE - 2, ARROW_START + ARROW_NOSE, LINE_MIDDLE - 2, c000);
    drawRectFilled(xStart, LINE_MIDDLE - 1, ARROW_START + ARROW_NOSE, LINE_MIDDLE + 1, c111);
    drawLineFast(xStart, LINE_MIDDLE + 2, ARROW_START + ARROW_NOSE, LINE_MIDDLE + 2, c000);
}

static void drawArrowProg(ciCrafting_t* cft, bool dual, int timeUnits, int64_t timerUs)
{
    int xStart               = CRAFT_X_BUFFER * 2;
    int len                  = ARROW_START + ARROW_NOSE - xStart;
    const ciRecipeProto_t* r = &recipeList[(intptr_t)cft->craftQueue.first->val];
    int unit                 = len / r->time;
    int curr                 = unit * timeUnits + (unit * timerUs) / UNIT;
    if (dual)
    {
        if (xStart + curr >= CRAFT_X_BUFFER * 3)
        {
            drawRect(CRAFT_X_BUFFER * 3, LINE_MIDDLE - 1, xStart + curr, LINE_MIDDLE + 1, c040);
            drawRect(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 30, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE - 28, c040);
            drawRect(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 30, CRAFT_X_BUFFER * 3 - 1, LINE_MIDDLE + 28, c040);
        }
        else if (xStart + curr < CRAFT_X_BUFFER * 3)
        {
            drawRect(CRAFT_X_BUFFER * 2, LINE_MIDDLE - 30, xStart + curr, LINE_MIDDLE - 28, c040);
            drawRect(CRAFT_X_BUFFER * 2, LINE_MIDDLE + 30, xStart + curr, LINE_MIDDLE + 28, c040);
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
        drawRect(xStart, LINE_MIDDLE - 1, xStart + curr, LINE_MIDDLE + 1, c040);
    }
}

static void drawQueueQtys(ciCrafting_t* cft, ciInventory_t* inv, font_t* font, int yPos, int idx)
{
    const ciRecipeProto_t* r = &recipeList[(intptr_t)cft->craftQueue.first->val];
    int total                = inv->qtys[r->items[idx].item];
    node_t* n                = cft->craftQueue.first;
    while (n != NULL)
    {
        const ciRecipeProto_t* re = &recipeList[(intptr_t)n->val];
        if (re->items[idx].item == r->items[idx].item)
        {
            total += re->items[idx].qty;
        }
        n = n->next;
    }
    drawQty(inv, font, total, r->items[idx].qty, 0, yPos, CRAFT_X_BUFFER);
    /*     snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, total);
        drawText(font, col, buffer, (CRAFT_X_BUFFER - textWidth(font, buffer)) / 2, yPos - (2 + font->height));
        snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, );
        drawText(font, col, buffer, (CRAFT_X_BUFFER - textWidth(font, buffer)) / 2, yPos + 2);
        drawLineFast(CRAFT_X_BUFFER / 3, yPos, (CRAFT_X_BUFFER * 2) / 3, yPos, col); */
}

static void drawQty(ciInventory_t* inv, font_t* font, int topVal, int botVal, int x, int y, int width)
{
    char buffer[10];
    paletteColor_t col = (topVal >= botVal) ? c040 : c400;
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, topVal);
    drawText(font, col, buffer, x - (width + textWidth(font, buffer)) / 2, y - (2 + font->height));
    drawLineFast(x + (width / 3), y, x + ((width * 2) / 3), y, col);
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, botVal);
    drawText(font, col, buffer, x - (width + textWidth(font, buffer)) / 2, y + 2);
}
