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

//==============================================================================
// Structs
//==============================================================================

typedef struct 
{
    ci_item_t item;
    int qty;
} ci_invItem_t;

typedef struct 
{
    ci_invItem_t* food; // Includes bad food
    ci_invItem_t* healing;
    ci_invItem_t* items;
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