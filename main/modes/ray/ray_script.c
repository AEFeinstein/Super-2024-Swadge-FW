#include <esp_heap_caps.h>
#include "ray_script.h"

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
        push(&ray->scriptList, newScript);
    }
}

/**
 * @brief Free scripts from RAM
 *
 * @param ray The entire game state
 */
void freeScripts(ray_t* ray)
{
    // Pop all scripts from the list
    rayScript_t* script = NULL;
    while (NULL != (script = pop(&ray->scriptList)))
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
                break;
            }
            case SHOOT_WALLS:
            case ENTER:
            {
                free(script->ifArgs.cellList.cells);
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
}
