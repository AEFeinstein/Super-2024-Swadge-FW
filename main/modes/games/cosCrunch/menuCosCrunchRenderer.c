#include "menuCosCrunchRenderer.h"

#include "cosCrunchUtil.h"
#include "esp_heap_caps.h"
#include "fs_wsg.h"
#include "hdw-tft.h"
#include "menu_utils.h"
#include "shapes.h"

#include <string.h>

#define TITLE_MARGIN_TOP    15
#define TITLE_MARGIN_BOTTOM 15
#define MENU_MARGIN_BOTTOM  15
#define MENU_ITEM_PADDING   10

menuCosCrunchRenderer_t* initMenuCosCrunchRenderer(font_t* titleFont, font_t* titleFontOutline, font_t* menuFont)
{
    menuCosCrunchRenderer_t* renderer = heap_caps_calloc(1, sizeof(menuCosCrunchRenderer_t), MALLOC_CAP_SPIRAM);

    loadWsg(CC_MENU_PIN_WSG, &renderer->wsg.pin, true);
    loadWsg(CC_MENU_FOLD_WSG, &renderer->wsg.fold, true);

    renderer->menuFont         = menuFont;
    renderer->titleFont        = titleFont;
    renderer->titleFontOutline = titleFontOutline;

    return renderer;
}

void deinitMenuCosCrunchRenderer(menuCosCrunchRenderer_t* renderer)
{
    freeWsg(&renderer->wsg.pin);
    freeWsg(&renderer->wsg.fold);

    heap_caps_free(renderer);
}

void drawMenuCosCrunch(menu_t* menu, menuCosCrunchRenderer_t* renderer, int64_t elapsedUs)
{
    uint16_t titleWidth = textWidth(renderer->titleFont, menu->title);
    drawText(renderer->titleFont, c555, menu->title, (TFT_WIDTH - titleWidth) / 2, TITLE_MARGIN_TOP);
    drawText(renderer->titleFontOutline, c000, menu->title, (TFT_WIDTH - titleWidth) / 2, TITLE_MARGIN_TOP);

    int16_t menuHeight
        = TFT_HEIGHT - (renderer->titleFont->height + TITLE_MARGIN_TOP + TITLE_MARGIN_BOTTOM + MENU_MARGIN_BOTTOM);
    int16_t titleHeight = renderer->titleFont->height + TITLE_MARGIN_TOP + TITLE_MARGIN_BOTTOM;
    int16_t itemHeight  = renderer->menuFont->height + MENU_ITEM_PADDING * 2;
    int16_t itemMargin  = (menuHeight - (itemHeight * menu->items->length)) / (menu->items->length - 1);

    uint8_t itemIdx = 0;
    node_t* node    = menu->items->first;
    while (node != NULL)
    {
        menuItem_t* item  = node->val;
        int16_t itemWidth = textWidth(renderer->menuFont, item->label) + MENU_ITEM_PADDING * 2;
        int16_t itemX     = (TFT_WIDTH - itemWidth) / 2;
        int16_t itemY     = itemIdx * (itemHeight + itemMargin) + titleHeight;

        drawMessageBox(itemX, itemY, itemX + itemWidth, itemY + itemHeight, renderer->wsg.fold);
        drawText(renderer->menuFont, c000, item->label, itemX + MENU_ITEM_PADDING, itemY + MENU_ITEM_PADDING);

        if (node->val == menu->currentItem->val)
        {
            drawWsgSimple(&renderer->wsg.pin, itemX - 8, itemY - 5);
        }

        node = node->next;
        itemIdx++;
    }
}
