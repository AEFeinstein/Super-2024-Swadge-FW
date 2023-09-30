//#include <nvs.h>
#include <string.h>
#include <inttypes.h>

#include "paint_browser.h"
#include "linked_list.h"
#include "spiffs_wsg.h"
#include "hdw-nvs.h"
#include "hdw-tft.h"
#include "macros.h"
#include "esp_heap_caps.h"
#include "wsg.h"
#include "font.h"
#include "hdw-btn.h"
#include "shapes.h"
#include "fill.h"
#include "esp_log.h"

#define THUMB_W 35
#define THUMB_H 30

#define SCROLL_OFFSET 30
#define SCROLL_HEIGHT (TFT_HEIGHT - SCROLL_OFFSET * 2)

static bool makeThumbnail(wsg_t* thumbnail, uint16_t w, uint16_t h, const wsg_t* image, bool spiram);

static bool makeThumbnail(wsg_t* thumbnail, uint16_t w, uint16_t h, const wsg_t* image, bool spiram)
{
    thumbnail->w = MIN(w, image->w);
    thumbnail->h = MIN(h, image->h);

    ESP_LOGI("Paint", "Thumbnail is %" PRIu16 " x %" PRIu16, thumbnail->w, thumbnail->h);
    ESP_LOGI("Paint", "Image is %" PRIu16 " x %" PRIu16, image->w, image->h);

    thumbnail->px
        = heap_caps_malloc(sizeof(paletteColor_t) * thumbnail->w * thumbnail->h, spiram ? MALLOC_CAP_SPIRAM : 0);

    if (thumbnail->w == image->w && thumbnail->h == image->h)
    {
        // Just copy the image, it's the same size as the thumbnail
        memcpy(thumbnail->px, image->px, image->w * image->h * sizeof(paletteColor_t));
    }
    else
    {
        uint16_t hScale = MAX(1, image->w / thumbnail->w);
        uint16_t vScale = MAX(1, image->h / thumbnail->h);

        // Sample for the thumbnail pixels
        for (uint16_t r = 0; r < thumbnail->h; ++r)
        {
            for (uint16_t c = 0; c < thumbnail->w; ++c)
            {
                // TODO average the surrounding pixels instead of doing nothing
                thumbnail->px[r * thumbnail->w + c] = image->px[r * image->w * vScale + c * hScale];
            }
        }
    }

    return true;
}

void setupImageBrowser(imageBrowser_t* browser, const char* namespace, const char* prefix)
{
    size_t numInfos = 0;
    readAllNvsEntryInfos(NULL, NULL, &numInfos);
    ESP_LOGI("Paint", "%" PRIu64 " NVS entries", (uint64_t)numInfos);

    nvs_entry_info_t imageInfos[numInfos] = {};

    nvs_entry_info_t* imagePtrs[numInfos] = {};
    for (size_t i = 0; i < numInfos; i++)
    {
        imagePtrs[i] = &imageInfos[i];
    }

    ESP_LOGI("Paint", "imageInfos is %" PRIu64 " bytes", (uint64_t)sizeof(imageInfos));
    if (readAllNvsEntryInfos(NULL, &imagePtrs, &numInfos))
    {
        ESP_LOGI("Paint", "readAll success");
        for (uint32_t i = 0; i < numInfos; ++i)
        {
            if (imageInfos[i].type == NVS_TYPE_BLOB)
            {
                if (!namespace
                    || !memcmp(namespace, imageInfos[i].namespace_name, MIN(strlen(namespace), 16)))
                {
                    if (!prefix || !memcmp(imageInfos[i].key, prefix, MIN(strlen(prefix), 16)))
                    {
                        imageBrowserItem_t* newItem = calloc(1, sizeof(imageBrowserItem_t));

                        bool result = loadWsgNvs(imageInfos[i].namespace_name, imageInfos[i].key, &newItem->preview, true);
                        ESP_LOGI("Paint", "loadWsgNvs result is %s", result ? "true" : "false");

                        // Make a thumbnail
                        makeThumbnail(&newItem->thumb, THUMB_W, THUMB_H, &newItem->preview, false);

                        strncpy(newItem->nvsKey, imageInfos[i].key, sizeof(newItem->nvsKey) - 1);

                        push(&browser->items, newItem);
                        if (!browser->currentItem)
                        {
                            browser->currentItem = browser->items.last;
                        }
                    }
                }
            }
        }
    }
}

void resetImageBrowser(imageBrowser_t* browser)
{
    browser->currentItem = NULL;

    imageBrowserItem_t* item;
    while (NULL != (item = pop(&browser->items)))
    {
        free(item->thumb.px);
        free(item->preview.px);
        free(item);
    }
}

