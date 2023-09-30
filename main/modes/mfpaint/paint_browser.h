#ifndef _PAINT_BROWSER_H_
#define _PAINT_BROWSER_H_

#include "linked_list.h"
#include "font.h"
#include "wsg.h"
#include "hdw-btn.h"
#include <nvs.h>

typedef void (*imageBrowserCbFn_t)(const char* nvsKey);

typedef struct
{
    node_t* currentItem;
    list_t items;

    imageBrowserCbFn_t callback;

    uint8_t cols;

    uint8_t firstRow;
} imageBrowser_t;

typedef struct
{
    wsg_t thumb;
    wsg_t preview;

    char nvsKey[NVS_KEY_NAME_MAX_SIZE];
} imageBrowserItem_t;

void setupImageBrowser(imageBrowser_t* browser, const char* namespace, const char* prefix);
void resetImageBrowser(imageBrowser_t* browser);
void drawImageBrowser(const imageBrowser_t* browser, const font_t* font, bool showPreview);
void imageBrowserButton(imageBrowser_t* browser, const buttonEvt_t* evt);

#endif
