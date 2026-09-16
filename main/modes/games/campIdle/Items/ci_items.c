//==============================================================================
// Includes
//==============================================================================

// Main
#include "ci_items.h"

// C
#include <inttypes.h>

// Swadge
#include "font.h"

// Camp
#include "ci_helpers.h"
#include "ci_nvs.h"

//==============================================================================
// Defines
//==============================================================================

// Drawing
#define ICON_BUFFER          4
#define ICON_MAX_SIZE        32
#define ICON_TEXT_Y          40
#define PANEL_CORNER_BUFFER  9
#define PANEL_TITLE_BUFFER   6
#define PANEL_DESC_BUFFER    20
#define PANEL_DESC_HEIGHT    72
#define PANEL_TEXT_OFFSET    12
#define PANEL_TEXT_Y_SPACING 4
#define PANEL_INFO_HEIGHT    124

//==============================================================================
// Consts
//==============================================================================

static const char* const panelText[] = {
    "Forest", "Swamp",  "Mountain", "Jungle",    "Magical Forest", "Location:",         "Crafted", "All",
    "Small",  "Medium", "Large",    "Category:", "Food",           "Crafting material", "Healing",
};

//==============================================================================
// Function Definitions
//==============================================================================

/**
 * @brief Copy data from packed to data struct
 *
 * @param inv Game Data
 * @param packed Packed version of the data
 */
static void invNVSToRAM(ciInventory_t* inv, ciInvQtysPacked_t* packed);

/**
 * @brief Copy data from data struct to packed
 *
 * @param inv Game Data
 * @param packed Packed version of the data
 */
static void invRAMToNVS(ciInventory_t* inv, ciInvQtysPacked_t* packed);

/**
 * @brief Loads inventory from NVS
 *
 * @param inv Game Data
 * @param packed Packed version of the data
 */
static void loadInvFromNVS(ciInventory_t* inv, ciInvQtysPacked_t* packed);

/**
 * @brief Saves inventory to NVS
 *
 * @param inv Game Data
 * @param packed Packed version of the data
 */
static void saveInvToNVS(ciInventory_t* inv);

//==============================================================================
// Functions
//==============================================================================

// Initialization

void ciInitInventory(ciInventory_t* inv)
{
    // Load static data
    inv->qtys                 = (int16_t*)heap_caps_calloc(ciGetItemCount(), sizeof(int16_t), MALLOC_CAP_8BIT);
    ciInvQtysPacked_t qtyPack = {0};
    loadInvFromNVS(inv, &qtyPack);
    inv->itemImages = heap_caps_calloc(ciGetItemCount(), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ciGetItemCount(); idx++)
    {
        loadWsg(ciItemData[idx].image, &inv->itemImages[idx], true);
    }
}

void ciFreeInventory(ciInventory_t* inv)
{
    saveInvToNVS(inv);
    for (int idx = 0; idx < ciGetItemCount(); idx++)
    {
        freeWsg(&inv->itemImages[idx]);
    }
    free(inv->itemImages);
    free(inv->qtys);
}

int32_t ciAddToInv(ciInventory_t* inv, ciItemIdx_t item, int qty)
{
    if (item != CI_NO_ITEM)
    {
        if (INT16_MAX < inv->qtys[item] + qty)
        {
            inv->qtys[item] = INT16_MAX;
            return inv->qtys[item] + qty;
        }
        inv->qtys[item] += qty;
    }
    return 0;
}

bool ciRemoveFromInv(ciInventory_t* inv, ciItemIdx_t item, int qty)
{
    if (item != CI_NO_ITEM)
    {
        if (inv->qtys[item] < qty)
        {
            return false;
        }
        inv->qtys[item] -= qty;
    }
    return true;
}

