#include "menuQuickSettingsRenderer.h"
#include "menu_utils.h"

#include "hdw-tft.h"
#include "shapes.h"
#include "fill.h"
#include "wsg.h"
#include "font.h"
#include "menu.h"
#include "palette.h"
#include "esp_log.h"

#include <malloc.h>
#include <stdint.h>
#include <inttypes.h>

//==============================================================================
// Defines
//==============================================================================

/// The width of the panel, centered in the screen
#define PANEL_W (TFT_WIDTH - 60)

/// The height of the panel, from the top of the screen
#define PANEL_H 60

/// The panel corner-radius
#define PANEL_R 15

#define PANEL_BG_COLOR c333
#define PANEL_BOX_COLOR c000
#define PANEL_TEXT_COLOR c000

#define ICON_W 16
#define ICON_H 16
#define ICON_BOX_PX 2


//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    /// @brief The menu item label these
    const char* label;
    /// @brief The icon to draw when the setting is on
    const wsg_t* onWsg;
    /// @brief The icon to draw when the setting is off
    const wsg_t* offWsg;
} quickSettingsItemInfo_t;

//==============================================================================
// Static Function Prototypes
//==============================================================================

static const quickSettingsItemInfo_t* getInfoForLabel(menuQuickSettingsRenderer_t* renderer, const char* label);

//==============================================================================
// Static Functions
//==============================================================================

static const quickSettingsItemInfo_t* getInfoForLabel(menuQuickSettingsRenderer_t* renderer, const char* label)
{
    node_t* node = renderer->iconMap.first;

    while (node != NULL)
    {
        quickSettingsItemInfo_t* info = (quickSettingsItemInfo_t*)(node->val);

        // The labels should be the exact same strings, so it's fine to compare like this
        if (info->label == label)
        {
            return info;
        }

        node = node->next;
    }

    return NULL;
}

//==============================================================================
// Functions
//==============================================================================

menuQuickSettingsRenderer_t* initMenuQuickSettingsRenderer(font_t* font)
{
    menuQuickSettingsRenderer_t* renderer = calloc(1, sizeof(menuQuickSettingsRenderer_t));

    renderer->font = font;
    return renderer;
}

void deinitMenuQuickSettingsRenderer(menuQuickSettingsRenderer_t* renderer)
{
    clear(&renderer->iconMap);
    free(renderer);
}

void drawMenuQuickSettings(menu_t* menu, menuQuickSettingsRenderer_t* renderer, int64_t elapsedUs)
{
    node_t* node = menu->items->first;
    uint8_t index = 0;

    // Draw the panel background
    uint16_t panelX = (TFT_WIDTH - PANEL_W) / 2;
    fillDisplayArea(panelX, 0, panelX + PANEL_W, PANEL_H - PANEL_R, PANEL_BG_COLOR);
    fillDisplayArea(panelX + PANEL_R, PANEL_H - PANEL_R, panelX + PANEL_W - PANEL_R, PANEL_H, PANEL_BG_COLOR);
    //drawCircleQuadrants(panelX + PANEL_R, PANEL_H - PANEL_R, PANEL_R, false, false, false, true, PANEL_BG_COLOR);
    //drawCircleQuadrants(panelX + PANEL_W - PANEL_R, PANEL_H - PANEL_R, PANEL_R, false, false, true, false, PANEL_BG_COLOR);
    drawCircleFilled(panelX + PANEL_R, PANEL_H - PANEL_R - 1, PANEL_R, PANEL_BG_COLOR);
    drawCircleFilled(panelX + PANEL_W - PANEL_R - 1, PANEL_H - PANEL_R - 1, PANEL_R, PANEL_BG_COLOR);


    while (node != NULL)
    {
        menuItem_t* item = (menuItem_t*)(node->val);
        const quickSettingsItemInfo_t* info = getInfoForLabel(renderer, item->label ? item->label : item->options[item->currentOpt]);
        const wsg_t* wsgToDraw = renderer->defaultIcon;

        if (info != NULL)
        {
            if (item->currentSetting <= item->minSetting)
            {
                // Setting is at the minimum, use the "off" icon
                wsgToDraw = info->offWsg;
            }
            else
            {
                wsgToDraw = info->onWsg;
            }
        }

        // Draw selected icon
        uint16_t iconX = panelX + index * PANEL_W / (menu->items->length) + ((PANEL_W - ICON_W * menu->items->length) / menu->items->length / 2);
        uint16_t iconY = (PANEL_H - renderer->font->height - 5 - 1 - wsgToDraw->h - ICON_BOX_PX * 2) / 2;
        drawWsgSimple(wsgToDraw, iconX, iconY);

        if (item == menu->currentItem->val)
        {
            char buffer[64] = {0};
            const char* label = getMenuItemLabelText(buffer, sizeof(buffer), item);

            drawRect(iconX - 1, iconY - 1, iconX + ICON_W + 1, iconY + ICON_H + 1, PANEL_BOX_COLOR);

            drawText(renderer->font, PANEL_TEXT_COLOR, label, panelX + 5, PANEL_H - renderer->font->height - 5);
        }

        node = node->next;
        index++;
    }
}

void quickSettingsRendererAddIcon(menuQuickSettingsRenderer_t* renderer, const char* label, const wsg_t* onWsg, const wsg_t* offWsg)
{
    quickSettingsItemInfo_t* info = calloc(1, sizeof(quickSettingsItemInfo_t));

    info->label = label;
    info->onWsg = onWsg;
    info->offWsg = offWsg;

    push(&renderer->iconMap, info);
}

