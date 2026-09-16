#pragma once

//==============================================================================
// Includes
//==============================================================================

// Swadge
#include "font.h"
#include "wsg.h"

// Camp
#include "ci_itemData.h"

//==============================================================================
// Defines
//==============================================================================

#define ICON_WIDTH  40
#define ICON_HEIGHT 54

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    wsg_t* itemImages; ///< Item Icons
    int16_t* qtys;     ///< Array of ints used as the inventory
} ciInventory_t;

//==============================================================================
// Function Definitions
//==============================================================================

/**
 * @brief Initializes the inventory
 *
 * @param inv Game Data
 */
void ciInitInventory(ciInventory_t* inv);

/**
 * @brief Frees the inventory
 *
 * @param inv Game Data
 */
void ciFreeInventory(ciInventory_t* inv);

/**
 * @brief Adds an item to the inventory
 *
 * @param inv Game Data
 * @param item Item ID to add
 * @param qty How many of item to add
 * @return int32_t Remainder (If overflowed)
 */
int32_t ciAddToInv(ciInventory_t* inv, ciItemIdx_t item, int qty);

/**
 * @brief Removes an item from the inventory if it exists
 *
 * @param inv Game data
 * @param item Item to remove
 * @param qty How many of item to remove
 * @return true If items were successfully removed
 * @return false If there was not enough items to remove the expected quantity
 */
bool ciRemoveFromInv(ciInventory_t* inv, ciItemIdx_t item, int qty);

/**
 * @brief Draws the item blurb
 *
 * @param inv Game Data
 * @param idx Item to draw
 */
void ciDrawItemPanel(ciInventory_t* inv, font_t* largeFont, font_t* smallFont, ciItemIdx_t idx);

/**
 * @brief Draws a small icon for a particular item
 *
 * @param inv Game Data
 * @param idx Item to draw
 * @param xStart Starting x position
 * @param yStart Starting y position
 * @param qty Quantity to display
 * @param selected If true, it is highlighted
 * @param showQty If true, draws qty. If false, draws abbreviation
 */
void ciDrawItemIcon(ciInventory_t* inv, font_t* smallFont, ciItemIdx_t idx, int xStart, int yStart, int qty,
                    bool selected, bool showQty);