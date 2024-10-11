#ifndef _PA_TABLES_H_
#define _PA_TABLES_H_

//==============================================================================
// Includes
//==============================================================================
#include <stdlib.h>
#include "pango_typedef.h"

//==============================================================================
// Look Up Tables
//==============================================================================

#define DEFAULT_ENEMY_SPAWN_LOCATION_TABLE_LENGTH 20

#define DEFAULT_ENEMY_SPAWN_LOCATION_TX_LOOKUP_OFFSET 0
#define DEFAULT_ENEMY_SPAWN_LOCATION_TY_LOOKUP_OFFSET 1
#define DEFAULT_ENEMY_SPAWN_LOCATION_ROW_LENGTH       2

static const uint8_t
    defaultEnemySpawnLocations[DEFAULT_ENEMY_SPAWN_LOCATION_TABLE_LENGTH * DEFAULT_ENEMY_SPAWN_LOCATION_ROW_LENGTH]
    = {

        // tx,ty
        1,  1, 15, 1,  1,  13, 15, 13, 8, 1,  15, 7, 8, 13, 1,  7, 4, 1,  12, 1,
        15, 4, 15, 10, 12, 13, 4,  13, 1, 10, 1,  4, 6, 1,  10, 1, 6, 13, 10, 13};

#define SPAWN_BLOCK_COMBO_SCORE_TABLE_LENGTH 10

static const uint32_t spawnBlockComboScores[SPAWN_BLOCK_COMBO_SCORE_TABLE_LENGTH] = {
    50, 50, 2000, 2000, 2000, 5000, 5000, 5000, 5000, 10000
};

#define HIT_BLOCK_COMBO_SCORE_TABLE_LENGTH 10

static const uint32_t hitBlockComboScores[HIT_BLOCK_COMBO_SCORE_TABLE_LENGTH] = {
    100, 200, 400, 800, 1600, 3200, 3200, 6400, 6400, 6400
};

/*#define MASTER_DIFFICULTY2_TABLE_LENGTH 6

#define TOTAL_ENEMIES_LOOKUP_OFFSET 0
#define INITIAL_ACTIVE_ENEMIES_LOOKUP_OFFSET 1
#define MAX_ACTIVE_ENEMIES_LOOKUP_OFFSET 2
#define INITIAL_AGGRESSIVE_ENEMIES_LOOKUP_OFFET 3
#define MAX_AGGRESSIVE_ENEMIES_LOOKUP_OFFSET 4
#define INITIAL_ENEMY_SPEED 5
#define MAX_ENEMY_SPEED 6
#define MASTER_DIFFICULTY2_TABLE_ROW_LENGTH 7

static const int16_t masterDifficulty2[MASTER_DIFFICULTY2_TABLE_LENGTH * MASTER_DIFFICULTY2_TABLE_ROW_LENGTH] = {

//In-level difficulty curve:
//Starting at 15 seconds, the leftmost parameter will increment every 10 seconds until it reaches the max,
//Then the same will happen with the next parameter in the table until all reach the end-state for the current level.

// Total    initial max     initial     max         initial max
// enemies, active, active, aggressive, aggressive,  speed, speed
         5,      1,       2,         0,          1,     12,    12,
         5,      2,       3,         0,          1,     12,    12,
         6,      3,       3,         0,          2,     12,    14,
         6,      3,       3,         0,          3,     12,    14,
         7,      3,       3,         0,          3,     13,    15,
         7,      4,       4,         0,          1,      8,    10,
};*/

#define MASTER_DIFFICULTY_TABLE_LENGTH 16

#define BLOCK_WSG_LOOKUP_OFFSET            0
#define TOTAL_ENEMIES_LOOKUP_OFFSET        1
#define MAX_ACTIVE_ENEMIES_LOOKUP_OFFSET   2
#define ENEMY_INITIAL_SPEED_LOOKUP_OFFSET  3
#define MASTER_DIFFICULTY_TABLE_ROW_LENGTH 4

static const int16_t masterDifficulty[MASTER_DIFFICULTY_TABLE_LENGTH * MASTER_DIFFICULTY_TABLE_ROW_LENGTH] = {

    //          Block  Total    max            min    max
    //            wsg, enemies, active, speed, aggro, aggro,
    PA_WSG_BLOCK_BLUE,       5,       2,   12, 
    PA_WSG_BLOCK_RED, 5,       3,   12, 
    PA_WSG_BLOCK_YELLOW, 6,       3,   13, 
    PA_WSG_BLOCK_MAGENTA,         7,       4,   10,
    PA_WSG_BLOCK_BLUE,         8,       3,   13, 
    PA_WSG_BLOCK_BLUE,        8,       3,   14, 
    PA_WSG_BLOCK_BLUE,         8,       3,   15, 
    PA_WSG_BLOCK_BLUE,         7,       2,   16,
    PA_WSG_BLOCK_BLUE,         8,       3,   15, 
    PA_WSG_BLOCK_BLUE,         8,       3,   16, 
    PA_WSG_BLOCK_BLUE,         9,       3,   16, 
    PA_WSG_BLOCK_BLUE,         8,       4,   12, 
    PA_WSG_BLOCK_BLUE,         9,       3,   16, 
    PA_WSG_BLOCK_BLUE,         8,       3,   17, 
    PA_WSG_BLOCK_BLUE,        10,       4,   14, 
    PA_WSG_BLOCK_BLUE,        12,       1,   18,
};

static const paletteColor_t greenColors[4]  = {c555, c051, c030, c051};

#endif