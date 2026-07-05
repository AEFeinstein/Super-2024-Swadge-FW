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

const char* const bfTitles[] = {
    "Floor Pizza", "Furry Milk",    "I Can't Believe It's Not Margarine!", "Malort", "Pilk",
    "Raver Sweat", "Squirrel Nuts", "Your Parent's Lingering Affection",
};

const char* const bfDescriptions[] = {
    "Pizza from the floor. Let's hope nobody stepped on it.",
    "Please don't think about where it came from.",
    "It appears to be all natural butterfly.",
    "There are things that taste worse, but not by much.",
    "On one hand, it's disgusting. On the other hand, PILK!",
    "Ethically sourced, but may contain psychoactive chemicals.",
    "Think one those cosplayers dropped these?",
    "'Of course we love you, honey, but maybe if you'd settle down-'",
};

const ci_item_t ci_badFoodData[] = {
    {
        .image  = CI_FLOOR_PIZZA_WSG,
        .type   = CI_BAD_FOOD,
        .loc    = CI_M_FOREST,
        .size   = CI_MED,
        .weight = 3,
        .value  = -10,
        .title  = bfTitles[0],
        .desc   = bfDescriptions[0],
    },
    {
        .image  = CI_FURRY_MILK_WSG,
        .type   = CI_BAD_FOOD,
        .loc    = CI_SWAMP,
        .size   = CI_SMALL,
        .weight = 1,
        .value  = -30,
        .title  = bfTitles[1],
        .desc   = bfDescriptions[1],
    },
    {
        .image  = CI_ICBINB_WSG,
        .type   = CI_BAD_FOOD,
        .loc    = CI_JUNGLE,
        .size   = CI_SMALL,
        .weight = 2,
        .value  = -50,
        .title  = bfTitles[2],
        .desc   = bfDescriptions[2],
    },
    {
        .image  = CI_MALORT_WSG,
        .type   = CI_BAD_FOOD,
        .loc    = CI_MOUNTAIN,
        .size   = CI_MED,
        .weight = 7,
        .value  = -50,
        .title  = bfTitles[3],
        .desc   = bfDescriptions[3],
    },
    {
        .image  = CI_PILK_WSG,
        .type   = CI_BAD_FOOD,
        .loc    = CI_SWAMP,
        .size   = CI_MED,
        .weight = 4,
        .value  = -30,
        .title  = bfTitles[4],
        .desc   = bfDescriptions[4],
    },
    {
        .image  = CI_RAVER_SWEAT_WSG,
        .type   = CI_BAD_FOOD,
        .loc    = CI_MOUNTAIN,
        .size   = CI_SMALL,
        .weight = 1,
        .value  = -20,
        .title  = bfTitles[5],
        .desc   = bfDescriptions[5],
    },
    {
        .image  = CI_SQUIRREL_NUTS_WSG,
        .type   = CI_BAD_FOOD,
        .loc    = CI_JUNGLE,
        .size   = CI_SMALL,
        .weight = 1,
        .value  = -10,
        .title  = bfTitles[6],
        .desc   = bfDescriptions[6],
    },
    {
        .image  = CI_YPLA_WSG,
        .type   = CI_BAD_FOOD,
        .loc    = CI_M_FOREST,
        .size   = CI_SMALL,
        .weight = 10,
        .value  = -1,
        .title  = bfTitles[7],
        .desc   = bfDescriptions[7],
    },
};

const char* const cTitles[] = {
    "Cloth", "Cut Blocks",      "Diamond",           "Diamond Powder", "Gears", "Iron",
    "Pelts", "Polished Blocks", "Polished Crystals", "Rope",           "Salt",  "String",
};

const char* const cDescriptions[] = {
    "A small bit of cloth",
    "Much easier to buid with",
    "Ooooo. Shiny.",
    "Try not to inhale it",
    "Useful for all manner of mechanical contraptions",
    "Iron ready to use",
    "Yeah, this is definitely pleather",
    "Perfect for a countertop",
    "You can feel a buzz of mysterious energy flowing through it",
    "I can't believe you didn't start with at least 50ft of rope",
    "Na: Explosive. Cl: Highly Acidic. NaCl: Tasty!",
    "If you could dye it red, you might find something important",
};

