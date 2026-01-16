#ifndef _SM_ITEM_H_
#define _SM_ITEM_H_

#include <stdbool.h>
#include <stdint.h>

// Bag pocket an item is stored in
typedef enum {
    POCKET_ITEMS,
    POCKET_KEY_ITEMS,
    POCKET_BALLS,
    POCKET_RESTORATIVES,
    POCKET_TMS_HMS,
} pocket_t;

// Definitions of items
typedef struct {
    const char* const name;
    const char* const description;
    const pocket_t pocket;
    const uint16_t id;
    const uint16_t price;
    const uint8_t parameter;
    const bool isAssignableToSelect;
    // TODO: function pointers?
} item_t;

#endif
