#include <string.h>
#include <inttypes.h>

#include "paint_browser.h"
#include "paint_common.h"
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
#include "esp_timer.h"
#include "paint_draw.h"
#include "paint_ui.h"

static const char saveAsNewStr[]      = "New...";
static const char textEntryTitleStr[] = "File Name";
static const char noItemsStr[]        = "No Items";

#define THUMB_W 35
#define THUMB_H 30

#define SCROLL_OFFSET 30
#define SCROLL_HEIGHT (TFT_HEIGHT - SCROLL_OFFSET * 2)

#define ENTRY_W       160
#define ENTRY_Y       60
#define ENTRY_TITLE_Y 40

typedef struct
{
    bool isNewButton;
    wsg_t thumb;

    char nvsKey[NVS_KEY_NAME_MAX_SIZE];
} imageBrowserItem_t;

static bool makeThumbnail(wsg_t* thumbnail, uint16_t w, uint16_t h, const wsg_t* image, bool spiram);
static void imageBrowserTextEntryCb(const char* text, void* data);

static bool makeThumbnail(wsg_t* thumbnail, uint16_t w, uint16_t h, const wsg_t* image, bool spiram)
{
    thumbnail->w = MIN(w, image->w);
    thumbnail->h = MIN(h, image->h);

    thumbnail->px = NULL;
    thumbnail->px
        = heap_caps_malloc(sizeof(paletteColor_t) * thumbnail->w * thumbnail->h, spiram ? MALLOC_CAP_SPIRAM : 0);

    if (NULL == thumbnail->px)
    {
        thumbnail->w = 0;
        thumbnail->h = 0;
        return false;
    }

    if (thumbnail->w == image->w && thumbnail->h == image->h)
    {
        // Just copy the image, it's the same size as the thumbnail
        memcpy(thumbnail->px, image->px, image->w * image->h * sizeof(paletteColor_t));
    }
    else
    {
        uint16_t hScale = MIN(1, image->w / thumbnail->w);
        uint16_t vScale = MIN(1, image->h / thumbnail->h);

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

static void imageBrowserTextEntryCb(const char* text, void* data)
{
    imageBrowser_t* browser = (imageBrowser_t*)data;
    if (browser && browser->callback)
    {
        browser->callback(text, browser->primaryAction);
    }
}

void setupImageBrowser(imageBrowser_t* browser, const font_t* font, const char* namespace, const char* prefix,
                       imageBrowserAction_t action, imageBrowserAction_t secondaryAction)
{
    size_t numInfos = 0;
    if (!readNamespaceNvsEntryInfos(namespace, NULL, NULL, &numInfos))
    {
        PAINT_DIE("Unable to read namespace entry count for namespace %s", namespace);
        return;
    }

    browser->font            = font;
    browser->prevUpdateTime  = 0;
    browser->primaryAction   = action;
    browser->secondaryAction = secondaryAction;

    if (BROWSER_SAVE == action)
    {
        imageBrowserItem_t* newButton = calloc(1, sizeof(imageBrowserItem_t));

        newButton->isNewButton = true;

        loadWsg("wheel_new.wsg", &newButton->thumb, false);
        push(&browser->items, newButton);

        browser->showTextEntry = false;

        if (NULL == browser->textEntry)
        {
            browser->textEntry = initTextEntry((TFT_WIDTH - ENTRY_W) / 2, ENTRY_Y, ENTRY_W, 16, font,
                                               ENTRY_WORD | ENTRY_WHITESPACE, imageBrowserTextEntryCb);
            textEntrySetData(browser->textEntry, browser);
        }

        browser->currentItem = browser->items.last;
    }

    if (numInfos > 0)
    {
        nvs_entry_info_t* imageInfos = calloc(numInfos, sizeof(nvs_entry_info_t));
        if (readNamespaceNvsEntryInfos(namespace, NULL, imageInfos, NULL))
        {
            for (uint32_t i = 0; i < numInfos; ++i)
            {
                if (imageInfos[i].type == NVS_TYPE_BLOB)
                {
                    if (!prefix || !memcmp(imageInfos[i].key, prefix, MIN(strlen(prefix), 16)))
                    {
                        imageBrowserItem_t* newItem = calloc(1, sizeof(imageBrowserItem_t));

                        // Load the image from NVS
                        wsg_t fullImage = {0};

                        if (loadWsgNvs(imageInfos[i].namespace_name, imageInfos[i].key, &fullImage, true))
                        {
                            // Make a thumbnail and add it to the info
                            if (!makeThumbnail(&newItem->thumb, THUMB_W, THUMB_H, &fullImage, true))
                            {
                                PAINT_LOGE("Unable to create thumbnail for %s", imageInfos[i].key);
                            }

                            // Free the full image
                            freeWsg(&fullImage);
                        }
                        else
                        {
                            PAINT_LOGE("Unable to load blob %s", imageInfos[i].key);
                            loadWsg("error.wsg", &newItem->thumb, false);
                        }

                        snprintf(newItem->nvsKey, sizeof(newItem->nvsKey), "%s", imageInfos[i].key);
                        newItem->nvsKey[sizeof(newItem->nvsKey) - 1] = '\0';

                        push(&browser->items, newItem);

                        if (!browser->currentItem)
                        {
                            browser->currentItem = browser->items.last;
                        }
                    }
                }
            }
        }
        free(imageInfos);
    }
}

void resetImageBrowser(imageBrowser_t* browser)
{
    browser->currentItem = NULL;

    imageBrowserItem_t* item;
    while (NULL != (item = pop(&browser->items)))
    {
        free(item->thumb.px);
        free(item);
    }

    if (NULL != browser->textEntry)
    {
        freeTextEntry(browser->textEntry);
        browser->textEntry     = NULL;
        browser->showTextEntry = false;
    }
}

void drawImageBrowser(imageBrowser_t* browser)
{
    // Calculate the total number of full or partial rows
    uint8_t rows = (browser->items.length + (browser->cols - 1)) / browser->cols;

    uint16_t marginTop    = 15;
    uint16_t textMargin   = 5;
    uint16_t marginBottom = 15 + browser->font->height + 1 + textMargin;
    uint16_t marginLeft   = 18;
    uint16_t marginRight  = 18;
    uint16_t thumbMargin  = 5;
    uint16_t thumbHeight  = THUMB_H + 4;
    uint16_t thumbWidth   = THUMB_W + 4;

    // The last visible row is the first row, plus the number of rows (== spaceForRows / spacePerRow)
    uint8_t visibleRows    = (TFT_HEIGHT - marginTop - marginBottom - thumbMargin) / (thumbHeight + thumbMargin);
    uint8_t lastVisibleRow = browser->firstRow + visibleRows - 1;

    uint16_t extraW
        = (TFT_WIDTH - marginLeft - marginRight) - (browser->cols * thumbWidth + (browser->cols - 1) * thumbMargin);
    uint16_t extraH = (TFT_HEIGHT - marginBottom - marginTop)
                      - (visibleRows * thumbHeight + (visibleRows > 0 ? (visibleRows - 1) * thumbMargin : 0));

    // Fill the whole screen with a nice gray
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c444);

    if (browser->textEntry && browser->showTextEntry)
    {
        if (0 == browser->prevUpdateTime)
        {
            browser->prevUpdateTime = esp_timer_get_time();
        }

        int64_t now = esp_timer_get_time();

        textEntryMainLoop(browser->textEntry, now - browser->prevUpdateTime);
        drawText(browser->font, c000, textEntryTitleStr, (TFT_WIDTH - textWidth(browser->font, textEntryTitleStr)) / 2,
                 ENTRY_TITLE_Y);
        drawTextEntry(browser->textEntry, c000, c555, false);

        browser->prevUpdateTime = now;
        return;
    }

    if (rows > visibleRows)
    {
        // Calculate the scrollbar height
        // height = (proportion of rows visible) * (total height)
        uint16_t scrollHeight = rows ? visibleRows * SCROLL_HEIGHT / rows : SCROLL_HEIGHT;
        uint16_t scrollOffset
            = rows ? SCROLL_OFFSET + SCROLL_HEIGHT - (rows - lastVisibleRow - 1) * SCROLL_HEIGHT / rows - scrollHeight
                   : SCROLL_OFFSET;

        // Draw the scroll bar on the right side
        // Border
        drawRect(TFT_WIDTH - 4 - marginRight, SCROLL_OFFSET, TFT_WIDTH - 1 - marginRight, TFT_HEIGHT - SCROLL_OFFSET,
                 c000);

        // Scrollbar
        fillDisplayArea(TFT_WIDTH - 3 - marginRight, scrollOffset, TFT_WIDTH - 1 - marginRight,
                        scrollOffset + scrollHeight, c000);
    }

    node_t* node = browser->items.first;

    if (NULL == node)
    {
        drawText(browser->font, c000, noItemsStr, (TFT_WIDTH - textWidth(browser->font, noItemsStr)) / 2,
                 (TFT_HEIGHT - browser->font->height) / 2);
    }
    else
    {
        for (uint8_t row = 0; row < rows; ++row)
        {
            if (row > lastVisibleRow)
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
                uint16_t x = marginLeft + col * (thumbWidth + thumbMargin) + extraW * col / browser->cols;
                uint16_t y = marginTop + (row - browser->firstRow) * (thumbHeight + thumbMargin)
                             + extraH * (row - browser->firstRow) / rows;
                //+ row * (TFT_HEIGHT - marginTop - marginBottom - visibleRows * (thumbHeight + thumbMargin))
                //    / visibleRows;

                // Background
                fillDisplayArea(x + 2, y + 2, x + 2 + THUMB_W, y + 2 + THUMB_H, c333);

                if (!item->isNewButton)
                {
                    // Make a checkerboard for any transparency
                    shadeDisplayArea(x + 2, y + 2, x + 2 + THUMB_W, y + 2 + THUMB_H, 2, c111);
                }

                // Draw thumb if set
                if (item->thumb.px)
                {
                    drawWsgSimple(&item->thumb, x + 2, y + 2);
                }

                // Draw multiple levels of border - 4 for a selected item, 2 otherwise
                for (int i = 0; i < (selected ? 4 : 2); i++)
                {
                    // Draw a white border first, then either black or blue depending on if it's selected
                    paletteColor_t color = (i == 0) ? c555 : (selected ? c335 : c000);
                    drawRect(x + 1 - i, y + 1 - i, x + 2 + THUMB_W + 1 + i, y + 2 + THUMB_H + 1 + i, color);
                }

                // Draw the label of the selected item
                if (selected)
                {
                    const char* text = item->isNewButton ? saveAsNewStr : item->nvsKey;
                    drawText(browser->font, c000, text, (TFT_WIDTH - textWidth(browser->font, text)) / 2,
                             TFT_HEIGHT - marginBottom + textMargin);
                }
            }
        }
    }
}

