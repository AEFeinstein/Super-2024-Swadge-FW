#ifndef _PAINT_BROWSER_H_
#define _PAINT_BROWSER_H_

#include "linked_list.h"
#include "font.h"
#include "wsg.h"
#include "hdw-btn.h"
#include "textEntry.h"
#include <nvs.h>

typedef void (*imageBrowserCbFn)(const char* nvsKey);

typedef struct
{
    node_t* currentItem;
    list_t items;

    const font_t* font;

    textEntry_t* textEntry;
    bool showTextEntry;
    int64_t prevUpdateTime;

    imageBrowserCbFn callback;

    uint8_t cols;

    uint8_t firstRow;
} imageBrowser_t;

void setupImageBrowser(imageBrowser_t* browser, const font_t* font, const char* namespace, const char* prefix, bool addNewButton);
void resetImageBrowser(imageBrowser_t* browser);
void drawImageBrowser(imageBrowser_t* browser);
void imageBrowserButton(imageBrowser_t* browser, const buttonEvt_t* evt);

#endif
