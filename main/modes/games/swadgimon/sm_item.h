#ifndef _SM_ITEM_H_
#define _SM_ITEM_H_

#include <stdbool.h>

// Bag pocket an item is stored in
typedef enum {
    POCKET_ITEMS,
    POCKET_KEY_ITEMS,
    POCKET_BALLS,
    POCKET_RESTORATIVES
    POCKET_TMS_HMS
} pocket_t;

// Definitions of items
typedef struct {
    char* name;
    char* description;
    pocket_t pocket;
    uint16_t id;
    uint16_t price;
    uint8_t parameter;
    bool isAssignableToSelect;
    // TODO: function pointers?
} item_t;

#endif