const ci_craftedItem_t ci_craftedData[] = {
    {
        .image = CI_CLOTH_WSG,
        .type  = CI_CRAFTED,
        .title = cTitles[0],
        .desc  = cDescriptions[0],
    },
    {
        .image = CI_CUT_ROCK_WSG,
        .type  = CI_CRAFTED,
        .title = cTitles[1],
        .desc  = cDescriptions[1],
    },
    {
        .image = CI_DIAMOND_WSG,
        .type  = CI_CRAFTED,
        .title = cTitles[2],
        .desc  = cDescriptions[2],
    },
    {
        .image = CI_DIAMOND_POWDER_WSG,
        .type  = CI_CRAFTED,
        .title = cTitles[3],
        .desc  = cDescriptions[3],
    },
    {
        .image = CI_GEAR_WSG,
        .type  = CI_CRAFTED,
        .title = cTitles[4],
        .desc  = cDescriptions[4],
    },
    {
        .image = CI_IRON_WSG,
        .type  = CI_CRAFTED,
        .title = cTitles[5],
        .desc  = cDescriptions[5],
    },
    {
        .image = CI_HIDE_WSG,
        .type  = CI_CRAFTED,
        .title = cTitles[6],
        .desc  = cDescriptions[6],
    },
    {
        .image = CI_POLISHED_ROCK_WSG,
        .type  = CI_CRAFTED,
        .title = cTitles[7],
        .desc  = cDescriptions[7],
    },
    {
        .image = CI_POLISHED_CRYSTAL_WSG,
        .type  = CI_CRAFTED,
        .title = cTitles[8],
        .desc  = cDescriptions[8],
    },
    {
        .image = CI_ROPE_WSG,
        .type  = CI_CRAFTED,
        .title = cTitles[9],
        .desc  = cDescriptions[9],
    },
    {
        .image = CI_SALT_WSG,
        .type  = CI_CRAFTED,
        .title = cTitles[10],
        .desc  = cDescriptions[10],
    },
    {
        .image = CI_STRING_WSG,
        .type  = CI_CRAFTED,
        .title = cTitles[11],
        .desc  = cDescriptions[11],
    },
};

const char* const fTitles[] = {
    "Apple",         "Beans",        "Berries",
    "Donuts",        "Energy Drink", "Honey",
    "MRE",           "Mushrooms",    "Mystery Meat",
    "Pan Pizza",     "Pickles",      "Protein Powder",
    "Pudding",       "Roast Turkey", "Squeezy Peanut Butter",
    "String Cheese", "Tasty Noods",
};

const char* const fDescriptions[] = {
    "It looks kinda... smooshed, but it should still taste good",
    "I sure hope you brought a can opener",
    "A rich assortment of hopefully non-toxic berries",
    "Donuts are classically a fall fruit, but occasionally you cna find them out of season",
    "Monster Bang Bull: Unleash your Winged Destiny!",
    "Honey? Where is my superfood?",
    "Meal, Ready-to-Eat. Hopefully not the vegetable omelet",
    "Anything is edible at least once",
    "It was just lying there on the ground. Are you sure you want to eat it?",
    "Hot, fresh and ready. Nobody knows who made it",
    "The pickles are reasonable, but where did the jar come from?",
    "GOTTA GO TO THE GYM BRAH. GOTTA GET THEM GAINS BRAH.",
    "Delicious pudding, good thing you have a spoon",
    "There aren't even turkeys in the mountains...",
    "Now you just need some jelly and two slices of bread",
    "Best consumed between the hours of 3 and 4 AM next to a fountain",
    "This wasn't what you expected, but it's still a nice find",
};

