#pragma once

//==============================================================================
// Includes
//==============================================================================

// CI
#include "ci_genericData.h"
#include "wsg.h"

//==============================================================================
// Defines
//==============================================================================

#define ICON_WIDTH  40
#define ICON_HEIGHT 54

//==============================================================================
// Function Definitions
//==============================================================================

/**
 * @brief Initilaizes the inventory
 *
 * @param ccd Game Data
 */
void ciInitInventory(ciCampData_t* ccd);

/**
 * @brief Frees the inventory
 *
 * @param ccd Game Data
 */
void ciFreeInventory(ciCampData_t* ccd);

/**
 * @brief Adds an item to the inventory
 *
 * @param ccd Game Data
 * @param item Item ID to add
 * @param qty How many of itme to add
 * @return int32_t Remainder (If overflowed)
 */
int32_t ciAddToInv(ciCampData_t* ccd, ciItemIdx_t item, int qty);

/**
 * @brief Removes an item from the inventory if it exists
 *
 * @param ccd Game data
 * @param item Item to remove
 * @param qty How many of item to remove
 * @return true If items were successfully removed
 * @return false If there was not enough items to remove the expected quantity
 */
bool ciRemoveFromInv(ciCampData_t* ccd, ciItemIdx_t item, int qty);

/**
 * @brief Draws the item blurb
 *
 * @param ccd Game Data
 * @param idx Item to draw
 */
void ciDrawItemPanel(ciCampData_t* ccd, ciItemIdx_t idx);

/**
 * @brief Draws a small icon for a particualr item
 *
 * @param ccd Game Data
 * @param idx Item to draw
 * @param xStart Starting x position
 * @param yStart Starting y position
 * @param qty Quantity to display
 * @param selected If true, it is highlighted
 * @param showQty If true, draws qty. If false, draws abbrieviation
 */
void ciDrawItemIcon(ciCampData_t* ccd, ciItemIdx_t idx, int xStart, int yStart, int qty, bool selected, bool showQty);