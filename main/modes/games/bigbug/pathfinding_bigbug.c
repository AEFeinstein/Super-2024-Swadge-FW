//==============================================================================
// Includes
//==============================================================================
#include "pathfinding_bigbug.h"

//==============================================================================
// Functions
//==============================================================================
uint16_t fCost(const bb_midgroundTileInfo_t* tile)
{
    return tile->gCost + tile->hCost;
}

// Returns True if the node is one of the pieces near the edge of the level (where player can't traverse).
// functions as my target nodes all down the sides which offer grounded stability to tiles.
bool isPerimeterNode(const bb_midgroundTileInfo_t* tile)
{
    return tile->x == 4 || tile->x == TILE_FIELD_WIDTH - 5 || tile->y == TILE_FIELD_HEIGHT - 5;
}

void getNeighbors(const bb_midgroundTileInfo_t* tile, list_t* neighbors, bb_tilemap_t* tilemap, const bool toggleIteration)
{
    //left neighbor
    uint16_t neighborX = tile->x - 1;
    uint16_t neighborY = tile->y;
    bool neighborZ = tile->z;
    
    //adds orthogonal neighbors
    for(uint8_t i = 0; i < 5; i++)
    {
        if(neighborX < TILE_FIELD_WIDTH && neighborY < TILE_FIELD_HEIGHT)
        {
            bb_midgroundTileInfo_t* neighbor = neighborZ ? &tilemap->fgTiles[neighborX][neighborY] : &tilemap->mgTiles[neighborX][neighborY];
            if(neighbor->toggleIteration != toggleIteration)
            {
                neighbor->gCost = 0;
                neighbor->hCost = 0;
                //neighbor->parent = NULL;
                neighbor->toggleIteration = toggleIteration;
            }
            push(neighbors, (void*)neighbor);
        }

        //moves checkers to another orthogonal neighbor.
        switch(i)
        {
            //(left neighbor) was already handled in first iteration
            case 0://right neighbor
            {
                neighborX = tile->x + 1;
                break;
            }
            case 1://up neighbor
            {
                neighborX = tile->x;
                neighborY = tile->y - 1;
                break;
            }
            case 2://down neighbor
            {
                neighborY = tile->y + 1;
                break;
            }
            case 3://z neighbor
            {
                neighborY = tile->y;
                neighborZ = !tile->z;
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

bool contains(const list_t* nodeList, const bb_midgroundTileInfo_t* tile)
{
    node_t* cur = nodeList->first;
    while(cur != NULL)
    {
        bb_midgroundTileInfo_t* curNode = (bb_midgroundTileInfo_t*)cur->val;
        if(tile->x == curNode->x && tile->y == curNode->y && tile->z == curNode->z)
        {
            return true;
        }
        cur = cur->next;
    }
    return false;
}

// Returns True if there is a way to the perimeter
// start[0]=x;start[1]=y;start[2]=z
bool pathfindToPerimeter(bb_midgroundTileInfo_t* start, bb_tilemap_t* tilemap, const bool toggleIteration)
{
    uint16_t halfFieldWidth = TILE_FIELD_WIDTH/2;
    start->gCost = 0;
    start->hCost = 0;
    start->toggleIteration = toggleIteration;
    //bb_tileInfo_t* start = HEAP_CAPS_MALLOC_DBG(sizeof(bb_tileInfo_t), MALLOC_CAP_SPIRAM);
    // It's like a memcopy
    //*start = *_start;
    
    // 1. initialize the open list
    list_t* open   = HEAP_CAPS_CALLOC_DBG(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
    // 2. initialize the closed list
    list_t* closed = HEAP_CAPS_CALLOC_DBG(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
    // put the starting node on the open list (you can leave its f at zero)
    push(open, (void*)start);

    // 3. while the open list is not empty
    // a) find the node with the least f on the open list, call it "current"
    
    while (open->first != NULL)
    {
        node_t* currentNode = open->first;
        node_t* openNode = open->first;
        while (openNode != NULL)
        {
            if(fCost((bb_midgroundTileInfo_t*)openNode->val) < fCost((bb_midgroundTileInfo_t*)currentNode->val) ||
              (fCost((bb_midgroundTileInfo_t*)openNode->val) < fCost((bb_midgroundTileInfo_t*)currentNode->val) &&
              ((bb_midgroundTileInfo_t*)openNode->val)->hCost < ((bb_midgroundTileInfo_t*)currentNode->val)->hCost))
            {
                currentNode = openNode;
            }
            openNode = openNode->next;
        }
        // b) remove current from open and add current to closed
        bb_midgroundTileInfo_t* current = removeEntry(open,currentNode);
        //printf("Is this true?: %d\n", current == (current->z ? &tilemap->fgTiles[current->x][current->y] : &tilemap->mgTiles[current->x][current->y]));
        push(closed, (void*)current);
        if(isPerimeterNode(current))
        {
            return true;
        }

        list_t* neighbors = HEAP_CAPS_CALLOC_DBG(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
        getNeighbors(current, neighbors, tilemap, toggleIteration);

        //foreach neighbor of the current node
        node_t* neighbor = neighbors->first;
        while (neighbor != NULL)
        {
            bb_midgroundTileInfo_t* neighborTile = (bb_midgroundTileInfo_t*)neighbor->val;
            //if neighbor is not traversable or if neighbor is in closed
            //node_t* temp = closed->first;
            // while (temp != NULL) {
            //     printf("Node at %p, next: %p\n", (void*)temp, (void*)temp->next);
            //     temp = temp->next;
            // }

            if((neighborTile->z ?
                tilemap->fgTiles[neighborTile->x][neighborTile->y].health :
                tilemap->mgTiles[neighborTile->x][neighborTile->y].health) == 0 ||
                contains(closed, neighborTile))
            {
                //skip to the next neighbor
                neighbor = neighbor->next;
                continue;
            }

            //if new path to neighbor is shorter or neighbor is not in open
            uint16_t newCostToNeighbor = current->gCost + 1;//simply use + 1 because each neighbor is an orthogonal step
            if(newCostToNeighbor < neighborTile->gCost || !contains(open, neighborTile))
            {
                //set fCost of neighbor (we don't set the fCost, we calculate the gCost and hCost)
                neighborTile->gCost = newCostToNeighbor;
                //manhattan distance from the target
                if(neighborTile->x < halfFieldWidth)
                {
                    neighborTile->hCost = neighborTile->x - 4;
                }
                else
                {
                    neighborTile->hCost = TILE_FIELD_WIDTH - 5 - neighborTile->x;
                    //set parent of neighbor to current
                    // neighborTile->parent = current;
                }
                //if neighbor is not in open
                if(!contains(open, neighborTile))
                {
                    //add neighbor to open
                    push(open, (void*)neighborTile);
                }
            }
            neighbor = neighbor->next;
        }
        clear(neighbors);
        FREE_DBG(neighbors);
    } // end (while loop)
    clear(open);
    FREE_DBG(open);
    clear(closed);
    FREE_DBG(closed);
    printf("path false\n");
    return false;
}