const ci_item_t ci_foodData[] = {
    {
        .image  = CI_APPLE_WSG,
        .type   = CI_FOOD,
        .loc    = CI_FOREST,
        .size   = CI_SMALL,
        .weight = 1,
        .value  = 5,
        .title  = fTitles[0],
        .desc   = fDescriptions[0],
    },
    {
        .image  = CI_BEANS_WSG,
        .type   = CI_FOOD,
        .loc    = CI_M_FOREST,
        .size   = CI_SMALL,
        .weight = 4,
        .value  = 100,
        .title  = fTitles[1],
        .desc   = fDescriptions[1],
    },
    {
        .image  = CI_BERRIES_WSG,
        .type   = CI_FOOD,
        .loc    = CI_FOREST,
        .size   = CI_SMALL,
        .weight = 1,
        .value  = 5,
        .title  = fTitles[2],
        .desc   = fDescriptions[2],
    },
    {
        .image  = CI_DONUT_WSG,
        .type   = CI_FOOD,
        .loc    = CI_SWAMP,
        .size   = CI_SMALL,
        .weight = 2,
        .value  = 10,
        .title  = fTitles[3],
        .desc   = fDescriptions[3],
    },
    {
        .image  = CI_ENERGY_DRINK_WSG,
        .type   = CI_FOOD,
        .loc    = CI_SWAMP,
        .size   = CI_SMALL,
        .weight = 4,
        .value  = 30,
        .title  = fTitles[4],
        .desc   = fDescriptions[4],
    },
    {
        .image  = CI_HONEY_WSG,
        .type   = CI_FOOD,
        .loc    = CI_FOREST,
        .size   = CI_SMALL,
        .weight = 3,
        .value  = 30,
        .title  = fTitles[5],
        .desc   = fDescriptions[5],
    },
    {
        .image  = CI_MRE_WSG,
        .type   = CI_FOOD,
        .loc    = CI_MOUNTAIN,
        .size   = CI_MED,
        .weight = 6,
        .value  = 50,
        .title  = fTitles[6],
        .desc   = fDescriptions[6],
    },
    {
        .image  = CI_MUSHROOMS_WSG,
        .type   = CI_FOOD,
        .loc    = CI_JUNGLE,
        .size   = CI_SMALL,
        .weight = 2,
        .value  = 20,
        .title  = fTitles[7],
        .desc   = fDescriptions[7],
    },
    {
        .image  = CI_MYSTERY_MEAT_WSG,
        .type   = CI_FOOD,
        .loc    = CI_JUNGLE,
        .size   = CI_MED,
        .weight = 8,
        .value  = 30,
        .title  = fTitles[8],
        .desc   = fDescriptions[8],
    },
    {
        .image  = CI_PAN_PIZZA_WSG,
        .type   = CI_FOOD,
        .loc    = CI_MOUNTAIN,
        .size   = CI_SMALL,
        .weight = 2,
        .value  = 30,
        .title  = fTitles[9],
        .desc   = fDescriptions[9],
    },
    {
        .image  = CI_PICKLES_WSG,
        .type   = CI_FOOD,
        .loc    = CI_JUNGLE,
        .size   = CI_SMALL,
        .weight = 5,
        .value  = 30,
        .title  = fTitles[10],
        .desc   = fDescriptions[10],
    },
    {
        .image  = CI_PROTEIN_POWDER_WSG,
        .type   = CI_FOOD,
        .loc    = CI_M_FOREST,
        .size   = CI_MED,
        .weight = 6,
        .value  = 10,
        .title  = fTitles[11],
        .desc   = fDescriptions[11],
    },
    {
        .image  = CI_PUDDING_WSG,
        .type   = CI_FOOD,
        .loc    = CI_M_FOREST,
        .size   = CI_SMALL,
        .weight = 3,
        .value  = 60,
        .title  = fTitles[12],
        .desc   = fDescriptions[12],
    },
    {
        .image  = CI_ROAST_TURKEY_WSG,
        .type   = CI_FOOD,
        .loc    = CI_MOUNTAIN,
        .size   = CI_MED,
        .weight = 10,
        .value  = 40,
        .title  = fTitles[13],
        .desc   = fDescriptions[13],
    },
    {
        .image  = CI_SQUEEZY_PEANUT_BUTTER_WSG,
        .type   = CI_FOOD,
        .loc    = CI_JUNGLE,
        .size   = CI_SMALL,
        .weight = 1,
        .value  = 5,
        .title  = fTitles[14],
        .desc   = fDescriptions[14],
    },
    {
        .image  = CI_STRING_CHEESE_WSG,
        .type   = CI_FOOD,
        .loc    = CI_SWAMP,
        .size   = CI_SMALL,
        .weight = 1,
        .value  = 20,
        .title  = fTitles[15],
        .desc   = fDescriptions[15],
    },
    {
        .image  = CI_TASTEFUL_NOODZ_WSG,
        .type   = CI_FOOD,
        .loc    = CI_SWAMP,
        .size   = CI_MED,
        .weight = 3,
        .value  = 35,
        .title  = fTitles[16],
        .desc   = fDescriptions[16],
    },
};