void ciDrawItemPanel(ciInventory_t* inv, font_t* largeFont, font_t* smallFont, ciItemIdx_t idx)
{
    // Draw shadowbox
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c111);
    // Draw Icon
    drawRectFilled(PANEL_CORNER_BUFFER + ICON_BUFFER, PANEL_CORNER_BUFFER + ICON_BUFFER,
                   PANEL_CORNER_BUFFER + ICON_BUFFER + ICON_MAX_SIZE, PANEL_CORNER_BUFFER + ICON_BUFFER + ICON_MAX_SIZE,
                   c222);
    drawWsgSimple(&inv->itemImages[idx],
                  PANEL_CORNER_BUFFER + ICON_BUFFER + (ICON_MAX_SIZE - inv->itemImages[idx].w) / 2,
                  PANEL_CORNER_BUFFER + ICON_BUFFER + (ICON_MAX_SIZE - inv->itemImages[idx].h) / 2);
    // Draw title
    // Either draw the title centered on the icons
    int16_t xStart = PANEL_CORNER_BUFFER + ICON_BUFFER * 2 + ICON_MAX_SIZE + PANEL_TITLE_BUFFER;
    int16_t yStart = PANEL_CORNER_BUFFER + ICON_BUFFER + (ICON_MAX_SIZE + ICON_BUFFER - largeFont->height) / 2;
    if (textWidth(largeFont, ciItemData[idx].title) > (TFT_WIDTH - (PANEL_CORNER_BUFFER + ICON_BUFFER) * 2))
    {
        yStart = PANEL_CORNER_BUFFER + ICON_BUFFER + (ICON_MAX_SIZE + ICON_BUFFER) / 2 - largeFont->height;
        drawTextWordWrap(largeFont, c555, ciItemData[idx].title, &xStart, &yStart, TFT_WIDTH - PANEL_CORNER_BUFFER,
                         TFT_HEIGHT);
    }
    else
    {
        drawText(largeFont, c555, ciItemData[idx].title, xStart, yStart);
    }
    // Draw description
    xStart = PANEL_CORNER_BUFFER;
    yStart = PANEL_CORNER_BUFFER + ICON_BUFFER * 2 + ICON_MAX_SIZE + PANEL_DESC_BUFFER;
    drawRectFilled(xStart, yStart, TFT_WIDTH / 2, yStart + PANEL_DESC_HEIGHT, c222);
    xStart += ICON_BUFFER;
    yStart += ICON_BUFFER;
    drawTextWordWrap(smallFont, c555, ciItemData[idx].desc, &xStart, &yStart, TFT_WIDTH / 2 - ICON_BUFFER, TFT_HEIGHT);
    // Draw info
    xStart = TFT_WIDTH / 2 + ICON_BUFFER;
    yStart = PANEL_CORNER_BUFFER + ICON_BUFFER * 2 + ICON_MAX_SIZE + PANEL_DESC_BUFFER;
    drawRectFilled(xStart, yStart, TFT_WIDTH - ICON_BUFFER, yStart + PANEL_INFO_HEIGHT, c222);
    // Qty
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Quantity: %" PRIu8, inv->qtys[idx]);
    drawText(smallFont, c544, buffer, TFT_WIDTH / 2 + ICON_BUFFER * 2, yStart + ICON_BUFFER);
    // Location
    drawText(smallFont, c454, panelText[5], TFT_WIDTH / 2 + ICON_BUFFER * 2,
             yStart + ICON_BUFFER + PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING);
    drawText(smallFont, c454, panelText[ciItemData[idx].loc], TFT_WIDTH / 2 + ICON_BUFFER * 2,
             yStart + ICON_BUFFER + 2 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING);
    // Size
    snprintf(buffer, sizeof(buffer) - 1, "Size: %s", panelText[8 + ciItemData[idx].size]);
    drawText(smallFont, c445, buffer, TFT_WIDTH / 2 + ICON_BUFFER * 2,
             yStart + ICON_BUFFER + 3 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING * 2);
    // Weights
    snprintf(buffer, sizeof(buffer) - 1, "Weight: %" PRId8, ciItemData[idx].weight);
    drawText(smallFont, c554, buffer, TFT_WIDTH / 2 + ICON_BUFFER * 2,
             yStart + ICON_BUFFER + 4 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING * 3);
    // Type
    drawText(smallFont, c545, panelText[11], TFT_WIDTH / 2 + ICON_BUFFER * 2,
             yStart + ICON_BUFFER + 5 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING * 4);
    switch (ciItemData[idx].type)
    {
        case CI_FOOD:
        case CI_BAD_FOOD:
        {
            drawText(smallFont, c545, panelText[12], TFT_WIDTH / 2 + ICON_BUFFER * 2,
                     yStart + ICON_BUFFER + 6 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING * 4);
            break;
        }
        case CI_CRAFTED:
        case CI_FORAGED:
        {
            drawText(smallFont, c545, panelText[13], TFT_WIDTH / 2 + ICON_BUFFER * 2,
                     yStart + ICON_BUFFER + 6 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING * 4);
            break;
        }
        case CI_HEALING:
        {
            drawText(smallFont, c545, panelText[14], TFT_WIDTH / 2 + ICON_BUFFER * 2,
                     yStart + ICON_BUFFER + 6 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING * 4);
            break;
        }
        default:
        {
            break;
        }
    }
    // Value
    switch (ciItemData[idx].type)
    {
        case CI_FOOD:
        case CI_BAD_FOOD:
        {
            snprintf(buffer, sizeof(buffer) - 1, "Food amt: %" PRId16, ciItemData[idx].value);
            break;
        }
        case CI_CRAFTED:
        case CI_FORAGED:
        {
            snprintf(buffer, sizeof(buffer) - 1, "Fuel amt: %" PRId16, ciItemData[idx].value);
            break;
        }
        case CI_HEALING:
        {
            snprintf(buffer, sizeof(buffer) - 1, "Healing value: %" PRId16, ciItemData[idx].value);
            break;
        }
        default:
        {
            break;
        }
    }
    drawText(smallFont, c455, buffer, TFT_WIDTH / 2 + ICON_BUFFER * 2,
             yStart + ICON_BUFFER + 7 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING * 5);
}

