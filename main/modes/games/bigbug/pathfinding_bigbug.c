//==============================================================================
// Includes
//==============================================================================
#include "pathfinding_bigbug.h"

//==============================================================================
// Functions
//==============================================================================
uint16_t fCost(const bb_tileInfo_t* tile)
{
    return tile->gCost + tile->hCost;
}

// Returns True if the node is one of the pieces near the edge of the level (where player can't traverse).
// functions as my target nodes all down the sides which offer grounded stability to tiles.
bool isPerimeterNode(const bb_tileInfo_t* tile)
{
    return tile->x == 4 || tile->x == TILE_FIELD_WIDTH - 5 || tile->y == TILE_FIELD_HEIGHT - 5;
}

void getNeighbors(const bb_tileInfo_t* tile, const bb_tileInfo_t* startNode, list_t* neighbors, bb_tilemap_t* tilemap, const bool toggleIteration)
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
            bb_tileInfo_t* neighbor = neighborZ ? &tilemap->fgTiles[neighborX][neighborY] : &tilemap->mgTiles[neighborX][neighborY];
            if(neighbor->toggleIteration != toggleIteration)
            {
                neighbor->gCost = 0;
                neighbor->hCost = 0;
                neighbor->parent = NULL;
                neighbor->toggleIteration = toggleIteration;
            }
            push(neighbors, (void*)neighbor);
        }

        //moves checkers to another orthogonal neighbor.
        switch(i)
        {
            //case 0 (left neighbor) was already handled in first iteration
            case 1://right neighbor
            {
                neighborX = tile->x + 1;
                break;
            }
            case 2://up neighbor
            {
                neighborX = tile->x;
                neighborY = tile->y - 1;
                break;
            }
            case 3://down neighbor
            {
                neighborY = tile->y + 1;
                break;
            }
            case 4://z neighbor
            {
                neighborY = tile->y;
                neighborZ = !tile->z;
                break;
            }
            default:
            {
                //this shouldn't happen
                break;
            }
        }
    }
}

bool contains(const list_t* nodeList, const bb_tileInfo_t* tile)
{
    node_t* cur = nodeList->first;
    while(cur != NULL)
    {
        printf("contains\n");
        bb_tileInfo_t* curNode = (bb_tileInfo_t*)cur->val;
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
bool pathfindToPerimeter(bb_tileInfo_t* start, bb_tilemap_t* tilemap, const bool toggleIteration)
{
    uint16_t halfFieldWidth = TILE_FIELD_WIDTH/2;
    start->gCost = 0;
    start->hCost = 0;
    start->toggleIteration = toggleIteration;
    //bb_tileInfo_t* start = HEAP_CAPS_MALLOC_DBG(sizeof(bb_tileInfo_t), MALLOC_CAP_SPIRAM);
    // It's like a memcopy
    //*start = *_start;
    
    // 1. initialize the open list
    list_t* open = HEAP_CAPS_CALLOC_DBG(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
    // 2. initialize the closed list
    list_t* closed = HEAP_CAPS_CALLOC_DBG(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
    // put the starting node on the open list (you can leave its f at zero)
    push(open, (void*)start);

    // 3. while the open list is not empty
    // a) find the node with the least f on the open list, call it "current"
    node_t* currentNode = open->first;
    while (currentNode != NULL)
    {
        node_t* otherNode = currentNode->next;
        while (otherNode != NULL)
        {
            printf("first loop\n");
            if(fCost((bb_tileInfo_t*)otherNode->val) < fCost((bb_tileInfo_t*)currentNode->val) ||
              (fCost((bb_tileInfo_t*)otherNode->val) == fCost((bb_tileInfo_t*)currentNode->val) &&
              ((bb_tileInfo_t*)otherNode->val)->hCost < ((bb_tileInfo_t*)currentNode->val)->hCost))
            {
                currentNode = otherNode;
            }
            otherNode = otherNode->next;
        }
        // b) remove current from open and add current to closed
        bb_tileInfo_t* current = removeEntry(open,currentNode);
        push(closed, (void*)current);
        if(isPerimeterNode(current))
        {
            printf("path true\n");
            return true;
        }

        list_t* neighbors = HEAP_CAPS_CALLOC_DBG(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
        getNeighbors(current, start, neighbors, tilemap, toggleIteration);

        //foreach neihbor of the current node
        node_t* neighbor = neighbors->first;
        while (neighbor != NULL)
        {
            printf("second loop\n");
            bb_tileInfo_t* neighborTile = (bb_tileInfo_t*)neighbor->val;
            //if neighbor is not traversable or if neighbor is in closed
            if((neighborTile->z && tilemap->fgTiles[neighborTile->x][neighborTile->y].health == 0) || (!neighborTile->z && tilemap->mgTiles[neighborTile->x][neighborTile->y].health == 0) ||
                contains(closed, neighborTile))
            {
                //skip to the next neighbor
                neighbor = neighbor->next;
                continue;
            }

            //if new path to neighbor is shorter or neighbor is not in open
            uint16_t newMovementCostToNeighbor = current->gCost + 1;//simply use + 1 because each neighbor is an orthogonal step
            if(newMovementCostToNeighbor < neighborTile->gCost || !contains(open, neighborTile))
            {
                //set fCost of neighbor (we don't set the fCost, we calculate the gCost and hCost)
                neighborTile->gCost = newMovementCostToNeighbor;
                //manhattan distance from the target
                if(neighborTile->x < halfFieldWidth)
                {
                    neighborTile->hCost = neighborTile->x - 4;
                }
                else
                {
                    neighborTile->hCost = TILE_FIELD_WIDTH - 5 - neighborTile->x;
                    //set parent of neighbor to current
                    neighborTile->parent = current;
                    
                    //if neighbor is not in open
                    if(!contains(open, neighborTile))
                    {
                        //add neighbor to open
                        push(open, (void*)neighborTile);
                    }
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