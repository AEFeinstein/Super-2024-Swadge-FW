#include "menuCosCrunchRenderer.h"

#include "cosCrunchUtil.h"
#include "esp_heap_caps.h"
#include "fs_wsg.h"
#include "hdw-tft.h"
#include "macros.h"
#include "menu_utils.h"

#include <string.h>

#define TITLE_MARGIN_TOP    15
#define TITLE_MARGIN_BOTTOM 15
#define MENU_MARGIN_BOTTOM  15
#define MENU_ITEM_PADDING   10
#define OPTION_PADDING      5
#define ARROW_PADDING       2

menuCosCrunchRenderer_t* initMenuCosCrunchRenderer(font_t* titleFont, font_t* titleFontOutline, font_t* menuFont)
{
    menuCosCrunchRenderer_t* renderer = heap_caps_calloc(1, sizeof(menuCosCrunchRenderer_t), MALLOC_CAP_SPIRAM);

    loadWsg(CC_MENU_PIN_WSG, &renderer->wsg.pin, true);
    loadWsg(CC_MENU_FOLD_WSG, &renderer->wsg.fold, true);
    loadWsg(CC_MENU_ARROW_LEFT_WSG, &renderer->wsg.arrowLeft, true);
    loadWsg(CC_MENU_ARROW_RIGHT_WSG, &renderer->wsg.arrowRight, true);

    renderer->menuFont         = menuFont;
    renderer->titleFont        = titleFont;
    renderer->titleFontOutline = titleFontOutline;

    return renderer;
}

void deinitMenuCosCrunchRenderer(menuCosCrunchRenderer_t* renderer)
{
    freeWsg(&renderer->wsg.pin);
    freeWsg(&renderer->wsg.fold);
    freeWsg(&renderer->wsg.arrowLeft);
    freeWsg(&renderer->wsg.arrowRight);

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
        menuItem_t* item        = node->val;
        int16_t itemWidth       = textWidth(renderer->menuFont, item->label) + MENU_ITEM_PADDING * 2;
        uint16_t optionMaxWidth = 0;
        if (item->options)
        {
            for (int i = 0; i < item->numOptions; i++)
            {
                optionMaxWidth = MAX(optionMaxWidth, textWidth(renderer->menuFont, item->options[i]));
            }
            itemWidth += OPTION_PADDING + renderer->wsg.arrowLeft.w + optionMaxWidth + ARROW_PADDING * 2
                         + renderer->wsg.arrowRight.w;
        }

        int16_t itemX = (TFT_WIDTH - itemWidth) / 2;
        int16_t itemY = itemIdx * (itemHeight + itemMargin) + titleHeight;
        drawMessageBox(itemX, itemY, itemX + itemWidth, itemY + itemHeight, renderer->wsg.fold);
        int16_t optionsX
            = drawText(renderer->menuFont, c000, item->label, itemX + MENU_ITEM_PADDING, itemY + MENU_ITEM_PADDING);

        if (item->options)
        {
            if (item->currentOpt > 0)
            {
                drawWsgSimple(&renderer->wsg.arrowLeft, optionsX + OPTION_PADDING,
                              itemY + (itemHeight - renderer->wsg.arrowLeft.h) / 2);
            }

            uint16_t optionWidth = textWidth(renderer->menuFont, item->options[item->currentOpt]);
            drawText(renderer->menuFont, c000, item->options[item->currentOpt],
                     optionsX + OPTION_PADDING + renderer->wsg.arrowLeft.w + ARROW_PADDING
                         + (optionMaxWidth - optionWidth) / 2,
                     itemY + MENU_ITEM_PADDING);

            if (item->currentSetting != item->maxSetting)
            {
                drawWsgSimple(&renderer->wsg.arrowRight,
                              optionsX + OPTION_PADDING + renderer->wsg.arrowLeft.w + ARROW_PADDING * 2
                                  + optionMaxWidth,
                              itemY + (itemHeight - renderer->wsg.arrowRight.h) / 2);
            }
        }

        if (node->val == menu->currentItem->val)
        {
            drawWsgSimple(&renderer->wsg.pin, itemX - 8, itemY - 4);
        }

        node = node->next;
        itemIdx++;
    }
}
