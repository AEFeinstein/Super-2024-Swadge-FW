#pragma once

//==============================================================================
// Includes
//==============================================================================

// Core
#include "cnfs.h"

// Camp Idle
#include "ci_genericData.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CI_SMALL,
    CI_MED,
    CI_LARGE
} ci_itemSizes_t;

typedef enum
{
    CI_FOOD,
    CI_BAD_FOOD,
    CI_CRAFTED,
    CI_FORAGED,
    CI_HEALING,
    CI_TYPE_ALL
} ci_itemTypes_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    cnfsFileIdx_t image;
    ci_itemTypes_t type;
    ci_locations_t loc;
    ci_itemSizes_t size;
    int weight;
    int value; // Changes based on type
    const char* title;
    const char* desc;
} ci_item_t;

typedef struct
{
    cnfsFileIdx_t image;
    ci_itemTypes_t type;
    const char* title;
    const char* desc;
} ci_craftedItem_t;

typedef struct
{
    const ci_item_t* badFood;
    const ci_craftedItem_t* crafted;
    const ci_item_t* food;
    const ci_item_t* foraged;
    const ci_craftedItem_t* healing;
} ci_itemDataList;

//==============================================================================
// Consts
//==============================================================================

extern const char* const bfTitles[];

extern const char* const bfDescriptions[];

extern const ci_item_t ci_badFoodData[];

extern const char* const cTitles[];

extern const char* const cDescriptions[];

extern const ci_craftedItem_t ci_craftedData[];

extern const char* const fTitles[];

extern const char* const fDescriptions[];

extern const ci_item_t ci_foodData[];

extern const char* const fdTitles[];

extern const char* const fdDescriptions[];

extern const ci_item_t ci_foragedData[];

extern const char* const hTitles[];

extern const char* const hDescriptions[];

extern const ci_craftedItem_t ci_healingData[];

extern const ci_itemDataList ci_itemList;

//==============================================================================
// Functions
//==============================================================================

int ci_getArrayLength(ci_itemTypes_t type);