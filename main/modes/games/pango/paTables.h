#ifndef _PA_TABLES_H_
#define _PA_TABLES_H_

//==============================================================================
// Includes
//==============================================================================
#include <stdlib.h>
#include "pango_typedef.h"
#include "hdw-btn.h"

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
        1,  1, 15, 1,  1,  13, 15, 13, 8, 1,  15, 7, 8, 13, 1,  7, 4, 1,  12, 1, //
        15, 4, 15, 10, 12, 13, 4,  13, 1, 10, 1,  4, 6, 1,  10, 1, 6, 13, 10, 13};

#define SPAWN_BLOCK_COMBO_SCORE_TABLE_LENGTH 10

static const uint32_t spawnBlockComboScores[SPAWN_BLOCK_COMBO_SCORE_TABLE_LENGTH]
    = {100, 500, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000};

#define HIT_BLOCK_COMBO_SCORE_TABLE_LENGTH 10

static const uint32_t hitBlockComboScores[HIT_BLOCK_COMBO_SCORE_TABLE_LENGTH]
    = {200, 400, 800, 1600, 3200, 3200, 6400, 6400, 6400, 6400};

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
//Starting at 15 seconds, the leftmost parameter will increment every 10 seconds until it reaches the max, //
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

#define MASTER_DIFFICULTY_TABLE_LENGTH 100

#define BLOCK_WSG_LOOKUP_OFFSET                      0
#define TOTAL_ENEMIES_LOOKUP_OFFSET                  1
#define MAX_ACTIVE_ENEMIES_LOOKUP_OFFSET             2
#define ENEMY_INITIAL_SPEED_LOOKUP_OFFSET            3
#define ENEMY_MINIMUM_AGGRESSIVE_TIME_LOOKUP_OFFSET  4
#define ENEMY_MAXIMUM_AGGRESSIVE_TIME_LOOKUP_OFFSET  5
#define ENEMY_MINIMUM_AGGRESSIVE_COUNT_LOOKUP_OFFSET 6
#define ENEMY_MAXIMUM_AGGRESSIVE_COUNT_LOOKUP_OFFSET 7
#define MASTER_DIFFICULTY_TABLE_ROW_LENGTH           8

