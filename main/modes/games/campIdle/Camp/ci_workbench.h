#pragma once

//==============================================================================
// Include
//==============================================================================

// Swadge
#include "wsg.h"

// Camp
#include "ci_itemData.h"
#include "ci_items.h"

//==============================================================================
// Defines
//==============================================================================

#define MAX_WORKBENCH_ITEMS 3

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CI_HEARTMAKER,
    CI_M_SPHERE_1,
    CI_M_SPHERE_2,
    CI_M_SPHERE_3,
    CI_M_WORKBENCH,
    CI_POLISHER,
    CI_POLISHER_BOX,
    CI_SMASHER,
    CI_SMASHER_HAMMER,
    CI_SMELTER,
    CI_SMELTER_FIRE_1,
    CI_SMELTER_FIRE_2,
    CI_SMELTER_FIRE_3,
    CI_SMELTER_WOOD,
    CI_STONECUTTER,
    CI_TANNING_RACK,
    CI_TANNING_RACK_PELT,
    CI_WEAVER,
    CI_WORKBENCH,
} ciWorkbenchImageEnum_t;

typedef enum
{
    CI_CRAFT_CRYSTAL_POLISHER,
    CI_CRAFT_HEARTMAKER,
    CI_CRAFT_MAGIC_WORKBENCH,
    CI_CRAFT_SMASHER,
    CI_CRAFT_SMELTER,
    CI_CRAFT_STONE_CUTTER,
    CI_CRAFT_TANNING_RACK,
    CI_CRAFT_WEAVER,
    CI_CRAFT_WORKBENCH,
} ciWorkbenchCraft_t;

//==============================================================================
// Struct
//==============================================================================

typedef struct
{
    const char* title;
    const char* desc;
    ciContainerItem_t items[MAX_WORKBENCH_ITEMS];
} ciWorkbench_t;

typedef struct
{
    wsg_t* benchImages;
    int16_t benches;
} ciWorkbenchData_t;

//==============================================================================
// Const
//==============================================================================

extern const ciWorkbench_t workbenchList[];

//==============================================================================
// Function definitions
//==============================================================================

/**
 * @brief Loads workbench data
 *
 * @param wbd Workbench Data
 */
void ciInitWorkbenchImages(ciWorkbenchData_t* wbd);

/**
 * @brief Fress the workbench data
 *
 * @param wbd Workbench Data
 */
void ciFreeWorkbenchImages(ciWorkbenchData_t* wbd);

/**
 * @brief Loads the workbenches from NVS
 *
 * @param wbd Workbench Data
 */
void ciLoadWorkbenchFromNVS(ciWorkbenchData_t* wbd);

/**
 * @brief Attempts to add a new workbench
 *
 * @param wbd Workbench Data
 * @param wb Workbench idx
 */
void ciSetWorkbench(ciWorkbenchData_t* wbd, ciInventory_t* inv, ciWorkbenchCraft_t wb);

/**
 * @brief Draws a workbench at the desired location
 *
 * @param wbd Workbench Data
 * @param x x Position
 * @param y y position
 * @param wb Workbench to draw
 * @param scale Scale to draw at
 * @param stage Used for animations
 */
void drawWorkbench(ciWorkbenchData_t* wbd, int x, int y, ciWorkbenchCraft_t wb, int scale, int stage);

/**
 * @brief Get the Wsg for a crafting station
 *
 * @param wbd Workbench Data
 * @param wb Workbench
 * @return wsg_t* Image found. NULL if not found
 */
wsg_t* getWsg(ciWorkbenchData_t* wbd, ciWorkbenchCraft_t wb);