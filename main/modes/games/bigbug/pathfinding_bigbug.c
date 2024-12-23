//==============================================================================
// Includes
//==============================================================================

#include "mode_bigbug.h"
#include "pathfinding_bigbug.h"

//==============================================================================
// Functions
//==============================================================================
//static inline function to get bits 0-6 of pos
static inline uint8_t getX(const bb_midgroundTileInfo_t* tile)
{
    return tile->pos & 0x7F;
}

//static inline function to get bits 7-14 of pos
static inline uint8_t getY(const bb_midgroundTileInfo_t* tile)
{
    return (tile->pos >> 7) & 0xFF;
}

//static inline function to get bit 15 of pos
static inline bool getZ(const bb_midgroundTileInfo_t* tile)
{
    return (tile->pos >> 15) & 0x1;
}

static inline uint16_t fCost(const bb_midgroundTileInfo_t* tile)
{
    return tile->gCost + tile->hCost;
}

// Returns True if the node is one of the pieces near the edge of the level (where player can't traverse).
// functions as my target nodes all down the sides which offer grounded stability to tiles.
bool isPerimeterNode(const bb_midgroundTileInfo_t* tile)
{
    return getX(tile) == 4 || getX(tile) == TILE_FIELD_WIDTH - 5 || getY(tile) == TILE_FIELD_HEIGHT - 5;
}

void getNeighbors(const bb_midgroundTileInfo_t* tile, list_t* neighbors, bb_tilemap_t* tilemap)
{
    // left neighbor
    uint16_t neighborX = getX(tile) - 1;
    uint16_t neighborY = getY(tile);
    bool neighborZ     = getZ(tile);

    // adds orthogonal neighbors
    for (uint8_t i = 0; i < 5; i++)
    {
        if (neighborX < TILE_FIELD_WIDTH && neighborY < TILE_FIELD_HEIGHT)
        {
            bb_midgroundTileInfo_t* neighbor = neighborZ
                                                   ? (bb_midgroundTileInfo_t*)&tilemap->fgTiles[neighborX][neighborY]
                                                   : &tilemap->mgTiles[neighborX][neighborY];
            neighbor->gCost                  = 0;
            neighbor->hCost                  = 0;
            // neighbor->parent = NULL;
            push(neighbors, (void*)neighbor);
        }

        // moves checkers to another orthogonal neighbor.
        switch (i)
        {
            //(left neighbor) was already handled in first iteration
            case 0: // right neighbor
            {
                neighborX = getX(tile) + 1;
                break;
            }
            case 1: // up neighbor
            {
                neighborX = getX(tile);
                neighborY = getY(tile) - 1;
                break;
            }
            case 2: // down neighbor
            {
                neighborY = getY(tile) + 1;
                break;
            }
            case 3: // z neighbor
            {
                neighborY = getY(tile);
                neighborZ = !getZ(tile);
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
    while (cur != NULL)
    {
        bb_midgroundTileInfo_t* curNode = (bb_midgroundTileInfo_t*)cur->val;
        if (getX(tile) == getX(curNode) && getY(tile) == getY(curNode) && getZ(tile) == getZ(curNode))
        {
            return true;
        }
        cur = cur->next;
    }
    return false;
}

// Returns True if there is a way to the perimeter
// start[0]=x;start[1]=y;start[2]=z
bool pathfindToPerimeter(bb_midgroundTileInfo_t* start, bb_tilemap_t* tilemap)
{
    uint16_t halfFieldWidth = TILE_FIELD_WIDTH / 2;
    start->gCost            = 0;
    // manhattan distance from the target
    if (getX(start) < halfFieldWidth)
    {
        start->hCost = getX(start) - 4;
    }
    else
    {
        start->hCost = TILE_FIELD_WIDTH - 5 - getX(start);
    }
    // bb_tileInfo_t* start = heap_caps_malloc(sizeof(bb_tileInfo_t), MALLOC_CAP_SPIRAM);
    //  It's like a memcopy
    //*start = *_start;

    // 1. initialize the open list
    list_t open = {0};
    // 2. initialize the closed list
    list_t closed = {0};
    // put the starting node on the open list (you can leave its f at zero)
    push(&open, (void*)start);

    // 3. while the open list is not empty
    // a) find the node with the least f on the open list, call it "current"

    while (open.first != NULL)
    {
        node_t* currentNode = open.first;
        node_t* openNode    = open.first;
        while (openNode != NULL)
        {
            if (fCost((bb_midgroundTileInfo_t*)openNode->val) < fCost((bb_midgroundTileInfo_t*)currentNode->val)
                || (fCost((bb_midgroundTileInfo_t*)openNode->val) < fCost((bb_midgroundTileInfo_t*)currentNode->val)
                    && ((bb_midgroundTileInfo_t*)openNode->val)->hCost
                           < ((bb_midgroundTileInfo_t*)currentNode->val)->hCost))
            {
                currentNode = openNode;
            }
            openNode = openNode->next;
        }
        // b) remove current from open and add current to closed
        bb_midgroundTileInfo_t* current = removeEntry(&open, currentNode);
        // ESP_LOGD(BB_TAG,"Is this true?: %d\n", current == (current->z ? &tilemap->fgTiles[current->x][current->y] :
        // &tilemap->mgTiles[current->x][current->y]));
        push(&closed, (void*)current);
        if (isPerimeterNode(current))
        {
            clear(&open);
            clear(&closed);
            return true;
        }

        list_t* neighbors = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
        getNeighbors(current, neighbors, tilemap);

        // foreach neighbor of the current node
        node_t* neighbor = neighbors->first;
        while (neighbor != NULL)
        {
            bb_midgroundTileInfo_t* neighborTile = (bb_midgroundTileInfo_t*)neighbor->val;
            // if neighbor is not traversable or if neighbor is in closed
            // node_t* temp = closed.first;
            //  while (temp != NULL) {
            //      ESP_LOGD(BB_TAG,"Node at %p, next: %p\n", (void*)temp, (void*)temp->next);
            //      temp = temp->next;
            //  }

            if ((getZ(neighborTile) ? tilemap->fgTiles[getX(neighborTile)][getY(neighborTile)].health
                                 : tilemap->mgTiles[getX(neighborTile)][getY(neighborTile)].health)
                    == 0
                || contains(&closed, neighborTile))
            {
                // skip to the next neighbor
                neighbor = neighbor->next;
                continue;
            }

            // if new path to neighbor is shorter or neighbor is not in open
            uint16_t newCostToNeighbor
                = current->gCost + 1; // simply use + 1 because each neighbor is an orthogonal step
            if (newCostToNeighbor < neighborTile->gCost || !contains(&open, neighborTile))
            {
                // set fCost of neighbor (we don't set the fCost, we calculate the gCost and hCost)
                neighborTile->gCost = newCostToNeighbor;
                // manhattan distance from the target
                if (getX(neighborTile) < halfFieldWidth)
                {
                    neighborTile->hCost = getX(neighborTile) - 4;
                }
                else
                {
                    neighborTile->hCost = TILE_FIELD_WIDTH - 5 - getX(neighborTile);
                    // set parent of neighbor to current
                    //  neighborTile->parent = current;
                }
                // if neighbor is not in open
                if (!contains(&open, neighborTile))
                {
                    // add neighbor to open
                    push(&open, (void*)neighborTile);
                }
            }
            neighbor = neighbor->next;
        }
        clear(neighbors);
        heap_caps_free(neighbors);
    } // end (while loop)
    clear(&open);
    clear(&closed);
    ESP_LOGD(BB_TAG, "path false\n");
    return false;
}