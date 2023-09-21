#include <esp_heap_caps.h>
#include "ray_script.h"
#include "ray_dialog.h"

static void executeScriptEvent(ray_t* ray, rayScript_t* script, wsg_t* portrait);
static void checkScriptId(ray_t* ray, list_t* scriptList, int32_t id, wsg_t* portrait);
static void checkScriptCell(ray_t* ray, list_t* scriptList, int32_t x, int32_t y);
static void freeScript(rayScript_t* script);

/**
 * @brief Load scripts from a map file to RAM
 *
 * @param ray The entire game state
 * @param fileData The file bytes to read
 * @param fileSize The size of the file
 * @param caps Memory allocation strategy
 */
void loadScripts(ray_t* ray, const uint8_t* fileData, uint32_t fileSize, uint32_t caps)
{
    uint32_t fileIdx = 0;

    // Read the number of scripts
    uint32_t numScripts = fileData[fileIdx++];
    // For each script
    for (int sIdx = 0; sIdx < numScripts; sIdx++)
    {
        // Read the script length
        uint8_t scriptLen = (fileData[fileIdx] << 8) | (fileData[fileIdx + 1]);
        fileIdx += 2;

        // Allocate a script
        rayScript_t* newScript = heap_caps_calloc(1, sizeof(rayScript_t), caps);

        // Scripts start as active
        newScript->isActive = true;

        // Read the if operation
        newScript->ifOp = fileData[fileIdx++];

        // Read the the args depending on the if operation
        switch (newScript->ifOp)
        {
            case SHOOT_OBJS:
            case KILL:
            case GET:
            case TOUCH:
            {
                // AND_OR
                newScript->ifArgs.idList.andOr = fileData[fileIdx++];
                // [IDs]
                newScript->ifArgs.idList.numIds = fileData[fileIdx++];
                newScript->ifArgs.idList.ids = heap_caps_calloc(newScript->ifArgs.idList.numIds, sizeof(uint8_t), caps);
                newScript->ifArgs.idList.idsTriggered
                    = heap_caps_calloc(newScript->ifArgs.idList.numIds, sizeof(bool), caps);
                for (uint8_t i = 0; i < newScript->ifArgs.idList.numIds; i++)
                {
                    newScript->ifArgs.idList.ids[i] = fileData[fileIdx++];
                }
                // ORDER
                newScript->ifArgs.idList.order = fileData[fileIdx++];
                // ONE_TIME
                newScript->ifArgs.idList.oneTime = fileData[fileIdx++];
                break;
            }
            case SHOOT_WALLS:
            case ENTER:
            {
                // AND_OR
                newScript->ifArgs.cellList.andOr = fileData[fileIdx++];
                // [CELLs]
                newScript->ifArgs.cellList.numCells = fileData[fileIdx++];
                newScript->ifArgs.cellList.cells
                    = heap_caps_calloc(newScript->ifArgs.cellList.numCells, sizeof(rayMapCoordinates_t), caps);
                newScript->ifArgs.cellList.cellsTriggered
                    = heap_caps_calloc(newScript->ifArgs.cellList.numCells, sizeof(bool), caps);
                for (uint8_t i = 0; i < newScript->ifArgs.cellList.numCells; i++)
                {
                    newScript->ifArgs.cellList.cells[i].x = fileData[fileIdx++];
                    newScript->ifArgs.cellList.cells[i].y = fileData[fileIdx++];
                }
                // ORDER
                newScript->ifArgs.cellList.order = fileData[fileIdx++];
                // ONE_TIME
                newScript->ifArgs.cellList.oneTime = fileData[fileIdx++];
                break;
            }
            case TIME_ELAPSED:
            {
                // TIME
                newScript->ifArgs.time = (fileData[fileIdx + 0] << 24) | (fileData[fileIdx + 1] << 16)
                                         | (fileData[fileIdx + 2] << 8) | (fileData[fileIdx + 3] << 0);
                fileIdx += 4;
                break;
            }
        }

        // Read the then operation
        newScript->thenOp = fileData[fileIdx++];

        // Read the args depending on the then operation
        switch (newScript->thenOp)
        {
            case OPEN:
            case CLOSE:
            {
                // [CELLs]
                newScript->thenArgs.cellList.numCells = fileData[fileIdx++];
                newScript->thenArgs.cellList.cells
                    = heap_caps_calloc(newScript->thenArgs.cellList.numCells, sizeof(rayMapCoordinates_t), caps);
                for (uint8_t i = 0; i < newScript->thenArgs.cellList.numCells; i++)
                {
                    newScript->thenArgs.cellList.cells[i].x = fileData[fileIdx++];
                    newScript->thenArgs.cellList.cells[i].y = fileData[fileIdx++];
                }
                break;
            }
            case SPAWN:
            {
                // [SPAWNs]
                newScript->thenArgs.spawnList.numSpawns = fileData[fileIdx++];
                newScript->thenArgs.spawnList.spawns
                    = heap_caps_calloc(newScript->thenArgs.spawnList.numSpawns, sizeof(raySpawn_t), caps);
                for (uint8_t i = 0; i < newScript->thenArgs.spawnList.numSpawns; i++)
                {
                    newScript->thenArgs.spawnList.spawns[i].type  = fileData[fileIdx++];
                    newScript->thenArgs.spawnList.spawns[i].id    = fileData[fileIdx++];
                    newScript->thenArgs.spawnList.spawns[i].pos.x = fileData[fileIdx++];
                    newScript->thenArgs.spawnList.spawns[i].pos.y = fileData[fileIdx++];
                }
                break;
            }
            case DESPAWN:
            {
                // [IDs]
                newScript->thenArgs.idList.numIds = fileData[fileIdx++];
                newScript->thenArgs.idList.ids
                    = heap_caps_calloc(newScript->thenArgs.idList.numIds, sizeof(uint8_t), caps);
                for (uint8_t i = 0; i < newScript->thenArgs.idList.numIds; i++)
                {
                    newScript->thenArgs.idList.ids[i] = fileData[fileIdx++];
                }
                break;
            }
            case DIALOG:
            {
                // TEXT, get the length first
                uint16_t textLen = (fileData[fileIdx + 0] << 8) | (fileData[fileIdx + 1] << 0);
                fileIdx += 2;
                // Allocate space for and copy the string
                newScript->thenArgs.text = heap_caps_calloc(textLen + 1, sizeof(char), caps);
                memcpy(newScript->thenArgs.text, &fileData[fileIdx], textLen);
                fileIdx += textLen;
                break;
            }
            case WARP:
            {
                // MAP, CELL
                newScript->thenArgs.warpDest.mapId = fileData[fileIdx++];
                newScript->thenArgs.warpDest.pos.x = fileData[fileIdx++];
                newScript->thenArgs.warpDest.pos.y = fileData[fileIdx++];
                break;
            }
            case WIN:
            {
                // No args
                break;
            }
        }

        // Add script to the list
        push(&ray->scripts[newScript->ifOp], newScript);
    }
}