void ciDrawItemIcon(ciInventory_t* inv, font_t* smallFont, ciItemIdx_t idx, int xStart, int yStart, int qty,
                    bool selected, bool showQty)
{
    drawRectFilled(xStart, yStart, xStart + ICON_WIDTH, yStart + ICON_HEIGHT, (selected) ? c330 : c111);
    drawRectFilled(xStart + ICON_BUFFER, yStart + ICON_BUFFER, xStart + ICON_BUFFER + ICON_MAX_SIZE,
                   yStart + ICON_BUFFER + ICON_MAX_SIZE, c222);
    if (idx != CI_NO_ITEM)
    {
        drawWsgSimple(&inv->itemImages[idx], xStart + ICON_BUFFER + (ICON_MAX_SIZE - inv->itemImages[idx].w) / 2,
                      yStart + ICON_BUFFER + (ICON_MAX_SIZE - inv->itemImages[idx].h) / 2);
        if (showQty)
        {
            char buffer[12];
            snprintf(buffer, sizeof(buffer) - 1, "%" PRId8, qty);
            drawText(smallFont, c555, buffer, xStart + (ICON_WIDTH - textWidth(smallFont, buffer)) / 2,
                     yStart + ICON_TEXT_Y);
        }
        else
        {
            drawText(smallFont, c555, ciItemData[idx].abbr,
                     xStart + (ICON_WIDTH - textWidth(smallFont, ciItemData[idx].abbr)) / 2, yStart + ICON_TEXT_Y);
        }
    }
    drawRect(xStart, yStart, xStart + ICON_WIDTH, yStart + ICON_HEIGHT, c000);
}

//==============================================================================
// Static Function Definitions
//==============================================================================

