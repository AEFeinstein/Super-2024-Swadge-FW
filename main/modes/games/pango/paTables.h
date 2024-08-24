#ifndef _PA_TABLES_H_
#define _PA_TABLES_H_

//==============================================================================
// Includes
//==============================================================================
#include <stdlib.h>

//==============================================================================
// Look Up Tables
//==============================================================================

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

#define TOTAL_ENEMIES_LOOKUP_OFFSET 0
#define MAX_ACTIVE_ENEMIES_LOOKUP_OFFSET 1
#define ENEMY_INITIAL_SPEED_LOOKUP_OFFSET 2
#define MASTER_DIFFICULTY_TABLE_ROW_LENGTH 3

static const int16_t masterDifficulty[MASTER_DIFFICULTY_TABLE_LENGTH * MASTER_DIFFICULTY_TABLE_ROW_LENGTH] = {

//Notes:
//-at any given time, at least 1 enemy should be always aggressive

// Total    max          
// enemies, active, speed
         5,      2,    12,
         5,      3,    12,
         6,      3,    13,
         7,      4,    10,
         8,      3,    13,
         8,      3,    14,
         8,      3,    15,
         7,      2,    16,
         8,      3,    15,
         8,      3,    16,
         9,      3,    16,
         8,      4,    12,
         9,      3,    16,
         8,      3,    17,
        10,      4,    14,
        12,      1,    18,             
};

#endif