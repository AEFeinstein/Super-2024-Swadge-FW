//==============================================================================
// Includes
//==============================================================================

#include "ray_player.h"
#include "ray_shop_dialog.h"
#include "dialogBox.h"

//==============================================================================
// Defines
//==============================================================================

void rayShopDialogCb(const char* label);

static const char SHOP_TITLE[] = "Fredward's Shoppe";
static const char BUY[]        = "Buy";
static const char NO_THANKS[]  = "No Thanks";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 * @param ray
 * @param itemName
 * @param icon
 * @param cost
 * @param obj
 */
void rayShowShopDialog(ray_t* ray, wsg_t* icon, uint32_t cost, rayMapCellType_t obj)
{
    // Save shop parameters
    ray->shopCost = cost;
    ray->shopObj  = obj;

    // Translate item to name
    const char* itemName = "Item";
    if (OBJ_ITEM_BOMB == obj)
    {
        itemName = "Bombs";
    }

    // Build description string
    ray->shopDescription = heap_caps_calloc(128, sizeof(char), MALLOC_CAP_SPIRAM);
    snprintf(ray->shopDescription, 128, "Buy %s for %" PRIu32 " MPoints", itemName, cost);

    // Build the dialog
    ray->shopDialog = initDialogBox(SHOP_TITLE, ray->shopDescription, icon, rayShopDialogCb);

    // If the player can afford it, add the OK option
    if (ray->p.mpoints >= cost)
    {
        dialogBoxAddOption(ray->shopDialog, BUY, NULL, OPTHINT_OK);
    }
    // Always add the cancel option
    dialogBoxAddOption(ray->shopDialog, NO_THANKS, NULL, OPTHINT_CANCEL);

    // Lock button inputs for a moment
    ray->btnLockoutUs = RAY_BUTTON_LOCKOUT_US;

    // Show the dialog
    raySwitchToScreen(RAY_SHOP_DIALOG);
}

/**
 * @brief Check for button input while showing dialog and either show the next part of the dialog or return to the game
 * loop
 *
 * @param ray The entire game state
 */
void rayShopDialogCheckButtons(ray_t* ray)
{
    // Check the button queue
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (0 == ray->btnLockoutUs)
        {
            dialogBoxButton(ray->shopDialog, &evt);
        }
    }
}

/**
 * @brief Render the current dialog box
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayShopDialogRender(ray_t* ray, uint32_t elapsedUs)
{
    drawDialogBox(ray->shopDialog, &ray->ibm, &ray->ibm, DIALOG_CENTER, DIALOG_CENTER, TFT_WIDTH - 80, TFT_HEIGHT - 80,
                  8);
}

/**
 * @brief
 *
 * @param label
 */
void rayShopDialogCb(const char* label)
{
    ray_t* ray = getRayState();
    if (label == BUY)
    {
        if (ray->p.mpoints >= ray->shopCost)
        {
            ray->p.mpoints -= ray->shopCost;
            rayObjCommon_t item = {
                .type = ray->shopObj,
            };
            rayPlayerTouchItem(ray, &item, ray->p.mapId);
        }
    }
    else if (label == NO_THANKS)
    {
        // Do nothing, just exit
    }

    // Option was selected, cleanup and return to game
    deinitDialogBox(ray->shopDialog);
    heap_caps_free(ray->shopDescription);
    ray->shopDialog = NULL;
    raySwitchToScreen(RAY_GAME);
}