/**
 * @brief Free scripts from RAM
 *
 * @param ray The entire game state
 */
void freeScripts(ray_t* ray)
{
    for (uint16_t sIdx = 0; sIdx < NUM_IF_OP_TYPES; sIdx++)
    {
        // Pop all scripts from the list
        rayScript_t* script = NULL;
        while (NULL != (script = pop(&ray->scripts[sIdx])))
        {
            freeScript(script);
        }
    }
}

/**
 * @brief Free a single script from RAM
 *
 * @param script The script to free
 */
static void freeScript(rayScript_t* script)
{
    // Free if args
    switch (script->ifOp)
    {
        case SHOOT_OBJS:
        case KILL:
        case GET:
        case TOUCH:
        {
            free(script->ifArgs.idList.ids);
            free(script->ifArgs.idList.idsTriggered);
            break;
        }
        case SHOOT_WALLS:
        case ENTER:
        {
            free(script->ifArgs.cellList.cells);
            free(script->ifArgs.cellList.cellsTriggered);
            break;
        }
        case TIME_ELAPSED:
        {
            // Nothing allocated
            break;
        }
    }

    // Free then args
    switch (script->thenOp)
    {
        case OPEN:
        case CLOSE:
        {
            // [CELLs]
            free(script->thenArgs.cellList.cells);
            break;
        }
        case SPAWN:
        {
            // [SPAWNs]
            free(script->thenArgs.spawnList.spawns);
            break;
        }
        case DESPAWN:
        {
            // [IDs]
            free(script->thenArgs.idList.ids);
            break;
        }
        case DIALOG:
        {
            // TEXT, get the length first
            free(script->thenArgs.text);
            break;
        }
        case WARP:
        case WIN:
        {
            // Nothing allocated
            break;
        }
    }

    // Free the script
    free(script);
}

