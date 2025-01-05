#include "soko_undo.h"

void sokoInitHistory(soko_abs_t* soko)
{
    soko->historyCurrent    = 0;
    soko->historyBufferTail = 0;
    soko->history[0].moveID = 0;
    soko->historyNewMove    = true;
}

void sokoHistoryTurnOver(soko_abs_t* soko)
{
    soko->historyNewMove = true;
}

void sokoAddTileMoveToHistory(soko_abs_t* soko, uint16_t tileX, uint16_t tileY, sokoTile_t oldTileType)
{
    uint16_t moveID = soko->history[soko->historyCurrent].moveID;
    if (soko->historyNewMove)
    {
        moveID += 1;
        soko->historyNewMove = false;
    }
    // i think this should basically always be false for tiles.

    soko->historyCurrent++;
    if (soko->historyCurrent >= SOKO_UNDO_BUFFER_SIZE)
    {
        soko->historyCurrent = 0;
    }
    if (soko->historyCurrent == soko->historyBufferTail)
    {
        soko->historyBufferTail++;
        if (soko->historyBufferTail >= SOKO_UNDO_BUFFER_SIZE)
        {
            soko->historyBufferTail = 0;
        }
    }
    sokoUndoMove_t* move = &soko->history[soko->historyCurrent];

    move->moveID   = moveID;
    move->isEntity = false;
    move->tile     = oldTileType;
    move->x        = tileX;
    move->y        = tileY;
}

void sokoAddEntityMoveToHistory(soko_abs_t* soko, sokoEntity_t* entity, uint16_t oldX, uint16_t oldY,
                                sokoDirection_t oldFacing)
{
    uint16_t moveID = soko->history[soko->historyCurrent].moveID;
    // should basically only be true for the player...
    if (soko->historyNewMove)
    {
        moveID += 1;
        soko->historyNewMove = false;
        // ESP_LOGD(SOKO_TAG, "first invalid move (oldest) %i",soko->historyOldestValidMoveID);
    }

    soko->historyCurrent++;
    if (soko->historyCurrent >= SOKO_UNDO_BUFFER_SIZE)
    {
        soko->historyCurrent = 0;
    }
    if (soko->historyCurrent == soko->historyBufferTail)
    {
        soko->historyBufferTail++;
        if (soko->historyBufferTail >= SOKO_UNDO_BUFFER_SIZE)
        {
            soko->historyBufferTail = 0;
        }
    }

    sokoUndoMove_t* move = &soko->history[soko->historyCurrent];
    move->moveID         = moveID;
    move->isEntity       = true;
    move->entity         = entity;
    move->x              = oldX;
    move->y              = oldY;
    move->facing         = oldFacing;
}

void sokoUndo(soko_abs_t* soko)
{
    // HistoryCurrent points to the last added move.
    uint16_t undoMoveId = soko->history[soko->historyCurrent].moveID;

    // nope! can't undo! out of history.
    if (undoMoveId == soko->history[soko->historyBufferTail].moveID)
    {
        return;
    }

    while (soko->history[soko->historyCurrent].moveID == undoMoveId)
    {
        // history can partially overwrite the oldest move in the buffer.
        // we can fix that by uh... storing the last move we overwrote in a 'invalidUndo' and stopping undoes of it?
        sokoUndoMove_t* m = &soko->history[soko->historyCurrent];
        // undo this move.
        if (m->isEntity)
        {
            // undo the entity
            m->entity->x      = m->x;
            m->entity->y      = m->y;
            m->entity->facing = m->facing;
            // todo: facing
        }
        else
        {
            // undo the tile
            soko->currentLevel.tiles[m->x][m->y] = m->tile;
        }
        // ring buffer
        if (soko->historyCurrent > 0)
        {
            soko->historyCurrent--;
        }
        else
        {
            soko->historyCurrent = SOKO_UNDO_BUFFER_SIZE - 1;
        }
    }
    soko->undoCount++;
}