const char* const fdTitles[] = {
    "Bamboo",    "Beehive",      "Birch Bark", "Coal",          "Crystal", "Dried Grasses",
    "Iron Ore",  "Large Leaves", "Latex",      "Logs",          "Resin",   "Rocks",
    "Rock Salt", "Spiderweb",    "Sticks",     "Uncured Hides", "Vines",   "Tar",
};

const char* const fdDescriptions[] = {
    "All fall before the might of the bamboo pole",
    "Did you know, according to all known laws of physics-",
    "A thin bark that's got a variety of uses",
    "Maybe if you're bad, a secular icon will sneak into your house and give you more",
    "If only it wasn't so rough it would be extremely pretty",
    "Extremely flammable, but very useful",
    "Raw iron clawed straight from the earth",
    "Any bigger and you could use it a parachute",
    "Latex comes from a rubber tree. It's not the weirdest thing to find out here",
    "Make sure to send this along with your bug report",
    "Everything is now sticky. Fingers. Clothes. The tree. Your pockets.",
    "Sure, there's lots of rocks around but these ones are special!",
    "If you heat this up, exactly nothing magical will happen",
    "Sorry I'm not at home right now. Leave a message and I'll call you back",
    "If you break your bones, getting out of here is going to suck",
    "This was just lying there on the floor... wait, isn't this pleather?",
    "Make sure to yell while you swing",
    "Try not to sink",
};