/**
 * @brief Check a script which is triggered by an ID
 *
 * @param ray The entire game state
 * @param scriptList The list of scripts to check
 * @param id The ID to check
 * @param portrait A portrait to draw on dialogs
 */
static void checkScriptId(ray_t* ray, list_t* scriptList, int32_t id, wsg_t* portrait)
{
    // Iterate over all nodes
    node_t* currentNode = scriptList->first;
    while (currentNode != NULL)
    {
        // Get the script
        rayScript_t* script = currentNode->val;

        // Only check if the script is active
        if (script->isActive)
        {
            // Don't execute it by default
            bool shouldExecute = false;

            // Check if this is an AND or OR script
            if (OR == script->ifArgs.idList.andOr)
            {
                // OR - check if any of the IDs are triggered
                for (int32_t idx = 0; idx < script->ifArgs.idList.numIds; idx++)
                {
                    // Check if any ID matches
                    if (id == script->ifArgs.idList.ids[idx])
                    {
                        // Do the then
                        shouldExecute = true;
                        break;
                    }
                }
            }
            else
            {
                // AND - check if all of the IDs are triggered
                for (int32_t idx = 0; idx < script->ifArgs.idList.numIds; idx++)
                {
                    // Check if the ID matches
                    if (id == script->ifArgs.idList.ids[idx])
                    {
                        // Mark it as triggered
                        script->ifArgs.idList.idsTriggered[idx] = true;
                        break;
                    }
                }

                // Make sure they were triggered in order
                if (IN_ORDER == script->ifArgs.idList.order)
                {
                    // Check if a 'true' is found after a 'false'
                    bool falseFound = false;
                    bool inOrder    = true;
                    for (int32_t idx = 0; idx < script->ifArgs.idList.numIds; idx++)
                    {
                        if (false == script->ifArgs.idList.idsTriggered[idx])
                        {
                            falseFound = true;
                        }
                        else if (falseFound) // idsTriggered[idx] is true
                        {
                            inOrder = false;
                            break;
                        }
                    }

                    // If the triggers are not in order
                    if (false == inOrder)
                    {
                        // Clear them all
                        memset(script->ifArgs.idList.idsTriggered, false, sizeof(bool) * script->ifArgs.idList.numIds);
                    }
                }

                // Check if all were triggered
                bool allTriggered = true;
                for (int32_t idx = 0; idx < script->ifArgs.idList.numIds; idx++)
                {
                    // Check if the ID matches
                    if (false == script->ifArgs.idList.idsTriggered[idx])
                    {
                        allTriggered = false;
                        break;
                    }
                }

                // If all IDs were triggered
                if (allTriggered)
                {
                    // Execute the script
                    shouldExecute = true;
                }
            }

            // If the script should execute
            if (shouldExecute)
            {
                // Do it
                executeScriptEvent(ray, script, portrait);

                // Mark it as inactive if this is a one time script
                script->isActive = (ALWAYS == script->ifArgs.idList.oneTime);

                // Reset the triggered IDs
                memset(script->ifArgs.idList.idsTriggered, false, sizeof(bool) * script->ifArgs.idList.numIds);
            }
        }

        // Move to the next script
        currentNode = currentNode->next;
    }
}

/**
 * @brief Check scripts when an object is shot
 *
 * @param ray The entire game state
 * @param id The ID of the shot object
 * @param portrait A portrait to draw on dialogs
 */
void checkScriptShootObjs(ray_t* ray, int32_t id, wsg_t* portrait)
{
    checkScriptId(ray, &ray->scripts[SHOOT_OBJS], id, portrait);
}

/**
 * @brief Check scripts when an enemy is killed
 * TODO call from somewhere
 *
 * @param ray The entire game state
 * @param id The ID of the dead enemy
 * @param portrait A portrait to draw on dialogs
 */
void checkScriptKill(ray_t* ray, int32_t id, wsg_t* portrait)
{
    checkScriptId(ray, &ray->scripts[KILL], id, portrait);
}

/**
 * @brief Check scripts when an item is obtained
 *
 * @param ray The entire game state
 * @param id The ID of the item obtained
 * @param portrait A portrait to draw on dialogs
 */
void checkScriptGet(ray_t* ray, int32_t id, wsg_t* portrait)
{
    checkScriptId(ray, &ray->scripts[GET], id, portrait);
}

/**
 * @brief Check scripts when an object is touched
 *
 * @param ray The entire game state
 * @param id The ID of the object touched
 * @param portrait A portrait to draw on dialogs
 */