void imageBrowserButton(imageBrowser_t* browser, const buttonEvt_t* evt)
{
    if (browser->textEntry && browser->showTextEntry)
    {
        if (evt->down && evt->button == PB_B && '\0' == *(browser->textEntry->value))
        {
            // Text entry is empty, B was pressed, go back
            browser->showTextEntry = false;
        }
        else
        {
            textEntryButton(browser->textEntry, evt);
        }
    }
    else if (evt->down)
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
                if (browser->currentItem)
                {
                    imageBrowserItem_t* item = browser->currentItem->val;

                    if (item->isNewButton)
                    {
                        // Open text dialog
                        browser->showTextEntry = true;
                    }
                    else if (browser->callback)
                    {
                        browser->callback(item->nvsKey, browser->primaryAction);
                    }
                }
                break;
            }

            case PB_B:
            {
                if (browser->callback)
                {
                    browser->callback(NULL, BROWSER_EXIT);
                }
                break;
            }

            case PB_START:
            {
                if (browser->callback)
                {
                    if (browser->currentItem)
                    {
                        imageBrowserItem_t* item = browser->currentItem->val;

                        if (!item->isNewButton)
                        {
                            browser->callback(item->nvsKey, browser->secondaryAction);
                        }
                    }
                }
                break;
            }

            case PB_SELECT:
                break;
        }

        // TODO un-copy-paste this
        uint16_t marginTop    = 15;
        uint16_t textMargin   = 5;
        uint16_t marginBottom = 15 + browser->font->height + 1 + textMargin;
        uint16_t thumbMargin  = 5;
        uint16_t thumbHeight  = THUMB_H + 4;

        // The last visible row is the first row, plus the number of rows (== spaceForRows / spacePerRow)
        uint8_t visibleRows = (TFT_HEIGHT - marginTop - marginBottom - thumbMargin) / (thumbHeight + thumbMargin);

        // Loop over the nodes to find the current one's index
        uint32_t curIndex = 0;
        for (node_t* node = browser->items.first; node != NULL && node != browser->currentItem;
             node         = node->next, curIndex++)
            ;

        // If we're past the final row, advance the first row until it's visible
        while (curIndex / browser->cols >= (browser->firstRow + visibleRows))
        {
            browser->firstRow++;
        }

        // If we're before the first row, decrease the first row until it's visible
        while (browser->firstRow > 0 && curIndex / browser->cols < browser->firstRow)
        {
            browser->firstRow--;
        }
    }
}
