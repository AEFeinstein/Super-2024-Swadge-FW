#ifndef _PAINT_BROWSER_H_
#define _PAINT_BROWSER_H_

#include "linked_list.h"
#include "font.h"
#include "wsg.h"
#include "hdw-btn.h"
#include "textEntry.h"
#include <nvs.h>

typedef enum
{
    BROWSER_EXIT,
    BROWSER_OPEN,
    BROWSER_SAVE,
    BROWSER_DELETE,
} imageBrowserAction_t;

typedef enum
{
    BROWSER_GRID = 0,
    BROWSER_GALLERY,
    BROWSER_FULLSCREEN,
} imageBrowserView_t;

typedef void (*imageBrowserCbFn)(const char* nvsKey, imageBrowserAction_t action);

typedef struct
{
    node_t* currentItem;
    list_t items;

    const font_t* font;
    const char* title;

    textEntry_t* textEntry;
    bool showTextEntry;
    int64_t prevUpdateTime;

    char namespace[16];
    char mainImageKey[16];
    wsg_t mainImage;

    imageBrowserCbFn callback;
    imageBrowserAction_t primaryAction;
    imageBrowserAction_t secondaryAction;
    imageBrowserView_t viewStyle;
    bool wraparound;

    uint8_t cols;

    uint8_t firstRow;
} imageBrowser_t;

void setupImageBrowser(imageBrowser_t* browser, const font_t* font, const char* namespace, const char* prefix,
                       imageBrowserAction_t action, imageBrowserAction_t secondaryAction);
void resetImageBrowser(imageBrowser_t* browser);
void drawImageBrowser(imageBrowser_t* browser);
void imageBrowserButton(imageBrowser_t* browser, const buttonEvt_t* evt);

#endif