static void invNVSToRAM(ciInventory_t* inv, ciInvQtysPacked_t* packed)
{
    inv->qtys[CI_FLOOR_PIZZA]           = packed->floorPizza;
    inv->qtys[CI_FURRY_MILK]            = packed->furryMilk;
    inv->qtys[CI_ICBINB]                = packed->notMargarine;
    inv->qtys[CI_MALORT]                = packed->malort;
    inv->qtys[CI_PILK]                  = packed->pilk;
    inv->qtys[CI_RAVER_SWEAT]           = packed->raverSweat;
    inv->qtys[CI_SQUIRREL_NUTS]         = packed->squirrelNuts;
    inv->qtys[CI_YPLA]                  = packed->ypla;
    inv->qtys[CI_CLOTH]                 = packed->cloth;
    inv->qtys[CI_CUT_ROCK]              = packed->cutBlocks;
    inv->qtys[CI_DIAMOND]               = packed->diamond;
    inv->qtys[CI_DIAMOND_POWDER]        = packed->diamondPowder;
    inv->qtys[CI_GEAR]                  = packed->gears;
    inv->qtys[CI_IRON]                  = packed->iron;
    inv->qtys[CI_IRON_NAIL]             = packed->ironNails;
    inv->qtys[CI_HIDE]                  = packed->pelts;
    inv->qtys[CI_POLISHED_ROCK]         = packed->polishedBlocks;
    inv->qtys[CI_POLISHED_CRYSTAL]      = packed->polishedCrystals;
    inv->qtys[CI_ROPE]                  = packed->rope;
    inv->qtys[CI_SALT]                  = packed->salt;
    inv->qtys[CI_STRING]                = packed->string;
    inv->qtys[CI_APPLE]                 = packed->apple;
    inv->qtys[CI_BEANS]                 = packed->beans;
    inv->qtys[CI_BERRIES]               = packed->berries;
    inv->qtys[CI_DONUT]                 = packed->donut;
    inv->qtys[CI_ENERGY_DRINK]          = packed->energyDrink;
    inv->qtys[CI_HONEY]                 = packed->honey;
    inv->qtys[CI_MRE]                   = packed->MRE;
    inv->qtys[CI_MUSHROOMS]             = packed->mushrooms;
    inv->qtys[CI_MYSTERY_MEAT]          = packed->mysteryMeat;
    inv->qtys[CI_PAN_PIZZA]             = packed->panPizza;
    inv->qtys[CI_PICKLES]               = packed->pickles;
    inv->qtys[CI_PROTEIN_POWDER]        = packed->proteinPowder;
    inv->qtys[CI_PUDDING]               = packed->pudding;
    inv->qtys[CI_ROAST_TURKEY]          = packed->roastTurkey;
    inv->qtys[CI_SQUEEZY_PEANUT_BUTTER] = packed->squeezyPB;
    inv->qtys[CI_STRING_CHEESE]         = packed->stringCheese;
    inv->qtys[CI_TASTEFUL_NOODZ]        = packed->noodz;
    inv->qtys[CI_BAMBOO]                = packed->bamboo;
    inv->qtys[CI_HONEY_COMB]            = packed->honeyComb;
    inv->qtys[CI_BIRCH_BARK]            = packed->birchBark;
    inv->qtys[CI_COAL]                  = packed->coal;
    inv->qtys[CI_COTTON]                = packed->cotton;
    inv->qtys[CI_CRYSTAL]               = packed->crystal;
    inv->qtys[CI_DRIED_GRASS]           = packed->driedGrass;
    inv->qtys[CI_IRON_ORE]              = packed->iron;
    inv->qtys[CI_LARGE_LEAF]            = packed->largeLeaf;
    inv->qtys[CI_LATEX]                 = packed->latex;
    inv->qtys[CI_LOG]                   = packed->log;
    inv->qtys[CI_RESIN]                 = packed->resin;
    inv->qtys[CI_ROCKS]                 = packed->rocks;
    inv->qtys[CI_ROCK_SALT]             = packed->rockSalt;
    inv->qtys[CI_SPIDER_WEB]            = packed->spiderWeb;
    inv->qtys[CI_STICK]                 = packed->stick;
    inv->qtys[CI_UNCURED_HIDE]          = packed->uncuredHide;
    inv->qtys[CI_VINE]                  = packed->vine;
    inv->qtys[CI_TAR]                   = packed->tar;
    inv->qtys[CI_HEALING_POWDER]        = packed->healingPowder;
    inv->qtys[CI_BANDAGES]              = packed->bandages;
    inv->qtys[CI_POULTICE]              = packed->poultice;
    inv->qtys[CI_HEALING_POTION]        = packed->healingPotion;
    inv->qtys[CI_HEART]                 = packed->heart;
}

