//==============================================================================
// Includes
//==============================================================================

#include "ci_items.h"
#include "macros.h"
#include "swadge.h"

//==============================================================================
// Defines
//==============================================================================

// Drawing
#define ICON_WIDTH           40
#define ICON_HEIGHT          54
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

static void invNVSToCCD(ciCampData_t* ccd, ciInvQtysPacked_t* packed);

static void invCCDToNVS(ciCampData_t* ccd, ciInvQtysPacked_t* packed);

static void loadInvFromNVS(ciCampData_t* ccd, ciInvQtysPacked_t* packed);

static void saveInvToNVS(ciCampData_t* ccd);

//==============================================================================
// Functions
//==============================================================================

// Initialization

void ciInitInventory(ciCampData_t* ccd)
{
    // Load static data
    ccd->qtys                 = (uint8_t*)heap_caps_calloc(ciGetArrayLength(), sizeof(uint8_t), MALLOC_CAP_8BIT);
    ciInvQtysPacked_t qtyPack = {0};
    loadInvFromNVS(ccd, &qtyPack);
    ccd->itemImages = heap_caps_calloc(ciGetArrayLength(), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ciGetArrayLength(); idx++)
    {
        loadWsg(ciItemData[idx].image, &ccd->itemImages[idx], true);
    }
}

void ciFreeInventory(ciCampData_t* ccd)
{
    saveInvToNVS(ccd);
    for (int idx = 0; idx < ciGetArrayLength(); idx++)
    {
        freeWsg(&ccd->itemImages[idx]);
    }
    free(ccd->itemImages);
    free(ccd->qtys);
}

