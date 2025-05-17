#pragma once

#include "ultimateTTT.h"

/**
 * @brief Given the game state, returns the next move as a value from 0 to 8.
 *
 * This value corresponds to either a sub-game, or the location to place the marker within a sub-game.
 *
 * @param game The game state
 * @return int The index of the sub-game or square within a sub-game
 */
void tttCpuNextMove(ultimateTTT_t* ttt);
