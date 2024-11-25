//==============================================================================
// Includes
//==============================================================================
#include "heap_bigbug.h"
#include "pathfinding_bigbug.h"

//==============================================================================
// Functions
//==============================================================================
void bb_addToHeap(bb_heap_t* heap, bb_heapItem_t* item)
{
    item->heapIdx              = heap->currentItemCount;
    heap->items[item->heapIdx] = *item;
    bb_sortUp(heap, item);
    heap->currentItemCount++;
}

// bb_heapItem_t* bb_removeFirstFromHeap(bb_heap_t* heap)
// {
//     bb_heapItem_t firstItem = heap->items[0];
//     heap->currentItemCount--;
//     heap->items[0].heapIdx = 0;
//     bb_sortDown(heap, &heap->items[0]);
//     // TODO you can't return a pointer to a local variable.
//     // It won't exist on the heap after returning!
//     return &firstItem;
// }

void bb_updateItem(bb_heap_t* heap, bb_heapItem_t* item)
{
    bb_sortUp(heap, item);
}

bool bb_heapContains(bb_heap_t* heap, const bb_heapItem_t* item)
{
    return heap->items[item->heapIdx].tile == item->tile;
}

// returns less than zero if A precedes B in the sort order
// returns zero if A and B are in the same position in the sort order
// returns greater than zero if A follows B in the sort order
int32_t bb_compareHeapItems(const bb_heapItem_t* itemA, const bb_heapItem_t* itemB)
{
    int32_t compare = (int32_t)fCost(itemA->tile) - (int32_t)fCost(itemB->tile);
    if (compare == 0)
    {
        compare = (int32_t)itemA->tile->hCost - (int32_t)itemB->tile->hCost;
    }
    // Not really sure if this should be negated.
    return -compare;
}

void bb_sortDown(bb_heap_t* heap, bb_heapItem_t* item)
{
    while (true)
    {
        uint16_t childIdxLeft  = item->heapIdx * 2 + 1;
        uint16_t childIdxRight = item->heapIdx * 2 + 2;
        uint16_t swapIdx       = 0;

        if (childIdxLeft < heap->currentItemCount)
        {
            swapIdx = childIdxLeft;

            if (childIdxRight < heap->currentItemCount)
            {
                if (bb_compareHeapItems(item, &heap->items[childIdxLeft]) < 0)
                {
                    swapIdx = childIdxRight;
                }
            }
        }

        if (bb_compareHeapItems(item, &heap->items[swapIdx]) < 0)
        {
            bb_swap(heap, item, &heap->items[swapIdx]);
        }
        else
        {
            return;
        }
    }
    // else{
    //     return;
    // }
}

void bb_sortUp(bb_heap_t* heap, bb_heapItem_t* item)
{
    uint16_t parentIdx = (item->heapIdx - 1) / 2;

    while (true)
    {
        bb_heapItem_t parentItem = heap->items[parentIdx];
        if (bb_compareHeapItems(item, &parentItem) > 0)
        {
            bb_swap(heap, item, &parentItem);
        }
        else
        {
            break;
        }

        parentIdx = (item->heapIdx - 1) / 2;
    }
}

void bb_swap(bb_heap_t* heap, bb_heapItem_t* itemA, bb_heapItem_t* itemB)
{
    // TODO validate this is the correct behavior

    // Swap items
    bb_heapItem_t tmp;
    memcpy(&tmp, itemA, sizeof(bb_heapItem_t));
    memcpy(itemA, itemB, sizeof(bb_heapItem_t));
    memcpy(itemB, &tmp, sizeof(bb_heapItem_t));

    // Swap indices
    uint16_t tmpIdx = itemA->heapIdx;
    itemA->heapIdx  = itemB->heapIdx;
    itemB->heapIdx  = tmpIdx;
}
