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
#define PANEL_H 70

/// The panel corner-radius
#define PANEL_R 15

#define TEXT_MARGIN 10

#define PANEL_BG_COLOR            c200
#define PANEL_BORDER_COLOR        c532
#define PANEL_ICON_BG_COLOR_SEL   c411
#define PANEL_ICON_BG_COLOR_UNSEL c311
#define PANEL_ICON_BG_COLOR_OFF   c222
#define PANEL_BOX_COLOR_SEL       c532
#define PANEL_BOX_COLOR_UNSEL     c200
#define PANEL_TEXT_COLOR          c542

#define ICON_W      16
#define ICON_H      16
#define ICON_BOX_PX 2

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    /// @brief The menu item label these settings are for
    const char* label;
    /// @brief The icon to draw when the setting is on
    const wsg_t* onWsg;
    /// @brief The icon to draw when the setting is off
    const wsg_t* offWsg;
    /// @brief An alternate label to use for the min value, or NULL for the default
    const char* minLabel;
    /// @brief An alternate label to use for the max value, or NULL for the default
    const char* maxLabel;
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
    node_t* info = NULL;
    while ((info = pop(&renderer->iconMap)))
    {
        free(info);
    }

    free(renderer);
}

void drawMenuQuickSettings(menu_t* menu, menuQuickSettingsRenderer_t* renderer, int64_t elapsedUs)
{
    node_t* node  = menu->items->first;
    uint8_t index = 0;

    // Draw the panel background
    uint16_t panelX = (TFT_WIDTH - PANEL_W) / 2;
    fillDisplayArea(panelX, 0, panelX + PANEL_W, PANEL_H - PANEL_R, PANEL_BG_COLOR);
    fillDisplayArea(panelX + PANEL_R, PANEL_H - PANEL_R, panelX + PANEL_W - PANEL_R, PANEL_H, PANEL_BG_COLOR);
    drawCircleFilled(panelX + PANEL_R - 1, PANEL_H - PANEL_R - 1, PANEL_R, PANEL_BG_COLOR);
    drawCircleFilled(panelX + PANEL_W - PANEL_R, PANEL_H - PANEL_R - 1, PANEL_R, PANEL_BG_COLOR);

    // Draw the panel outline
    drawLineFast(panelX - 1, 0, panelX - 1, PANEL_H - PANEL_R, PANEL_BORDER_COLOR);
    drawLineFast(panelX + PANEL_W, 0, panelX + PANEL_W, PANEL_H - PANEL_R, PANEL_BORDER_COLOR);
    drawLineFast(panelX + PANEL_R, PANEL_H, panelX + PANEL_W - PANEL_R, PANEL_H, PANEL_BORDER_COLOR);
    drawCircleQuadrants(panelX + PANEL_R - 1, PANEL_H - PANEL_R, PANEL_R, false, true, false, false,
                        PANEL_BORDER_COLOR);
    drawCircleQuadrants(panelX + PANEL_W - PANEL_R, PANEL_H - PANEL_R, PANEL_R, true, false, false, false,
                        PANEL_BORDER_COLOR);

    while (node != NULL)
    {
        menuItem_t* item = (menuItem_t*)(node->val);
        const quickSettingsItemInfo_t* info
            = getInfoForLabel(renderer, item->label ? item->label : item->options[item->currentOpt]);
        const wsg_t* wsgToDraw = renderer->defaultIcon;
        const char* label      = NULL;
        bool off               = false;

        if (info != NULL)
        {
            if (item->currentSetting <= item->minSetting)
            {
                // Setting is at the minimum, use the "off" icon
                wsgToDraw = info->offWsg;
                off       = true;
            }
            else
            {
                wsgToDraw = info->onWsg;
            }

            if (info->minLabel && item->currentSetting == item->minSetting)
            {
                label = info->minLabel;
            }
            else if (info->maxLabel && item->currentSetting == item->maxSetting)
            {
                label = info->maxLabel;
            }
        }

        bool selected = (item == menu->currentItem->val);

        // Draw selected icon
        uint16_t iconX = panelX + index * PANEL_W / (menu->items->length)
                         + ((PANEL_W - ICON_W * menu->items->length) / menu->items->length / 2);
        uint16_t iconY = (PANEL_H - renderer->font->height - TEXT_MARGIN - 1 - ICON_H - ICON_BOX_PX * 2) / 2;
        // Background behind icon
        // PANEL_ICON_BG_COLOR_[UN]SEL
        fillDisplayArea(iconX - 1, iconY - 1, iconX + ICON_W + 1, iconY + ICON_H + 1,
                        selected ? PANEL_ICON_BG_COLOR_SEL
                                 : (off ? PANEL_ICON_BG_COLOR_OFF : PANEL_ICON_BG_COLOR_UNSEL));
        if (wsgToDraw)
        {
            drawWsgSimple(wsgToDraw, iconX, iconY);
        }

        drawRect(iconX - 2, iconY - 2, iconX + ICON_W + 2, iconY + ICON_H + 2,
                 selected ? PANEL_BOX_COLOR_SEL : PANEL_BOX_COLOR_UNSEL);

        if (selected)
        {
            char buffer[64] = {0};
            if (NULL == label)
            {
                label = getMenuItemLabelText(buffer, sizeof(buffer), item);
            }

            drawText(renderer->font, PANEL_TEXT_COLOR, label, panelX + TEXT_MARGIN,
                     PANEL_H - renderer->font->height - TEXT_MARGIN - 1);
        }

        node = node->next;
        index++;
    }
}

void quickSettingsRendererCustomizeOption(menuQuickSettingsRenderer_t* renderer, const char* label, const wsg_t* onWsg,
                                          const wsg_t* offWsg, const char* maxLabel, const char* minLabel)
{
    quickSettingsItemInfo_t* info = calloc(1, sizeof(quickSettingsItemInfo_t));

    info->label    = label;
    info->onWsg    = onWsg;
    info->offWsg   = offWsg;
    info->minLabel = minLabel;
    info->maxLabel = maxLabel;

    push(&renderer->iconMap, info);
}