static const int16_t masterDifficulty[MASTER_DIFFICULTY_TABLE_LENGTH * MASTER_DIFFICULTY_TABLE_ROW_LENGTH] = {

    //          Block  Total    max            min         max         min          max
    //            wsg, enemies, active, speed, aggro time, aggro time, aggro count, aggro count
    PA_WSG_BLOCK_BLUE, 5, 2, 12, 300, 900, 0, 0, //
    PA_WSG_BLOCK_BLUE, 5, 2, 12, 300, 900, 1, 1, //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_BLUE, 6, 3, 12, 300, 900, 1, 1, //
    PA_WSG_BLOCK_BLUE, 7, 3, 13, 300, 900, 1, 1, //
    PA_WSG_BLOCK_BLUE, 7, 4, 10, 300, 900, 1, 1, //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_GREEN, 8, 3, 13, 300, 600, 1, 1,  //
    PA_WSG_BLOCK_GREEN, 9, 3, 13, 300, 600, 1, 1,  //
    PA_WSG_BLOCK_GREEN, 10, 4, 11, 300, 600, 1, 1, //
    PA_WSG_BLOCK_GREEN, 7, 2, 14, 200, 600, 1, 1,  //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_YELLOW, 10, 3, 13, 200, 600, 1, 1, //
    PA_WSG_BLOCK_YELLOW, 11, 3, 14, 200, 600, 1, 1, //
    PA_WSG_BLOCK_YELLOW, 12, 4, 12, 200, 600, 1, 1, //
    PA_WSG_BLOCK_YELLOW, 9, 2, 14, 100, 600, 1, 1,  //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_ORANGE, 12, 3, 14, 100, 600, 1, 1, //
    PA_WSG_BLOCK_ORANGE, 13, 4, 13, 100, 600, 1, 1, //
    PA_WSG_BLOCK_ORANGE, 10, 2, 15, 100, 600, 1, 1, //
    PA_WSG_BLOCK_ORANGE, 24, 8, 12, 100, 500, 1, 1, //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_RED, 13, 3, 15, 100, 500, 1, 1, //
    PA_WSG_BLOCK_RED, 14, 4, 14, 100, 500, 1, 1, //
    PA_WSG_BLOCK_RED, 11, 2, 16, 100, 400, 1, 1, //
    PA_WSG_BLOCK_RED, 28, 8, 13, 100, 500, 1, 1, //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_MAGENTA, 14, 3, 16, 100, 400, 1, 1,  //
    PA_WSG_BLOCK_MAGENTA, 15, 4, 15, 100, 400, 1, 1,  //
    PA_WSG_BLOCK_MAGENTA, 12, 2, 17, 100, 300, 1, 1,  //
    PA_WSG_BLOCK_MAGENTA, 30, 10, 13, 100, 400, 1, 1, // 25

    PA_WSG_BLOCK_BLUE, 12, 1, 18, 100, 300, 1, 1,   //
    PA_WSG_BLOCK_GREEN, 16, 3, 12, 300, 900, 1, 2,  //
    PA_WSG_BLOCK_YELLOW, 17, 3, 13, 300, 900, 1, 2, //
    PA_WSG_BLOCK_ORANGE, 17, 4, 10, 300, 900, 1, 2, //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_RED, 18, 3, 13, 300, 600, 1, 2,     //
    PA_WSG_BLOCK_MAGENTA, 19, 3, 13, 300, 600, 1, 2, //
    PA_WSG_BLOCK_BLUE, 20, 4, 11, 300, 600, 1, 2,    //
    PA_WSG_BLOCK_GREEN, 17, 2, 14, 200, 600, 1, 2,   //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_YELLOW, 20, 3, 13, 200, 600, 1, 2,  //
    PA_WSG_BLOCK_ORANGE, 21, 3, 14, 200, 600, 1, 2,  //
    PA_WSG_BLOCK_RED, 22, 4, 12, 200, 600, 1, 2,     //
    PA_WSG_BLOCK_MAGENTA, 19, 2, 14, 100, 600, 1, 2, //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_BLUE, 22, 3, 14, 100, 600, 1, 2,   //
    PA_WSG_BLOCK_GREEN, 23, 4, 13, 100, 600, 1, 2,  //
    PA_WSG_BLOCK_YELLOW, 20, 2, 15, 100, 600, 1, 2, //
    PA_WSG_BLOCK_ORANGE, 34, 8, 12, 100, 500, 1, 2, //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_RED, 23, 3, 15, 100, 500, 1, 2,     //
    PA_WSG_BLOCK_MAGENTA, 24, 4, 14, 100, 500, 1, 2, //
    PA_WSG_BLOCK_BLUE, 21, 2, 16, 100, 400, 1, 2,    //
    PA_WSG_BLOCK_GREEN, 38, 8, 13, 100, 500, 1, 2,   //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_YELLOW, 24, 3, 16, 100, 400, 1, 2,   //
    PA_WSG_BLOCK_ORANGE, 25, 4, 15, 100, 400, 1, 2,   //
    PA_WSG_BLOCK_RED, 22, 2, 17, 100, 300, 1, 2,      //
    PA_WSG_BLOCK_MAGENTA, 42, 10, 13, 100, 400, 1, 2, // 49
    
    // Placeholder: bonus stage
    PA_WSG_BLOCK_BLUE, 12, 1, 19, 100, 300, 1, 1,   //
    PA_WSG_BLOCK_BLUE, 16, 3, 13, 300, 900, 2, 2,  //
    PA_WSG_BLOCK_BLUE, 17, 3, 14, 300, 900, 2, 2, //
    PA_WSG_BLOCK_BLUE, 17, 4, 11, 300, 900, 2, 2, //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_GREEN, 18, 3, 14, 300, 600, 2, 2,     //
    PA_WSG_BLOCK_GREEN, 19, 3, 14, 300, 600, 2, 2, //
    PA_WSG_BLOCK_GREEN, 20, 4, 12, 300, 600, 2, 2,    //
    PA_WSG_BLOCK_GREEN, 17, 2, 15, 200, 600, 2, 2,   //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_YELLOW, 20, 3, 14, 200, 600, 2, 2,  //
    PA_WSG_BLOCK_YELLOW, 21, 3, 15, 200, 600, 2, 2,  //
    PA_WSG_BLOCK_YELLOW, 22, 4, 13, 200, 600, 2, 2,     //
    PA_WSG_BLOCK_YELLOW, 19, 2, 15, 100, 600, 2, 2, //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_ORANGE, 22, 3, 15, 100, 600, 2, 2,   //
    PA_WSG_BLOCK_ORANGE, 23, 4, 14, 100, 600, 2, 2,  //
    PA_WSG_BLOCK_ORANGE, 20, 2, 16, 100, 600, 2, 2, //
    PA_WSG_BLOCK_ORANGE, 34, 8, 13, 100, 500, 2, 2, //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_RED, 23, 3, 16, 100, 500, 2, 2,     //
    PA_WSG_BLOCK_RED, 24, 4, 15, 100, 500, 2, 2, //
    PA_WSG_BLOCK_RED, 21, 2, 17, 100, 400, 2, 2,    //
    PA_WSG_BLOCK_RED, 38, 8, 14, 100, 500, 2, 2,   //
    // Placeholder: bonus stage
    PA_WSG_BLOCK_MAGENTA, 24, 3, 17, 100, 400, 2, 2,   //
    PA_WSG_BLOCK_MAGENTA, 25, 4, 16, 100, 400, 2, 2,   //
    PA_WSG_BLOCK_MAGENTA, 22, 2, 18, 100, 300, 2, 2,      //
    PA_WSG_BLOCK_MAGENTA, 42, 11, 13, 100, 400, 2, 2, // 73
};

