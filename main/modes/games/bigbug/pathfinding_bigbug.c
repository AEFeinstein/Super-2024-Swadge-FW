//==============================================================================
// Includes
//==============================================================================
#include "pathfinding_bigbug.h"

//==============================================================================
// Functions
//==============================================================================
uint16_t fCost(bb_node_t* node)
{
    return node->gCost + node->hCost;
}

// Returns True if there is a way to the perimeter
// start[0]=x;start[1]=y;start[2]=z
bool pathfindToPerimeter(bb_node_t* start)
{
    // 1. initialize the open list
    list_t* open = calloc(1, sizeof(list_t));
    // 2. initialize the closed list
    list_t* closed = calloc(1, sizeof(list_t));
    // put the starting node on the open list (you can leave its f at zero)
    push(open, (void*)start);

    // 3. while the open list is not empty
    while (open->first != NULL)
    {
        // a) find the node with the least f on the open list, call it "q"
        uint16_t least_f    = 0;
        uint8_t least_idx   = 0;
        node_t* currentNode = open->first;
        currentNode         = currentNode->next;
        uint8_t cur_idx     = 1;
        while (currentNode != NULL)
        {
            // if(fCost(&currentNode->next) < ){

            // }

            if (((uint16_t*)currentNode->val)[3] < least_f)
            {
                least_f   = ((uint16_t*)currentNode->val)[3];
                least_idx = cur_idx;
            }
            currentNode = currentNode->next;
        }
        // b) pop q off the open list

        // c) generate q's 5 successors and set their parents to q

        // d) for each successor
        // i) if successor is the goal, stop search

        // ii) else, compute both g and h for successor
        // successor.g = q.g + distance between successor and q
        // successor.h = distance from goal to successor
        //(This can be done using many
        // ways, we will discuss three heuristics-
        // Manhattan, Diagonal and Euclidean Heuristics)

        // successor.f = successor.g + successor.h
        // iii) if a node with the same position as
        // successor is in the OPEN list which has a
        // lower f than successor, skip this successor
        // iV) if a node with the same position as
        // successor  is in the CLOSED list which has
        // a lower f than successor, skip this successor
        // otherwise, add  the node to the open list
        // end (for loop)

        // e) push q on the closed list

    } // end (while loop)
    return false;
}