const ci_item_t ci_foragedData[] = {
    {
        .image  = CI_BAMBOO_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_JUNGLE,
        .size   = CI_MED,
        .weight = 3,
        .value  = 1,
        .title  = fdTitles[0],
        .desc   = fdDescriptions[0],
    },
    {
        .image  = CI_HONEY_COMB_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_FOREST,
        .size   = CI_MED,
        .weight = 5,
        .value  = 0,
        .title  = fdTitles[1],
        .desc   = fdDescriptions[1],
    },
    {
        .image  = CI_BIRCH_BARK_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_SWAMP,
        .size   = CI_SMALL,
        .weight = 2,
        .value  = 10,
        .title  = fdTitles[2],
        .desc   = fdDescriptions[2],
    },
    {
        .image  = CI_COAL_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_MOUNTAIN,
        .size   = CI_MED,
        .weight = 8,
        .value  = 50,
        .title  = fdTitles[3],
        .desc   = fdDescriptions[3],
    },
    {
        .image  = CI_CRYSTAL_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_M_FOREST,
        .size   = CI_SMALL,
        .weight = 3,
        .value  = 255,
        .title  = fdTitles[4],
        .desc   = fdDescriptions[4],
    },
    {
        .image  = CI_DRIED_GRASS_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_FOREST,
        .size   = CI_SMALL,
        .weight = 1,
        .value  = 2,
        .title  = fdTitles[5],
        .desc   = fdDescriptions[5],
    },
    {
        .image  = CI_IRON_ORE_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_MOUNTAIN,
        .size   = CI_MED,
        .weight = 10,
        .value  = 0,
        .title  = fdTitles[6],
        .desc   = fdDescriptions[6],
    },
    {
        .image  = CI_LARGE_LEAF_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_JUNGLE,
        .size   = CI_MED,
        .weight = 5,
        .value  = 2,
        .title  = fdTitles[7],
        .desc   = fdDescriptions[7],
    },
    {
        .image  = CI_LATEX_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_FOREST,
        .size   = CI_SMALL,
        .weight = 2,
        .value  = 0,
        .title  = fdTitles[8],
        .desc   = fdDescriptions[8],
    },
    {
        .image  = CI_LOG_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_FOREST,
        .size   = CI_LARGE,
        .weight = 20,
        .value  = 50,
        .title  = fdTitles[9],
        .desc   = fdDescriptions[9],
    },
    {
        .image  = CI_RESIN_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_JUNGLE,
        .size   = CI_SMALL,
        .weight = 2,
        .value  = 0,
        .title  = fdTitles[10],
        .desc   = fdDescriptions[10],
    },
    {
        .image  = CI_ROCKS_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_ALL,
        .size   = CI_MED,
        .weight = 8,
        .value  = 0,
        .title  = fdTitles[11],
        .desc   = fdDescriptions[11],
    },
    {
        .image  = CI_ROCK_SALT_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_MOUNTAIN,
        .size   = CI_MED,
        .weight = 4,
        .value  = 0,
        .title  = fdTitles[12],
        .desc   = fdDescriptions[12],
    },
    {
        .image  = CI_SPIDER_WEB_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_SWAMP,
        .size   = CI_SMALL,
        .weight = 1,
        .value  = 0,
        .title  = fdTitles[13],
        .desc   = fdDescriptions[13],
    },
    {
        .image  = CI_STICK_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_ALL,
        .size   = CI_SMALL,
        .weight = 2,
        .value  = 5,
        .title  = fdTitles[14],
        .desc   = fdDescriptions[14],
    },
    {
        .image  = CI_UNCURED_HIDE_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_SWAMP,
        .size   = CI_LARGE,
        .weight = 7,
        .value  = 0,
        .title  = fdTitles[15],
        .desc   = fdDescriptions[15],
    },
    {
        .image  = CI_VINE_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_JUNGLE,
        .size   = CI_MED,
        .weight = 5,
        .value  = 0,
        .title  = fdTitles[16],
        .desc   = fdDescriptions[16],
    },
    {
        .image  = CI_TAR_WSG,
        .type   = CI_FORAGED,
        .loc    = CI_SWAMP,
        .size   = CI_MED,
        .weight = 5,
        .value  = 20,
        .title  = fdTitles[17],
        .desc   = fdDescriptions[17],
    },
};

const char* const hTitles[] = {
    "Healing Powder", "Bandages", "Healing Poultice", "Potion", "Healing Heart",
};

const char* const hDescriptions[] = {
    "Usage instructions: Inhale deeply and ignore the pretty lights",
    "If you get enough of these, you can pretend to be a mummy",
    "Looking at the ingredient list, it's a surprise this heals you up",
    "It's not... glowing, is it?",
    "Legend says these could be ripped out of monsters once upon a time",
};

const ci_craftedItem_t ci_healingData[] = {
    {
        .image = CI_HEALING_POWDER_WSG,
        .type  = CI_HEALING,
        .title = fdTitles[0],
        .desc  = fdDescriptions[0],
    },
    {
        .image = CI_BANDAGES_WSG,
        .type  = CI_HEALING,
        .title = fdTitles[1],
        .desc  = fdDescriptions[1],
    },
    {
        .image = CI_POULTICE_WSG,
        .type  = CI_HEALING,
        .title = fdTitles[2],
        .desc  = fdDescriptions[2],
    },
    {
        .image = CI_HEALING_POTION_WSG,
        .type  = CI_HEALING,
        .title = fdTitles[3],
        .desc  = fdDescriptions[3],
    },
    {
        .image = CI_HEART_WSG,
        .type  = CI_HEALING,
        .title = fdTitles[4],
        .desc  = fdDescriptions[4],
    },
};

const ci_itemDataList ci_itemList = {
    .badFood = ci_badFoodData,
    .crafted = ci_craftedData,
    .food    = ci_foodData,
    .foraged = ci_foragedData,
    .healing = ci_healingData,
};