#define DEMO_CONTROLS_SCRIPT_TABLE_LENGTH     17
#define DEMO_CONTROLS_BTN_STATE_LOOKUP_OFFSET 0
#define DEMO_CONTROLS_DURATION_LOOKUP_OFFSET  1
#define DEMO_CONTROLS_SCRIPT_TABLE_ROW_LENGTH 2

static const uint16_t demoControlsScript[DEMO_CONTROLS_SCRIPT_TABLE_LENGTH * DEMO_CONTROLS_SCRIPT_TABLE_ROW_LENGTH] = {
    PB_RIGHT, 54,               //
    PB_UP,    16,               //
    PB_RIGHT, 32,               //
    PB_DOWN,  16,               //
    PB_RIGHT, 60,               //
    PB_A,     4,                //
    PB_RIGHT, 32,               //
    PB_A,     4,                //
    PB_RIGHT, 28,               //
    PB_UP,    16,               //
    PB_RIGHT, 32,               //
    PB_DOWN,  16,               //
    PB_LEFT,  4,                //
    PB_A,     4,                //
    PB_DOWN,  16,               //
    0,        80, PB_LEFT, 120, //
};

#define DEMO_SCORING_SCRIPT_TABLE_LENGTH     17
#define DEMO_SCORING_BTN_STATE_LOOKUP_OFFSET 0
#define DEMO_SCORING_DURATION_LOOKUP_OFFSET  1
#define DEMO_SCORING_SCRIPT_TABLE_ROW_LENGTH 2

static const uint16_t demoScoringScript[DEMO_SCORING_SCRIPT_TABLE_LENGTH * DEMO_SCORING_SCRIPT_TABLE_ROW_LENGTH] = {
    PB_RIGHT, 54,               //
    PB_A,     6,                //
    PB_DOWN,  30,               //
    PB_RIGHT, 4,                //
    PB_A,     6,                //
    PB_RIGHT, 36,               //
    PB_A,     6,                //
    PB_DOWN,  36,               //
    PB_RIGHT, 4,                //
    PB_A,     6,                //
    PB_RIGHT, 54,               //
    PB_A,     6,                //
    0,        4,                //
    0,        1,                //
    PB_DOWN,  16,               //
    0,        80, PB_LEFT, 120, //
};

static const paletteColor_t greenColors[4] = {c555, c051, c030, c051};

static const uint8_t ledRemap[] = {4, 5, 6, 7, 8, 0, 1, 2, 3};

#endif