void checkScriptTouch(ray_t* ray, int32_t id, wsg_t* portrait)
{
    checkScriptId(ray, &ray->scripts[TOUCH], id, portrait);
}

/**
 * @brief Check a script which is triggered by a map cell
 *
 * @param ray The entire game state
 * @param scriptList The list of scripts to check
 * @param x The X coordinate of the cell
 * @param y The Y coordinate of the cell
 */
static void checkScriptCell(ray_t* ray, list_t* scriptList, int32_t x, int32_t y)
{
    // Iterate over all nodes
    node_t* currentNode = scriptList->first;
    while (currentNode != NULL)
    {
        // Get the script
        rayScript_t* script = currentNode->val;

        // Only check if the script is active
        if (script->isActive)
        {
            // Don't execute it by default
            bool shouldExecute = false;

            // Check if this is an AND or OR script
            if (OR == script->ifArgs.cellList.andOr)
            {
                // OR - check if any of the cells are triggered
                for (int32_t idx = 0; idx < script->ifArgs.cellList.numCells; idx++)
                {
                    // Check if any cell matches
                    if ((x == script->ifArgs.cellList.cells[idx].x) && (y == script->ifArgs.cellList.cells[idx].y))
                    {
                        // Do the then
                        shouldExecute = true;
                        break;
                    }
                }
            }
            else
            {
                // AND - check if all of the cells are triggered
                for (int32_t idx = 0; idx < script->ifArgs.cellList.numCells; idx++)
                {
                    // Check if the cell matches
                    if ((x == script->ifArgs.cellList.cells[idx].x) && (y == script->ifArgs.cellList.cells[idx].y))
                    {
                        // Mark it as triggered
                        script->ifArgs.cellList.cellsTriggered[idx] = true;
                        break;
                    }
                }

                // Make sure they were triggered in order
                if (IN_ORDER == script->ifArgs.cellList.order)
                {
                    // Check if a 'true' is found after a 'false'
                    bool falseFound = false;
                    bool inOrder    = true;
                    for (int32_t idx = 0; idx < script->ifArgs.cellList.numCells; idx++)
                    {
                        if (false == script->ifArgs.cellList.cellsTriggered[idx])
                        {
                            falseFound = true;
                        }
                        else if (falseFound) // cellsTriggered[idx] is true
                        {
                            inOrder = false;
                            break;
                        }
                    }

                    // If the triggers are not in order
                    if (false == inOrder)
                    {
                        // Clear them all
                        memset(script->ifArgs.cellList.cellsTriggered, false,
                               sizeof(bool) * script->ifArgs.cellList.numCells);
                    }
                }

                // Check if all were triggered
                bool allTriggered = true;
                for (int32_t idx = 0; idx < script->ifArgs.cellList.numCells; idx++)
                {
                    // Check if the cell matches
                    if (false == script->ifArgs.cellList.cellsTriggered[idx])
                    {
                        allTriggered = false;
                        break;
                    }
                }

                // If all cells were triggered
                if (allTriggered)
                {
                    // Execute the script
                    shouldExecute = true;
                }
            }

            // If the script should execute
            if (shouldExecute)
            {
                // Do it
                executeScriptEvent(ray, script, &ray->portrait);

                // Mark it as inactive if this is a one time script
                script->isActive = (ALWAYS == script->ifArgs.cellList.oneTime);

                // Reset the triggered cells
                memset(script->ifArgs.cellList.cellsTriggered, false, sizeof(bool) * script->ifArgs.cellList.numCells);
            }
        }

        // Move to the next script
        currentNode = currentNode->next;
    }
}

/**
 * @brief Check scripts when a wall is shot
 *
 * @param ray The entire game state
 * @param x The X coordinate of the shot wall
 * @param y The Y coordinate of the shot wall
 */
void checkScriptShootWall(ray_t* ray, int32_t x, int32_t y)
{
    checkScriptCell(ray, &ray->scripts[SHOOT_WALLS], x, y);
}

/**
 * @brief Check scripts when a cell is entered
 *
 * @param ray The entire game state
 * @param x The X coordinate of the cell entered
 * @param y The Y coordinate of the cell entered
 */
void checkScriptEnter(ray_t* ray, int32_t x, int32_t y)
{
    checkScriptCell(ray, &ray->scripts[ENTER], x, y);
}