void drawImageBrowser(const imageBrowser_t* browser, const font_t* font, bool showPreview)
{
    // Calculate the total number of full or partial rows
    uint8_t rows = (browser->items.length + (browser->cols - 1)) / browser->cols;

    uint16_t marginTop    = 15;
    uint16_t textMargin = 5;
    uint16_t marginBottom = 15 + font->height + 1 + textMargin;
    uint16_t marginLeft = 15;
    uint16_t marginRight = 15;
    uint16_t thumbMargin  = 5;
    uint16_t thumbHeight  = THUMB_H + 4;
    uint16_t thumbWidth   = THUMB_W + 4;

    // The last visible row is the first row, plus the number of rows (== spaceForRows / spacePerRow)
    uint8_t visibleRows    = (TFT_HEIGHT - marginTop - marginBottom - thumbMargin) / (thumbHeight + thumbMargin);
    uint8_t lastVisibleRow = browser->firstRow + visibleRows;

    // Calculate the scrollbar height
    uint16_t scrollHeight = rows ? visibleRows * SCROLL_HEIGHT / rows : SCROLL_HEIGHT;
    uint16_t scrollOffset = SCROLL_OFFSET + browser->firstRow * scrollHeight;

    // Fill the whole screen with a nice gray
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c444);

    // Draw the scroll bar on the right side
    // Border
    drawRect(TFT_WIDTH - 4, SCROLL_OFFSET, TFT_WIDTH - 1, TFT_HEIGHT - SCROLL_OFFSET, c000);

    // Scrollbar
    fillDisplayArea(TFT_WIDTH - 3, scrollOffset, TFT_WIDTH - 1, scrollOffset + scrollHeight, c000);

    node_t* node = browser->items.first;
    for (uint8_t row = browser->firstRow; row < rows; ++row)
    {
        if (row >= lastVisibleRow)
        {
            break;
        }

        for (uint8_t col = 0; col < browser->cols && NULL != node; ++col, node = node->next)
        {
            // Skip through the items until we are on a visible row
            if (row < browser->firstRow)
            {
                continue;
            }

            imageBrowserItem_t* item = (imageBrowserItem_t*)node->val;
            bool selected            = (node == browser->currentItem);

            // TODO divide by 2 after ... `+ thumbMargin)`
            // x = marginLeft + (lSpacing * (col + 1)) + (width) *
            uint16_t x
                = marginLeft + col * (TFT_WIDTH - marginLeft - marginRight - browser->cols * (thumbWidth + thumbMargin) + thumbWidth) / browser->cols;
            x = marginLeft + col * (thumbWidth * thumbMargin);
            uint16_t y = marginTop
                         + row * (TFT_HEIGHT - marginTop - marginBottom - visibleRows * (thumbHeight + thumbMargin))
                               / visibleRows;

            // Background
            fillDisplayArea(x + 2, y + 2, x + 2 + THUMB_W, y + 2 + THUMB_H, c333);
            // Make a checkerboard for any transparency
            shadeDisplayArea(x + 2, y + 2, x + 2 + THUMB_W, y + 2 + THUMB_H, 2, c111);

            // Draw thumb
            drawWsgSimple(&item->thumb, x + 2, y + 2);

            // Draw inner border
            drawRect(x + 1, y + 1, x + 2 + THUMB_W + 1, y + 2 + THUMB_H + 1, c555);
            // Draw outer border
            drawRect(x, y, x + 2 + THUMB_W + 2, y + 2 + THUMB_H + 2, selected ? c455 : c000);

            // Draw the label of the selected item
            if (selected)
            {
                drawText(font, c000, item->nvsKey, (TFT_WIDTH - textWidth(font, item->nvsKey)) / 2, TFT_HEIGHT - marginBottom + textMargin);
            }
        }
    }
}

void imageBrowserButton(imageBrowser_t* browser, const buttonEvt_t* evt)
{
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_UP:
            {
                for (uint8_t i = 0; i < browser->cols; i++)
                {
                    if (NULL == browser->currentItem)
                    {
                        browser->currentItem = browser->items.last;
                    }
                    else if (NULL != browser->currentItem->prev)
                    {
                        browser->currentItem = browser->currentItem->prev;
                    }
                }
                break;
            }

            case PB_DOWN:
            {
                for (uint8_t i = 0; i < browser->cols; i++)
                {
                    if (NULL == browser->currentItem)
                    {
                        browser->currentItem = browser->items.first;
                    }
                    else if (NULL != browser->currentItem->next)
                    {
                        browser->currentItem = browser->currentItem->next;
                    }
                }
                break;
            }

            case PB_LEFT:
            {
                if (NULL == browser->currentItem)
                {
                    browser->currentItem = browser->items.last;
                }
                else if (NULL != browser->currentItem->prev)
                {
                    browser->currentItem = browser->currentItem->prev;
                }
                break;
            }

            case PB_RIGHT:
            {
                if (NULL == browser->currentItem)
                {
                    browser->currentItem = browser->items.first;
                }
                else if (NULL != browser->currentItem->next)
                {
                    browser->currentItem = browser->currentItem->next;
                }
                break;
            }

            case PB_A:
            {
                if (browser->callback && browser->currentItem)
                {
                    browser->callback(((imageBrowserItem_t*)browser->currentItem->val)->nvsKey);
                }
                break;
            }

            case PB_B:
            {
                break;
            }

            case PB_START:
            case PB_SELECT:
                break;
        }
    }
}