static void invRAMToNVS(ciInventory_t* inv, ciInvQtysPacked_t* packed)
{
    packed->floorPizza       = inv->qtys[CI_FLOOR_PIZZA];
    packed->furryMilk        = inv->qtys[CI_FURRY_MILK];
    packed->notMargarine     = inv->qtys[CI_ICBINB];
    packed->malort           = inv->qtys[CI_MALORT];
    packed->pilk             = inv->qtys[CI_PILK];
    packed->raverSweat       = inv->qtys[CI_RAVER_SWEAT];
    packed->squirrelNuts     = inv->qtys[CI_SQUIRREL_NUTS];
    packed->ypla             = inv->qtys[CI_YPLA];
    packed->cloth            = inv->qtys[CI_CLOTH];
    packed->cutBlocks        = inv->qtys[CI_CUT_ROCK];
    packed->diamond          = inv->qtys[CI_DIAMOND];
    packed->diamondPowder    = inv->qtys[CI_DIAMOND_POWDER];
    packed->gears            = inv->qtys[CI_GEAR];
    packed->iron             = inv->qtys[CI_IRON];
    packed->ironNails        = inv->qtys[CI_IRON_NAIL];
    packed->pelts            = inv->qtys[CI_HIDE];
    packed->polishedBlocks   = inv->qtys[CI_POLISHED_ROCK];
    packed->polishedCrystals = inv->qtys[CI_POLISHED_CRYSTAL];
    packed->rope             = inv->qtys[CI_ROPE];
    packed->salt             = inv->qtys[CI_SALT];
    packed->string           = inv->qtys[CI_STRING];
    packed->apple            = inv->qtys[CI_APPLE];
    packed->beans            = inv->qtys[CI_BEANS];
    packed->berries          = inv->qtys[CI_BERRIES];
    packed->donut            = inv->qtys[CI_DONUT];
    packed->energyDrink      = inv->qtys[CI_ENERGY_DRINK];
    packed->honey            = inv->qtys[CI_HONEY];
    packed->MRE              = inv->qtys[CI_MRE];
    packed->mushrooms        = inv->qtys[CI_MUSHROOMS];
    packed->mysteryMeat      = inv->qtys[CI_MYSTERY_MEAT];
    packed->panPizza         = inv->qtys[CI_PAN_PIZZA];
    packed->pickles          = inv->qtys[CI_PICKLES];
    packed->proteinPowder    = inv->qtys[CI_PROTEIN_POWDER];
    packed->pudding          = inv->qtys[CI_PUDDING];
    packed->roastTurkey      = inv->qtys[CI_ROAST_TURKEY];
    packed->squeezyPB        = inv->qtys[CI_SQUEEZY_PEANUT_BUTTER];
    packed->stringCheese     = inv->qtys[CI_STRING_CHEESE];
    packed->noodz            = inv->qtys[CI_TASTEFUL_NOODZ];
    packed->bamboo           = inv->qtys[CI_BAMBOO];
    packed->honeyComb        = inv->qtys[CI_HONEY_COMB];
    packed->birchBark        = inv->qtys[CI_BIRCH_BARK];
    packed->coal             = inv->qtys[CI_COAL];
    packed->cotton           = inv->qtys[CI_COTTON];
    packed->crystal          = inv->qtys[CI_CRYSTAL];
    packed->driedGrass       = inv->qtys[CI_DRIED_GRASS];
    packed->iron             = inv->qtys[CI_IRON_ORE];
    packed->largeLeaf        = inv->qtys[CI_LARGE_LEAF];
    packed->latex            = inv->qtys[CI_LATEX];
    packed->log              = inv->qtys[CI_LOG];
    packed->resin            = inv->qtys[CI_RESIN];
    packed->rocks            = inv->qtys[CI_ROCKS];
    packed->rockSalt         = inv->qtys[CI_ROCK_SALT];
    packed->spiderWeb        = inv->qtys[CI_SPIDER_WEB];
    packed->stick            = inv->qtys[CI_STICK];
    packed->uncuredHide      = inv->qtys[CI_UNCURED_HIDE];
    packed->vine             = inv->qtys[CI_VINE];
    packed->tar              = inv->qtys[CI_TAR];
    packed->healingPowder    = inv->qtys[CI_HEALING_POWDER];
    packed->bandages         = inv->qtys[CI_BANDAGES];
    packed->poultice         = inv->qtys[CI_POULTICE];
    packed->healingPotion    = inv->qtys[CI_HEALING_POTION];
    packed->heart            = inv->qtys[CI_HEART];
}

static void loadInvFromNVS(ciInventory_t* inv, ciInvQtysPacked_t* packed)
{
    size_t blobSize = sizeof(ciInvQtysPacked_t);
    if (!readNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_INVENTORY], packed, &blobSize))
    {
        writeNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_INVENTORY], packed, blobSize);
    }
    else
    {
        // Initialize to array
        invNVSToRAM(inv, packed);
    }
}

static void saveInvToNVS(ciInventory_t* inv)
{
    ciInvQtysPacked_t qtyPack = {0};
    invRAMToNVS(inv, &qtyPack);
    size_t blobSize = sizeof(ciInvQtysPacked_t);
    writeNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_INVENTORY], &qtyPack, blobSize);
}