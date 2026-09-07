//==============================================================================
// Include
//==============================================================================

#include "ci_container.h"
#include "ci_items.h"

//==============================================================================
// Defines
//==============================================================================

#define MAX_PER_SLOT 255

// Drawing
#define EDGE_BUFFER 2

//==============================================================================
// Functions
//==============================================================================

void ciInitContainer(ciContainer_t* cont, int maxSlots, int weightLim, int sizeLim)
{
    cont->slotsLim  = maxSlots;
    cont->sizeLim   = sizeLim;
    cont->weightLim = weightLim;
    cont->items     = (ciContainerItem_t*)heap_caps_calloc(maxSlots, sizeof(ciContainerItem_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < maxSlots; idx++)
    {
        cont->items[idx].item = CI_NO_ITEM;
        cont->items[idx].qty  = 0;
    }
}

void ciFreeContainer(ciContainer_t* cont)
{
    free(cont->items);
}

int ciAddItemToContainer(ciContainer_t* cont, ciItemIdx_t item, int qty)
{
    // Check if there's already a slot with the same item
    for (int idx = 0; idx < cont->slotsLim; idx++)
    {
        if (cont->items[idx].item == item)
        {
            if (MAX_PER_SLOT == cont->items[idx].qty)
            {
                continue;
            }
            else if (MAX_PER_SLOT < cont->items[idx].qty + qty)
            {
                // Fills up
                qty -= MAX_PER_SLOT - cont->items[idx].qty;
                cont->items[idx].qty = MAX_PER_SLOT;
            }
            else
            {
                // Not full
                cont->items[idx].qty += qty;
                qty = 0;
            }
        }
    }
    if (qty > 0)
    {
        for (int idx = 0; idx < cont->slotsLim; idx++)
        {
            if (cont->items[idx].item == CI_NO_ITEM)
            {
                cont->items[idx].item = item;
                if (qty >= MAX_PER_SLOT)
                {
                    qty -= MAX_PER_SLOT;
                    cont->items[idx].qty = MAX_PER_SLOT;
                }
                else
                {
                    cont->items[idx].qty = qty;
                    qty                  = 0;
                    break;
                }
            }
        }
    }
    return qty;
}

int ciRemoveFromContainer(ciContainer_t* cont, ciItemIdx_t item, int qty)
{
    for (int idx = cont->slotsLim - 1; idx >= 0; idx--)
    {
        if (cont->items[idx].item == item && qty > 0)
        {
            if (qty > cont->items[idx].qty)
            {
                qty -= cont->items[idx].qty;
                cont->items[idx].item = CI_NO_ITEM;
                cont->items[idx].qty  = 0;
            }
            else
            {
                cont->items[idx].qty -= qty;
                qty = 0;
                break;
            }
        }
    }
    return qty;
}

int ciFindItemInContainer(ciContainer_t* cont, ciItemIdx_t item, int* slotLoc)
{
    int total = 0;
    for (int idx = 0; idx < cont->slotsLim; idx++)
    {
        if (cont->items[idx].item == item)
        {
            total += cont->items[idx].qty;
            *slotLoc = idx;
        }
    }
    return total;
}

bool ciTransferBetweenContainers(ciContainer_t* cont1, ciContainer_t* cont2, ciItemIdx_t item, int qty)
{
    int fromVal = 0;
    for (int idx = 0; idx < cont1->slotsLim; idx++)
    {
        if (cont1->items[idx].item == item)
        {
            fromVal += cont1->items[idx].item;
            if (fromVal >= qty)
            {
                break;
            }
        }
    }
    if (fromVal < qty)
    {
        return false;
    }
    ciRemoveFromContainer(cont1, item, qty);
    ciAddItemToContainer(cont2, item, qty);
    return true;
}

void ciDrawContainer(ciCampData_t* ccd, ciContainer_t* cont, int x, int y, int maxCols, int maxRows, font_t* fnt)
{
    int calcXSize = 2 * EDGE_BUFFER + maxCols * ICON_WIDTH;
    int calcYSize = 2 * EDGE_BUFFER + maxRows * ICON_HEIGHT;

    fillDisplayArea(x, y, x + calcXSize, y + calcYSize, c111);
    for (int idx = 0; idx < cont->slotsLim; idx++)
    {
        ciDrawItemIcon(ccd, cont->items[idx].item, x + EDGE_BUFFER + (idx % maxCols) * ICON_WIDTH,
                       y + EDGE_BUFFER + (idx / maxCols) * ICON_HEIGHT, cont->items[idx].qty, false, true);
    }
    drawRect(x, y, x + calcXSize, y + calcYSize, c000);
}