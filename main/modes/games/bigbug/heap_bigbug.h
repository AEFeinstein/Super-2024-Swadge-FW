#ifndef _HEAP_BIGBUG_H_
#define _HEAP_BIGBUG_H_

//==============================================================================
// Includes
//==============================================================================
#include "typedef_bigbug.h"
#include "tilemap_bigbug.h"

//==============================================================================
// Structs
//==============================================================================
typedef struct 
{
    bb_tileInfo_t*  tile;
    uint16_t heapIdx;
} bb_heapItem_t;

typedef struct
{
    bb_heapItem_t items[(TILE_FIELD_WIDTH-8) * (TILE_FIELD_HEIGHT-4) * 2];//Might get away with going even smaller,
                                                                          //but at least subtract 8 and 4 here because
                                                                          //the player will never move into the border
                                                                          //of the level.
    uint16_t currentItemCount;
} bb_heap_t;

//==============================================================================
// Prototypes
//==============================================================================
void bb_addToHeap(bb_heap_t* heap, bb_heapItem_t* item);
// bb_heapItem_t* bb_removeFirstFromHeap(bb_heap_t* heap);
void bb_updateItem(bb_heap_t* heap, bb_heapItem_t* item);
bool bb_heapContains(bb_heap_t* heap, const bb_heapItem_t* item);
int32_t bb_compareHeapItems(const bb_heapItem_t* itemA, const bb_heapItem_t* itemB);
void bb_sortDown(bb_heap_t* heap, bb_heapItem_t* item);
void bb_sortUp(bb_heap_t* heap, bb_heapItem_t* item);
void bb_swap(bb_heap_t* heap, bb_heapItem_t* itemA, bb_heapItem_t* itemB);

#endif