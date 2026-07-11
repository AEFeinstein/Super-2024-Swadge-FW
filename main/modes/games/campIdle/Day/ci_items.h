// Items:
// - Draw items
// - Load data
// - Inventory

#pragma once

//==============================================================================
// Includes
//==============================================================================

// CI
#include "ci_itemData.h"
#include "wsg.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct 
{
    const ci_item_t* item;
    int qty;
    bool discovered;
} ci_invItem_t;

typedef struct 
{
    wsg_t* testImage;
    ci_invItem_t* inventory;
} ci_inventory_t;

//==============================================================================
// Function Definitions
//==============================================================================

// Inventory manipulation
// Initialize inventory
void ci_initInv(ci_inventory_t* inv);

// Free Inventory
void ci_freeInv();

// Load inventory from NVS
void ci_loadInv();

// Save inventory to NVS
void ci_saveInv();

// Add or remove items
void ci_addToInv(ci_item_t* item, int qty);

// Draw
// Draws inventory screen
void ci_drawInv(ci_itemTypes_t type);

// Draws item at specified location
void ci_drawItem(ci_item_t* item, int x, int y);