void ciDrawItemPanel(ciCampData_t* ccd, int idx)
{
    // Draw shadowbox
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c111);
    // Draw Icon
    drawRectFilled(PANEL_CORNER_BUFFER + ICON_BUFFER, PANEL_CORNER_BUFFER + ICON_BUFFER,
                   PANEL_CORNER_BUFFER + ICON_BUFFER + ICON_MAX_SIZE, PANEL_CORNER_BUFFER + ICON_BUFFER + ICON_MAX_SIZE,
                   c222);
    drawWsgSimple(&ccd->itemImages[idx],
                  PANEL_CORNER_BUFFER + ICON_BUFFER + (ICON_MAX_SIZE - ccd->itemImages[idx].w) / 2,
                  PANEL_CORNER_BUFFER + ICON_BUFFER + (ICON_MAX_SIZE - ccd->itemImages[idx].h) / 2);
    // Draw title
    // Either draw the title centered on the icons
    int16_t xStart = PANEL_CORNER_BUFFER + ICON_BUFFER * 2 + ICON_MAX_SIZE + PANEL_TITLE_BUFFER;
    int16_t yStart = PANEL_CORNER_BUFFER + ICON_BUFFER + (ICON_MAX_SIZE + ICON_BUFFER - ccd->largeText.height) / 2;
    if (textWidth(&ccd->largeText, ciItemData[idx].title) > (TFT_WIDTH - (PANEL_CORNER_BUFFER + ICON_BUFFER) * 2))
    {
        yStart = PANEL_CORNER_BUFFER + ICON_BUFFER + (ICON_MAX_SIZE + ICON_BUFFER) / 2 - ccd->largeText.height;
        drawTextWordWrap(&ccd->largeText, c555, ciItemData[idx].title, &xStart, &yStart,
                         TFT_WIDTH - PANEL_CORNER_BUFFER, TFT_HEIGHT);
    }
    else
    {
        drawText(&ccd->largeText, c555, ciItemData[idx].title, xStart, yStart);
    }
    // Draw description
    xStart = PANEL_CORNER_BUFFER;
    yStart = PANEL_CORNER_BUFFER + ICON_BUFFER * 2 + ICON_MAX_SIZE + PANEL_DESC_BUFFER;
    drawRectFilled(xStart, yStart, TFT_WIDTH / 2, yStart + PANEL_DESC_HEIGHT, c222);
    xStart += ICON_BUFFER;
    yStart += ICON_BUFFER;
    drawTextWordWrap(&ccd->smallFont, c555, ciItemData[idx].desc, &xStart, &yStart, TFT_WIDTH / 2 - ICON_BUFFER,
                     TFT_HEIGHT);
    // Draw info
    xStart = TFT_WIDTH / 2 + ICON_BUFFER;
    yStart = PANEL_CORNER_BUFFER + ICON_BUFFER * 2 + ICON_MAX_SIZE + PANEL_DESC_BUFFER;
    drawRectFilled(xStart, yStart, TFT_WIDTH - ICON_BUFFER, yStart + PANEL_INFO_HEIGHT, c222);
    // Qty
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Quantity: %" PRIu8, ccd->qtys[idx]);
    drawText(&ccd->smallFont, c544, buffer, TFT_WIDTH / 2 + ICON_BUFFER * 2, yStart + ICON_BUFFER);
    // Location
    drawText(&ccd->smallFont, c454, panelText[5], TFT_WIDTH / 2 + ICON_BUFFER * 2,
             yStart + ICON_BUFFER + PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING);
    drawText(&ccd->smallFont, c454, panelText[ciItemData[idx].loc], TFT_WIDTH / 2 + ICON_BUFFER * 2,
             yStart + ICON_BUFFER + 2 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING);
    // Size
    snprintf(buffer, sizeof(buffer) - 1, "Size: %s", panelText[8 + ciItemData[idx].size]);
    drawText(&ccd->smallFont, c445, buffer, TFT_WIDTH / 2 + ICON_BUFFER * 2,
             yStart + ICON_BUFFER + 3 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING * 2);
    // Weights
    snprintf(buffer, sizeof(buffer) - 1, "Weight: %" PRId8, ciItemData[idx].weight);
    drawText(&ccd->smallFont, c554, buffer, TFT_WIDTH / 2 + ICON_BUFFER * 2,
             yStart + ICON_BUFFER + 4 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING * 3);
    // Type
    drawText(&ccd->smallFont, c545, panelText[11], TFT_WIDTH / 2 + ICON_BUFFER * 2,
             yStart + ICON_BUFFER + 5 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING * 4);
    switch (ciItemData[idx].type)
    {
        case CI_FOOD:
        case CI_BAD_FOOD:
        {
            drawText(&ccd->smallFont, c545, panelText[12], TFT_WIDTH / 2 + ICON_BUFFER * 2,
                     yStart + ICON_BUFFER + 6 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING * 4);
            break;
        }
        case CI_CRAFTED:
        case CI_FORAGED:
        {
            drawText(&ccd->smallFont, c545, panelText[13], TFT_WIDTH / 2 + ICON_BUFFER * 2,
                     yStart + ICON_BUFFER + 6 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING * 4);
            break;
        }
        case CI_HEALING:
        {
            drawText(&ccd->smallFont, c545, panelText[14], TFT_WIDTH / 2 + ICON_BUFFER * 2,
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
    drawText(&ccd->smallFont, c455, buffer, TFT_WIDTH / 2 + ICON_BUFFER * 2,
             yStart + ICON_BUFFER + 7 * PANEL_TEXT_OFFSET + PANEL_TEXT_Y_SPACING * 5);
}

void ciDrawItemIcon(ciCampData_t* ccd, int idx, int xStart, int yStart, bool selected)
{
    drawRectFilled(xStart, yStart, xStart + ICON_WIDTH, yStart + ICON_HEIGHT, (selected) ? c330 : c111);
    drawRectFilled(xStart + ICON_BUFFER, yStart + ICON_BUFFER, xStart + ICON_BUFFER + ICON_MAX_SIZE,
                   yStart + ICON_BUFFER + ICON_MAX_SIZE, c222);
    drawWsgSimple(&ccd->itemImages[idx], xStart + ICON_BUFFER + (ICON_MAX_SIZE - ccd->itemImages[idx].w) / 2,
                  yStart + ICON_BUFFER + (ICON_MAX_SIZE - ccd->itemImages[idx].h) / 2);
    char buffer[12];
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId8, ccd->qtys[idx]);
    drawText(&ccd->smallFont, c555, buffer, xStart + (ICON_WIDTH - textWidth(&ccd->smallFont, buffer)) / 2,
             yStart + ICON_TEXT_Y);
    drawRect(xStart, yStart, xStart + ICON_WIDTH, yStart + ICON_HEIGHT, c000);
}

//==============================================================================
// Static Function Definitions
//==============================================================================

static void invNVSToCCD(ciCampData_t* ccd, ciInvQtysPacked_t* packed)
{
    ccd->qtys[CI_FLOOR_PIZZA]           = packed->floorPizza;
    ccd->qtys[CI_FURRY_MILK]            = packed->furryMilk;
    ccd->qtys[CI_ICBINB]                = packed->notMargarine;
    ccd->qtys[CI_MALORT]                = packed->malort;
    ccd->qtys[CI_PILK]                  = packed->pilk;
    ccd->qtys[CI_RAVER_SWEAT]           = packed->raverSweat;
    ccd->qtys[CI_SQUIRREL_NUTS]         = packed->squirrelNuts;
    ccd->qtys[CI_YPLA]                  = packed->ypla;
    ccd->qtys[CI_CLOTH]                 = packed->cloth;
    ccd->qtys[CI_CUT_ROCK]              = packed->cutBlocks;
    ccd->qtys[CI_DIAMOND]               = packed->diamond;
    ccd->qtys[CI_DIAMOND_POWDER]        = packed->diamondPowder;
    ccd->qtys[CI_GEAR]                  = packed->gears;
    ccd->qtys[CI_IRON]                  = packed->iron;
    ccd->qtys[CI_HIDE]                  = packed->pelts;
    ccd->qtys[CI_POLISHED_ROCK]         = packed->polishedBlocks;
    ccd->qtys[CI_POLISHED_CRYSTAL]      = packed->polishedCrystals;
    ccd->qtys[CI_ROPE]                  = packed->rope;
    ccd->qtys[CI_SALT]                  = packed->salt;
    ccd->qtys[CI_STRING]                = packed->string;
    ccd->qtys[CI_APPLE]                 = packed->apple;
    ccd->qtys[CI_BEANS]                 = packed->beans;
    ccd->qtys[CI_BERRIES]               = packed->berries;
    ccd->qtys[CI_DONUT]                 = packed->donut;
    ccd->qtys[CI_ENERGY_DRINK]          = packed->energyDrink;
    ccd->qtys[CI_HONEY]                 = packed->honey;
    ccd->qtys[CI_MRE]                   = packed->MRE;
    ccd->qtys[CI_MUSHROOMS]             = packed->mushrooms;
    ccd->qtys[CI_MYSTERY_MEAT]          = packed->mysteryMeat;
    ccd->qtys[CI_PAN_PIZZA]             = packed->panPizza;
    ccd->qtys[CI_PICKLES]               = packed->pickles;
    ccd->qtys[CI_PROTEIN_POWDER]        = packed->proteinPowder;
    ccd->qtys[CI_PUDDING]               = packed->pudding;
    ccd->qtys[CI_ROAST_TURKEY]          = packed->roastTurkey;
    ccd->qtys[CI_SQUEEZY_PEANUT_BUTTER] = packed->squeezyPB;
    ccd->qtys[CI_STRING_CHEESE]         = packed->stringCheese;
    ccd->qtys[CI_TASTEFUL_NOODZ]        = packed->noodz;
    ccd->qtys[CI_BAMBOO]                = packed->bamboo;
    ccd->qtys[CI_HONEY_COMB]            = packed->honeyComb;
    ccd->qtys[CI_BIRCH_BARK]            = packed->birchBark;
    ccd->qtys[CI_COAL]                  = packed->coal;
    ccd->qtys[CI_CRYSTAL]               = packed->crystal;
    ccd->qtys[CI_DRIED_GRASS]           = packed->driedGrass;
    ccd->qtys[CI_IRON_ORE]              = packed->iron;
    ccd->qtys[CI_LARGE_LEAF]            = packed->largeLeaf;
    ccd->qtys[CI_LATEX]                 = packed->latex;
    ccd->qtys[CI_LOG]                   = packed->log;
    ccd->qtys[CI_RESIN]                 = packed->resin;
    ccd->qtys[CI_ROCKS]                 = packed->rocks;
    ccd->qtys[CI_ROCK_SALT]             = packed->rockSalt;
    ccd->qtys[CI_SPIDER_WEB]            = packed->spiderWeb;
    ccd->qtys[CI_STICK]                 = packed->stick;
    ccd->qtys[CI_UNCURED_HIDE]          = packed->uncuredHide;
    ccd->qtys[CI_VINE]                  = packed->vine;
    ccd->qtys[CI_TAR]                   = packed->tar;
    ccd->qtys[CI_HEALING_POWDER]        = packed->healingPowder;
    ccd->qtys[CI_BANDAGES]              = packed->bandages;
    ccd->qtys[CI_POULTICE]              = packed->poultice;
    ccd->qtys[CI_HEALING_POTION]        = packed->healingPotion;
    ccd->qtys[CI_HEART]                 = packed->heart;
}

static void invCCDToNVS(ciCampData_t* ccd, ciInvQtysPacked_t* packed)
{
    packed->floorPizza       = ccd->qtys[CI_FLOOR_PIZZA];
    packed->furryMilk        = ccd->qtys[CI_FURRY_MILK];
    packed->notMargarine     = ccd->qtys[CI_ICBINB];
    packed->malort           = ccd->qtys[CI_MALORT];
    packed->pilk             = ccd->qtys[CI_PILK];
    packed->raverSweat       = ccd->qtys[CI_RAVER_SWEAT];
    packed->squirrelNuts     = ccd->qtys[CI_SQUIRREL_NUTS];
    packed->ypla             = ccd->qtys[CI_YPLA];
    packed->cloth            = ccd->qtys[CI_CLOTH];
    packed->cutBlocks        = ccd->qtys[CI_CUT_ROCK];
    packed->diamond          = ccd->qtys[CI_DIAMOND];
    packed->diamondPowder    = ccd->qtys[CI_DIAMOND_POWDER];
    packed->gears            = ccd->qtys[CI_GEAR];
    packed->iron             = ccd->qtys[CI_IRON];
    packed->pelts            = ccd->qtys[CI_HIDE];
    packed->polishedBlocks   = ccd->qtys[CI_POLISHED_ROCK];
    packed->polishedCrystals = ccd->qtys[CI_POLISHED_CRYSTAL];
    packed->rope             = ccd->qtys[CI_ROPE];
    packed->salt             = ccd->qtys[CI_SALT];
    packed->string           = ccd->qtys[CI_STRING];
    packed->apple            = ccd->qtys[CI_APPLE];
    packed->beans            = ccd->qtys[CI_BEANS];
    packed->berries          = ccd->qtys[CI_BERRIES];
    packed->donut            = ccd->qtys[CI_DONUT];
    packed->energyDrink      = ccd->qtys[CI_ENERGY_DRINK];
    packed->honey            = ccd->qtys[CI_HONEY];
    packed->MRE              = ccd->qtys[CI_MRE];
    packed->mushrooms        = ccd->qtys[CI_MUSHROOMS];
    packed->mysteryMeat      = ccd->qtys[CI_MYSTERY_MEAT];
    packed->panPizza         = ccd->qtys[CI_PAN_PIZZA];
    packed->pickles          = ccd->qtys[CI_PICKLES];
    packed->proteinPowder    = ccd->qtys[CI_PROTEIN_POWDER];
    packed->pudding          = ccd->qtys[CI_PUDDING];
    packed->roastTurkey      = ccd->qtys[CI_ROAST_TURKEY];
    packed->squeezyPB        = ccd->qtys[CI_SQUEEZY_PEANUT_BUTTER];
    packed->stringCheese     = ccd->qtys[CI_STRING_CHEESE];
    packed->noodz            = ccd->qtys[CI_TASTEFUL_NOODZ];
    packed->bamboo           = ccd->qtys[CI_BAMBOO];
    packed->honeyComb        = ccd->qtys[CI_HONEY_COMB];
    packed->birchBark        = ccd->qtys[CI_BIRCH_BARK];
    packed->coal             = ccd->qtys[CI_COAL];
    packed->crystal          = ccd->qtys[CI_CRYSTAL];
    packed->driedGrass       = ccd->qtys[CI_DRIED_GRASS];
    packed->iron             = ccd->qtys[CI_IRON_ORE];
    packed->largeLeaf        = ccd->qtys[CI_LARGE_LEAF];
    packed->latex            = ccd->qtys[CI_LATEX];
    packed->log              = ccd->qtys[CI_LOG];
    packed->resin            = ccd->qtys[CI_RESIN];
    packed->rocks            = ccd->qtys[CI_ROCKS];
    packed->rockSalt         = ccd->qtys[CI_ROCK_SALT];
    packed->spiderWeb        = ccd->qtys[CI_SPIDER_WEB];
    packed->stick            = ccd->qtys[CI_STICK];
    packed->uncuredHide      = ccd->qtys[CI_UNCURED_HIDE];
    packed->vine             = ccd->qtys[CI_VINE];
    packed->tar              = ccd->qtys[CI_TAR];
    packed->healingPowder    = ccd->qtys[CI_HEALING_POWDER];
    packed->bandages         = ccd->qtys[CI_BANDAGES];
    packed->poultice         = ccd->qtys[CI_POULTICE];
    packed->healingPotion    = ccd->qtys[CI_HEALING_POTION];
    packed->heart            = ccd->qtys[CI_HEART];
}

static void loadInvFromNVS(ciCampData_t* ccd, ciInvQtysPacked_t* packed)
{
    size_t blobSize = sizeof(ciInvQtysPacked_t);
    if (!readNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_INVENTORY], packed, &blobSize))
    {
        writeNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_INVENTORY], packed, blobSize);
    }
    else
    {
        // Initialize to array
        invNVSToCCD(ccd, packed);
    }
}

static void saveInvToNVS(ciCampData_t* ccd)
{
    ciInvQtysPacked_t qtyPack = {0};
    invCCDToNVS(ccd, &qtyPack);
    size_t blobSize = sizeof(ciInvQtysPacked_t);
    writeNamespaceNvsBlob(ciNVSKeys[CI_NVS_NAMESPACE], ciNVSKeys[CI_NVS_INVENTORY], &qtyPack, blobSize);
}