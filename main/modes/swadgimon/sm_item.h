#ifndef _SM_ITEM_H_
#define _SM_ITEM_H_

// Bag pocket an item is stored in
enum {
    POCKET_ITEMS,
    POCKET_KEY_ITEMS,
    POCKET_BALLS,
    POCKET_RESTORATIVES
    POCKET_TMS_HMS
} pocket_t;

// Definitions of items
struct {
    uint16_t id;
    char* name;
    char* description;
    uint16_t price;
    uint8_t parameter;
    pocket_t pocket;
    bool isAssignableToSelect;
    // TODO: function pointers?
} item_t;

#endif