/**
 * @brief Check scripts when time elapses
 *
 * @param ray The entire game state
 * @param elapsedUs The time since this function was called last, in microseconds
 */
void checkScriptTime(ray_t* ray, uint32_t elapsedUs)
{
    // Keep track of elapsed microseconds
    ray->scriptTimer += elapsedUs;
    // When a whole second elapses
    if (ray->scriptTimer >= 1000000)
    {
        // Decrement the timer, increment the seconds count
        ray->scriptTimer -= 1000000;
        ray->secondsSinceStart++;

        // Check for executions. Iterate over all nodes
        node_t* currentNode = ray->scripts[TIME_ELAPSED].first;
        while (currentNode != NULL)
        {
            // Get the script
            rayScript_t* script = currentNode->val;

            // Only check if the script is active
            if (script->isActive)
            {
                // If the time elapsed
                if (script->ifArgs.time <= ray->secondsSinceStart)
                {
                    // Do it
                    executeScriptEvent(ray, script, &ray->portrait);

                    // Mark it as inactive
                    script->isActive = false;
                }
            }

            // Move to the next
            currentNode = currentNode->next;
        }
    }
}

/**
 * @brief Execute a script when it has been triggered
 *
 * @param ray The entire game state
 * @param script The script which should be executed
 * @param portrait A portrait to draw on dialogs
 */
static void executeScriptEvent(ray_t* ray, rayScript_t* script, wsg_t* portrait)
{
    switch (script->thenOp)
    {
        case OPEN:
        {
            // For each cell
            for (int cIdx = 0; cIdx < script->thenArgs.cellList.numCells; cIdx++)
            {
                int32_t x = script->thenArgs.cellList.cells[cIdx].x;
                int32_t y = script->thenArgs.cellList.cells[cIdx].y;
                // If the door isn't open
                if (0 == ray->map.tiles[x][y].doorOpen)
                {
                    // Start opening it
                    ray->map.tiles[x][y].doorOpen = 1;
                }
            }
            break;
        }
        case CLOSE:
        {
            for (int cIdx = 0; cIdx < script->thenArgs.cellList.numCells; cIdx++)
            {
                int32_t x = script->thenArgs.cellList.cells[cIdx].x;
                int32_t y = script->thenArgs.cellList.cells[cIdx].y;
                // If the door is open
                if (0 != ray->map.tiles[x][y].doorOpen)
                {
                    // TODO how to close doors smoothly??
                    ray->map.tiles[x][y].doorOpen = 0;
                }
            }
            break;
        }
        case SPAWN:
        {
            // TODO create objects
            break;
        }
        case DESPAWN:
        {
            // For each ID
            for (int32_t dIdx = 0; dIdx < script->thenArgs.idList.numIds; dIdx++)
            {
                // Get the ID
                int32_t id = script->thenArgs.idList.ids[dIdx];
                // Iterate over enemies, items, and scenery
                list_t* lists[] = {&ray->enemies, &ray->items, &ray->scenery};
                for (int32_t lIdx = 0; lIdx < ARRAY_SIZE(lists); lIdx++)
                {
                    // Iterate over all nodes
                    node_t* currentNode = lists[lIdx]->first;
                    while (currentNode != NULL)
                    {
                        // If the ID matches
                        if (id == ((rayObjCommon_t*)currentNode->val)->id)
                        {
                            // Save the node for removal
                            node_t* oldNode = currentNode;
                            // Iterate to the next
                            currentNode = currentNode->next;
                            // Remove the node that was iterated past
                            free(oldNode->val);
                            removeEntry(lists[lIdx], oldNode);
                        }
                        else
                        {
                            // Iterate to the next
                            currentNode = currentNode->next;
                        }
                    }
                }
            }
            break;
        }
        case DIALOG:
        {
            rayShowDialog(ray, script->thenArgs.text, portrait);
            break;
        }
        case WARP:
        {
            if (ray->mapId == script->thenArgs.warpDest.mapId)
            {
                // Warp within the map
                ray->posX = ADD_FX(TO_FX(script->thenArgs.warpDest.pos.x), TO_FX_FRAC(1, 2));
                ray->posY = ADD_FX(TO_FX(script->thenArgs.warpDest.pos.y), TO_FX_FRAC(1, 2));
            }
            else
            {
                // Warp to another map
                // TODO unload current map, load new one, move player
            }
            break;
        }
        case WIN:
        {
            // TODO unlock something or whatever.
            break;
        }
    }
}
