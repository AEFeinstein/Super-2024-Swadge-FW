//==============================================================================
// Includes
//==============================================================================
#include "bombadeetle.h"
#include "cnfs_image.h"
#include "wsg.h"
#include <ctype.h>

//==============================================================================
// Defines
//==============================================================================
#define TAG                         "BOMBADEETLE"
#define OFFSETMAP_X                 44
#define OFFSETMAP_Y                 10
#define ANIMATIONSPEED              125
#define MOVETIME                    75
#define TILESIZE                    16

#define GRIDHEIGHT                  9
#define GRIDWIDTH                   12

#define WALL_W                      1
#define WALL_S                      2
#define WALL_E                      4
#define WALL_N                      8
#define HOLE                        16
#define GOAL                        32


#define DIRECTION_NONE                    0
#define DIRECTION_W                       1
#define DIRECTION_S                       2
#define DIRECTION_E                       4
#define DIRECTION_N                       8

#define BG_LEVELNAME_X              44
#define BG_LEVELNAME_Y              166

#define BOMBADEETLE_FRAMECOUNT       4
#define BOMBADEETLE_COUNT            100



const char bombadeetleModeName[] = "Bombadeetle";

typedef enum
{
    MENU,
    STATE_PLACING,
    STATE_RUNNING,
    STATE_WIN,

} bombState_t;

typedef enum
{
    NOTHOLDING,
    HOLDING,
} bombCursorState_t;

static const cnfsFileIdx_t bombadeetleTiles[] ={
    BOMB_TILE_001_WSG, BOMB_TILE_002_WSG
};

static const cnfsFileIdx_t bombadeetleArrows[] ={
    BOMB_ARROW_LEFT_WSG, BOMB_ARROW_DOWN_WSG, BOMB_ARROW_RIGHT_WSG, BOMB_ARROW_UP_WSG,
};

static const cnfsFileIdx_t bombadeetleCursor[] ={
    BOMB_SBOX_001_WSG, BOMB_SBOX_002_WSG, BOMB_SBOX_003_WSG,
};
static const cnfsFileIdx_t bombadeetleActiveCursor[] ={
    BOMB_ABOX_001_WSG, BOMB_ABOX_002_WSG, BOMB_ABOX_003_WSG,
};

static const cnfsFileIdx_t bombadeetleWall[] = {
    BOMB_WALL_V_WSG, BOMB_WALL_H_WSG
};

static const cnfsFileIdx_t bombadeetleBombadeetleSprite[] = {
    BOMB_BEETLETES_101_WSG, BOMB_BEETLETES_102_WSG, BOMB_BEETLETES_103_WSG, BOMB_BEETLETES_201_WSG, BOMB_BEETLETES_202_WSG, BOMB_BEETLETES_203_WSG, BOMB_BEETLETES_301_WSG, BOMB_BEETLETES_302_WSG, BOMB_BEETLETES_303_WSG,
};

static const cnfsFileIdx_t bombadeetleGoal[] = {
    BOMB_GOAL_001_WSG, BOMB_GOAL_002_WSG, BOMB_GOAL_003_WSG, BOMB_GOAL_004_WSG, BOMB_GOAL_005_WSG, BOMB_GOAL_006_WSG,
};

static const cnfsFileIdx_t bombadeetleLevels[] = {
    BOMB_ONE_BIN, BOMB_HELLO_BIN,
};

static const int bombadeetleCursorFrames[] ={
    0, 1, 2, 1,
};
static const int bombadeetleBeetleFrames[] ={
    0, 1, 2, 1,
};

static const int bombadeetleGoalAnimation[] ={
    0, 1, 2, 3, 4, 5, 4, 3,
};


static void bombadeetleEnterMode(void);
static void bombadeetleExitMode(void);
static void bombadeetleMainLoop(int64_t elapsedUs);
static void bombadeetleLoadMap();
static void bombadeetleImportMap(int64_t index);
static bool bombadeetleMove(int tile, int direction);
static int bombadeetleRight(int direction);
static int bombadeetleLeft(int direction);
static int bombadeetleTurnAround(int direction);




swadgeMode_t bombadeetleMode = {
    .modeName                   = bombadeetleModeName,
    .wifiMode                   = NO_WIFI,

    .overrideUsb                = false,
    .usesAccelerometer          = false,
    .usesThermometer            = false,
    .overrideSelectBtn          = true,
    .fnEnterMode                = bombadeetleEnterMode,
    .fnExitMode                 = bombadeetleExitMode,
    .fnMainLoop                 = bombadeetleMainLoop,
    .fnAudioCallback            = NULL,
    .fnBackgroundDrawCallback   = NULL,
    .fnEspNowRecvCb             = NULL,
    .fnEspNowSendCb             = NULL,
    .fnAdvancedUSB              = NULL,
};

typedef struct
{
    wsg_t* tiles;
    
} bombadeetleBackground_t;

typedef struct{
    char name[17];
    int8_t map[GRIDWIDTH * GRIDHEIGHT];
    int8_t bombadeetleSpawn[GRIDWIDTH * GRIDHEIGHT];
    int8_t left;
    int8_t up;
    int8_t down;
    int8_t right;

} bombadeetleCurrentLevel_t;

typedef struct
{
    int8_t direction; 
    int8_t tileX;
    int8_t tileY;
    int8_t frame;
    bool goal;
    bool doomed;
    
} bombadeetleBombadeetleData_t;

typedef struct
{
    bombState_t state;

    bombadeetleCurrentLevel_t mapFile;

    wsg_t* arrows;
    wsg_t* cursor;
    wsg_t* acursor;
    wsg_t* walls;
    wsg_t* bombadeetleSprites;
    wsg_t* goal;
    wsg_t success;
    wsg_t levelNameBackground;
    wsg_t tools;

    bool building;
    bool goalAnimating;
    
    bombadeetleBackground_t background;
    int8_t cursorFrame;
    int16_t cursorTime;
    int8_t cursorX;
    int8_t cursorY;

    int8_t goalFrame;
    int16_t goalTime;
    int8_t goalCount;

    int8_t levelIndex;
    int8_t levelMax;

    int16_t successTime;

    
    int8_t characterMoveAmount;
    font_t mainFont;

    int16_t cursorMoveTime;
    int16_t characterMoveTime;

    int8_t grid[GRIDHEIGHT * GRIDWIDTH]; // I don't like this.
    int8_t map[GRIDHEIGHT * GRIDWIDTH];

    bombadeetleBombadeetleData_t* bombadeetles;
    
} bombadeetleData_t;


bombadeetleData_t* bombadeetle;


static void bombadeetleEnterMode()
{
    bombadeetle = (bombadeetleData_t*)heap_caps_calloc(1,sizeof(bombadeetleData_t), MALLOC_CAP_8BIT);

    bombadeetle->cursorTime = 0;
    bombadeetle->cursorX = 0;
    bombadeetle->cursorY = 0;
    bombadeetle->building = false;
    bombadeetle->goalCount = 0;
    bombadeetle->successTime = 0;

    bombadeetle->cursorMoveTime = 0;

    bombadeetle->bombadeetles = (bombadeetleBombadeetleData_t*)heap_caps_calloc(BOMBADEETLE_COUNT, sizeof(bombadeetleBombadeetleData_t), MALLOC_CAP_8BIT);

    
    //Load from disk to see what the current level max is
    bombadeetle->levelMax = 0;
    bombadeetle->levelIndex = 0;

    loadWsg(BOMB_SUCCESS_WSG, &bombadeetle->success, true);
    loadWsg(BOMB_LEVEL_NAME_WSG, &bombadeetle->levelNameBackground, true);
    loadWsg(BOMB_TOOLS_WSG, &bombadeetle->tools, true);

    bombadeetle->background.tiles = heap_caps_calloc(ARRAY_SIZE(bombadeetleTiles), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleTiles); idx++)
    {
        loadWsg(bombadeetleTiles[idx], &bombadeetle->background.tiles[idx], true);
    }
    
    bombadeetle->arrows = heap_caps_calloc(ARRAY_SIZE(bombadeetleArrows), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleArrows); idx++)
    {
        loadWsg(bombadeetleArrows[idx], &bombadeetle->arrows[idx], true);
    }

    bombadeetle->cursor = heap_caps_calloc(ARRAY_SIZE(bombadeetleCursor), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleCursor); idx++)
    {
        loadWsg(bombadeetleCursor[idx], &bombadeetle->cursor[idx], true);
    }
    
    bombadeetle->acursor = heap_caps_calloc(ARRAY_SIZE(bombadeetleActiveCursor), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleActiveCursor); idx++)
    {
        loadWsg(bombadeetleActiveCursor[idx], &bombadeetle->acursor[idx], true);
    }

    bombadeetle->walls = heap_caps_calloc(ARRAY_SIZE(bombadeetleWall), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleWall); idx++)
    {
        loadWsg(bombadeetleWall[idx], &bombadeetle->walls[idx], true);
    }

    bombadeetle->bombadeetleSprites = heap_caps_calloc(ARRAY_SIZE(bombadeetleBombadeetleSprite), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleBombadeetleSprite); idx++)
    {
        loadWsg(bombadeetleBombadeetleSprite[idx], &bombadeetle->bombadeetleSprites[idx], true);
    }

    bombadeetle->goal = heap_caps_calloc(ARRAY_SIZE(bombadeetleGoal), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bombadeetleGoal); idx++)
    {
        loadWsg(bombadeetleGoal[idx], &bombadeetle->goal[idx], true);
    }
    loadFont(BOMBBADEETLE_FONT, &bombadeetle->mainFont, true);

    
    for (int idx = 0; idx < GRIDHEIGHT * GRIDWIDTH; idx++)
    {
        bombadeetle->grid[idx] = 0;
        bombadeetle->map[idx] = 0; //Clear it before loading level. 

        if (idx % GRIDWIDTH == 0)
        {
            bombadeetle->map[idx] |= WALL_W;
        }

        if (idx % GRIDWIDTH == GRIDWIDTH - 1)
        {
            bombadeetle->map[idx] |= WALL_E;
        }

        if (idx < GRIDWIDTH)
        {
            bombadeetle->map[idx] |= WALL_N;
        }

        if (idx / GRIDWIDTH == GRIDHEIGHT - 1)
        {
            bombadeetle->map[idx] |= WALL_S;
        }
    }


    bombadeetleImportMap(0);
    bombadeetleLoadMap();
}

static void bombadeetleImportMap(int64_t index)
{
    //Clear previous map?

    for(int idx = 0; idx < BOMBADEETLE_COUNT; idx++)
    {
        bombadeetle->bombadeetles[idx].direction = 0;
        bombadeetle->bombadeetles[idx].goal = false;
        bombadeetle->bombadeetles[idx].doomed = false;
        bombadeetle->bombadeetles[idx].frame = 0;
        bombadeetle->bombadeetles[idx].tileX = -1;
        bombadeetle->bombadeetles[idx].tileY = -1;
    }

    //Bring in new map
    size_t levelSize = 0;
    int offset = 0;
    uint8_t *levelFile = cnfsGetFile(bombadeetleLevels[index], &levelSize);
    bombadeetle->goalCount = 0;

    for (int idx = 0; idx < 16; idx ++)
    {
        bombadeetle->mapFile.name[idx] = toupper(levelFile[idx]);
    }


    
    for (int idx = 0; idx < GRIDHEIGHT * GRIDWIDTH; idx++)
    {
        bombadeetle->grid[idx] = 0;
    }
    
    offset = 16;
    for (int idx = offset; idx < offset + 108; idx++)
    {
        bombadeetle->mapFile.map[idx - offset] = levelFile[idx];
        bombadeetle->map[idx - offset] = levelFile[idx];

        if ((idx - offset) % GRIDWIDTH == 0)
        {
            bombadeetle->map[(idx - offset)] |= WALL_W;
        }

        if ((idx - offset) % GRIDWIDTH == GRIDWIDTH - 1)
        {
            bombadeetle->map[(idx - offset)] |= WALL_E;
        }

        if ((idx - offset) < GRIDWIDTH)
        {
            bombadeetle->map[(idx - offset)] |= WALL_N;
        }

        if ((idx - offset) / GRIDWIDTH == GRIDHEIGHT - 1)
        {
            bombadeetle->map[(idx - offset)] |= WALL_S;
        }
    }

    offset += 108;    
    for (int idx = offset; idx < offset + 108; idx++)
    {
        bombadeetle->mapFile.bombadeetleSpawn[idx - offset] = levelFile[idx];
    }

    offset += 108;
    for (int idx = offset; idx < offset + 108; idx++)
    {
    //    bombadeetle->mapFile.bombadeetleSpawn[idx - offset] = levelFile[idx];
    }

    offset += 108;
    
    bombadeetle->mapFile.left = levelFile[offset+ 0];
    bombadeetle->mapFile.up = levelFile[offset+ 1];
    bombadeetle->mapFile.down = levelFile[offset+ 2];
    bombadeetle->mapFile.right = levelFile[offset+ 3];
    //ESP_LOGI(TAG, "%d %d %d %d", levelFile[offset +0], levelFile[offset +1], levelFile[offset +2], levelFile[offset +3] );
}

static void bombadeetleLoadMap()
{
    int bombadeetleIndex = 0;
    int tileIndex = 0;
    ESP_LOGI(TAG, "%d %d", bombadeetle->map[0], bombadeetle->map[1]);
    
    bombadeetle->state = STATE_PLACING;    
    bombadeetle->characterMoveTime = 0;
    bombadeetle->characterMoveAmount = 0;
    bombadeetle->goalCount = 0;

    for (int idx = 0; idx < BOMBADEETLE_COUNT; idx++)
    {
        bombadeetle->bombadeetles[idx].direction = DIRECTION_NONE;
    }

    for (int idx = 0; idx < GRIDHEIGHT * GRIDWIDTH; idx++)
    {
        tileIndex = bombadeetle->mapFile.bombadeetleSpawn[idx];
        if (tileIndex & DIRECTION_N || tileIndex & DIRECTION_E || tileIndex & DIRECTION_S || tileIndex & DIRECTION_W)
        {
            if (bombadeetleIndex >= BOMBADEETLE_COUNT) continue;
            

            bombadeetle->goalCount++;
            bombadeetle->bombadeetles[bombadeetleIndex].direction = tileIndex;
            bombadeetle->bombadeetles[bombadeetleIndex].tileX = idx % GRIDWIDTH;
            bombadeetle->bombadeetles[bombadeetleIndex].tileY = idx / GRIDWIDTH;
            bombadeetleIndex++;

        }
    }
}

static bool bombadeetleMove(int tile, int direction)
{
    return (tile & direction) == 0;
}

static int bombadeetleRight(int direction)
{
    switch (direction)
    {
        case DIRECTION_W: return DIRECTION_N;
        case DIRECTION_N: return DIRECTION_E;
        case DIRECTION_E: return DIRECTION_S;
        case DIRECTION_S: return DIRECTION_W;
    }

    return direction;
}

static int bombadeetleLeft(int direction)
{
    switch (direction)
    {
        case DIRECTION_W: return DIRECTION_S;
        case DIRECTION_S: return DIRECTION_E;
        case DIRECTION_E: return DIRECTION_N;
        case DIRECTION_N: return DIRECTION_W;
    }

    return direction;
}

static int bombadeetleTurnAround(int direction)
{
    switch (direction)
    {
        case DIRECTION_W: return DIRECTION_E;
        case DIRECTION_E: return DIRECTION_W;
        case DIRECTION_N: return DIRECTION_S;
        case DIRECTION_S: return DIRECTION_N;
    }

    return direction;
}

static void bombadeetleMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    int16_t tick = elapsedUs / 1000;
    int8_t gridIndex = bombadeetle->cursorX + (bombadeetle->cursorY * GRIDWIDTH);
    int8_t tileIndex = 0;
    bool update = false;
    int8_t winCount = 0;

    bombadeetle->cursorTime += tick;
    if (bombadeetle->cursorTime > ANIMATIONSPEED)
    {
        bombadeetle->cursorTime -= ANIMATIONSPEED;
        bombadeetle->cursorFrame ++;
        bombadeetle->cursorFrame %=4;
        
        for(int i = 0; i < BOMBADEETLE_COUNT; i++)
        {
            if (bombadeetle->bombadeetles[i].direction != 0)
            {
                bombadeetle->bombadeetles[i].frame++;
                bombadeetle->bombadeetles[i].frame %= BOMBADEETLE_FRAMECOUNT;
            }
        }
    }

    if (bombadeetle->cursorMoveTime > 0)
    {
        bombadeetle->cursorMoveTime -= tick;
    }
    

    //Controls
    switch (bombadeetle->state)
    {
        case STATE_WIN:
            while(checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_A)
                    {
                        //Reset map
                        bombadeetle->levelIndex++;
                        
                        if (bombadeetle->levelIndex >= ARRAY_SIZE(bombadeetleLevels))
                        {
                            bombadeetle->levelIndex = ARRAY_SIZE(bombadeetleLevels ) - 1;
                        }

                        if (bombadeetle->levelIndex > bombadeetle->levelMax)
                        {
                            bombadeetle->levelMax = bombadeetle->levelIndex;
                            //Save max level
                        }

                        bombadeetleImportMap(bombadeetle->levelIndex);
                        bombadeetleLoadMap();
                    }
                }
            }
        case STATE_RUNNING:
            while(checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_B)
                    {
                        //Reset map
                        bombadeetleLoadMap();
                    }
                }
            }
        case STATE_PLACING:
        default:
            while(checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {

                    if (bombadeetle->building)
                    {
                        if (evt.button & PB_DOWN && bombadeetle->mapFile.down)
                        {
                            bombadeetle->mapFile.down--;
                            bombadeetle->grid[gridIndex] = DIRECTION_S;                       
                        }
                        else if (evt.button & PB_UP && bombadeetle->mapFile.up)
                        {
                            bombadeetle->mapFile.up--;
                            bombadeetle->grid[gridIndex] = DIRECTION_N;
                        }
                        else if (evt.button & PB_LEFT && bombadeetle->mapFile.left)
                        {
                            bombadeetle->grid[gridIndex] = DIRECTION_W;
                            bombadeetle->mapFile.left--;
                        }
                        else if (evt.button & PB_RIGHT && bombadeetle->mapFile.right)
                        {
                            bombadeetle->grid[gridIndex] = DIRECTION_E;
                            bombadeetle->mapFile.right--;
                        }
                    }
                    else {
                        
                        if (evt.button & PB_DOWN)
                        {
                            bombadeetle->cursorY++;                        
                            bombadeetle->cursorMoveTime = MOVETIME;       
                        }
                        else if (evt.button & PB_UP)
                        {
                            bombadeetle->cursorY--;
                            bombadeetle->cursorMoveTime = MOVETIME;
                        }
                        else if (evt.button & PB_LEFT)
                        {
                            bombadeetle->cursorX--;
                        }
                        else if (evt.button & PB_RIGHT)
                        {
                            bombadeetle->cursorX++;
                        }
                    }

                    if (evt.button & PB_B && !bombadeetle->building)
                    {
                        switch (bombadeetle->grid[gridIndex])
                        {
                            case DIRECTION_N:
                                bombadeetle->mapFile.up++;
                                break;
                            case DIRECTION_S:
                                bombadeetle->mapFile.down++;
                                break;
                                
                            case DIRECTION_E:
                                bombadeetle->mapFile.right++;
                                break;
                                
                            case DIRECTION_W:
                                bombadeetle->mapFile.left++;
                                break;
                        }
                        bombadeetle->grid[gridIndex] = 0;
                    }

                    if (evt.button & PB_START)
                    {
                        bombadeetle->state = STATE_RUNNING;
                    }
                }
                
                if (evt.button & PB_A)
                {
                    if (evt.down)
                    {
                        bombadeetle->building = true;
                        ESP_LOGI(TAG, "A button!");
                    }
                    else
                    {
                        bombadeetle->building = false;

                        ESP_LOGI(TAG, "NO A button!");
                    }
                }
            }
    }
    
    //Level updating

    if (bombadeetle->state == STATE_RUNNING)
    {
        bombadeetle->characterMoveTime += tick;
        if (bombadeetle->characterMoveTime > 25)
        {
            bombadeetle->characterMoveTime -= 25;
            bombadeetle->characterMoveAmount += 4;
            if (bombadeetle->characterMoveAmount >= TILESIZE)
            {
                bombadeetle->characterMoveAmount = 0;
                update = true;
            }
        } 

        for(int idx = 0; idx < BOMBADEETLE_COUNT; idx++)
        {
            if (bombadeetle->bombadeetles[idx].direction == DIRECTION_NONE) continue;
            if (bombadeetle->bombadeetles[idx].direction == GOAL) continue;

            if (update)
            {
                
                //Update direction
                switch (bombadeetle->bombadeetles[idx].direction)
                {
                    case DIRECTION_E:
                        bombadeetle->bombadeetles[idx].tileX++;
                        break;
                        case DIRECTION_N:
                        bombadeetle->bombadeetles[idx].tileY--;
                        break;
                    case DIRECTION_S:
                        bombadeetle->bombadeetles[idx].tileY++;
                        break;
                    case DIRECTION_W:
                        bombadeetle->bombadeetles[idx].tileX--;
                        break;
                    default:
                        continue;
                }

                //Check if on arrow tile
                if (bombadeetle->grid[(bombadeetle->bombadeetles[idx].tileX) + (bombadeetle->bombadeetles[idx].tileY * GRIDWIDTH)] != DIRECTION_NONE)
                {
                    bombadeetle->bombadeetles[idx].direction = bombadeetle->grid[(bombadeetle->bombadeetles[idx].tileX) + (bombadeetle->bombadeetles[idx].tileY * GRIDWIDTH)];
                }
                
                //Check direction
                int8_t tile;
                tile = bombadeetle->map[(bombadeetle->bombadeetles[idx].tileX) + (bombadeetle->bombadeetles[idx].tileY * GRIDWIDTH)];
                if (tile & GOAL)
                {
                    bombadeetle->bombadeetles[idx].direction = GOAL;
                    bombadeetle->bombadeetles[idx].goal = true;
                    bombadeetle->goalAnimating = true;
                    bombadeetle->goalFrame = 0;
                    bombadeetle->goalTime = 0;

                    ESP_LOGI(TAG, "GOAL!");
                }


                if (bombadeetleMove(tile, bombadeetle->bombadeetles[idx].direction))
                {

                }
                else if (bombadeetleMove(tile, bombadeetleRight(bombadeetle->bombadeetles[idx].direction)))
                {
                    bombadeetle->bombadeetles[idx].direction = bombadeetleRight(bombadeetle->bombadeetles[idx].direction);
                }
                else if (bombadeetleMove(tile, bombadeetleLeft(bombadeetle->bombadeetles[idx].direction)))
                {
                    bombadeetle->bombadeetles[idx].direction = bombadeetleLeft(bombadeetle->bombadeetles[idx].direction);                    
                }
                else
                {
                    bombadeetle->bombadeetles[idx].direction = bombadeetleTurnAround(bombadeetle->bombadeetles[idx].direction);
                }


            }
        }

        if (bombadeetle->goalAnimating)
        {
            bombadeetle->goalTime += tick;
            if (bombadeetle->goalTime >= 75)
            {
                bombadeetle->goalTime -= 75;
                bombadeetle->goalFrame++;
                
                if (bombadeetle->goalFrame >= ARRAY_SIZE(bombadeetleGoalAnimation))
                {
                    bombadeetle->goalFrame = 0;
                    bombadeetle->goalAnimating = false;
                }
            }
        }

        
        for (int idx = 0; idx < BOMBADEETLE_COUNT; idx++)
        {
            if (bombadeetle->bombadeetles[idx].goal)
            {
                winCount++;
            }
        }

        if (winCount >= bombadeetle->goalCount)
        {
            ESP_LOGI(TAG, "WIN!");
            bombadeetle->state = STATE_WIN;
            bombadeetle->successTime = 0;
        }

    }

    if (bombadeetle->cursorY >= GRIDHEIGHT)
    {
        bombadeetle->cursorY = GRIDHEIGHT - 1;
    }
    if (bombadeetle->cursorY < 0)
    {
        bombadeetle->cursorY = 0;
    }
    if (bombadeetle->cursorX < 0)
    {
        bombadeetle->cursorX = 0;
    }
    if (bombadeetle->cursorX >= GRIDWIDTH)
    {
        bombadeetle->cursorX = GRIDWIDTH - 1;
    }
    

    //Drawing has to be the last thing in the stack
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c112);

    for (int y = 0; y < GRIDHEIGHT; y++)
    {
        for (int x = 0; x < GRIDWIDTH; x++)
        {      
            tileIndex = (y * GRIDWIDTH)+ x;
            drawWsgSimple(&bombadeetle->background.tiles[(x+y)% 2], OFFSETMAP_X + (x * 16), OFFSETMAP_Y + (y * 16));
            if (bombadeetle->grid[tileIndex] != 0)
            {
                
                drawWsgSimple(&bombadeetle->arrows[__builtin_ctz(bombadeetle->grid[tileIndex])],  OFFSETMAP_X + (x * 16 + 1), OFFSETMAP_Y + (y * 16) + 1);
            }
        }
    }

    
    for (int y = 0; y < GRIDHEIGHT; y++)
    {
        for (int x = 0; x < GRIDWIDTH; x++)
        {
            tileIndex = (y * GRIDWIDTH)+ x;
            if (y == 0)
            {
                drawWsgSimple(&bombadeetle->walls[1], OFFSETMAP_X + (x * TILESIZE) - 2,  OFFSETMAP_Y + (y * TILESIZE) - 2);
            }

            if (x == 0)
            {
                drawWsgSimple(&bombadeetle->walls[0], OFFSETMAP_X + ((x) * TILESIZE) - 2, OFFSETMAP_Y + (y * TILESIZE) - 2);
                
            }
            
            if (bombadeetle->map[tileIndex] & WALL_E)
            {
                drawWsgSimple(&bombadeetle->walls[0], OFFSETMAP_X + ((x+1) * TILESIZE) - 2, OFFSETMAP_Y + (y * TILESIZE) - 2);
                //drawRectFilled(OFFSETMAP_X + ((x+1) * TILESIZE) - 2, OFFSETMAP_Y + (y * TILESIZE), OFFSETMAP_X + ((x+1) * TILESIZE) + 2, OFFSETMAP_Y + ((y +1) * TILESIZE) + 2, c505);
            }
            
            if (bombadeetle->map[tileIndex] & WALL_S)
            {
                drawWsgSimple(&bombadeetle->walls[1], OFFSETMAP_X + (x * TILESIZE) -2,  OFFSETMAP_Y + ((y+1) * TILESIZE) - 2);
                
                // drawRectFilled(OFFSETMAP_X + (x * TILESIZE) , OFFSETMAP_Y + ((y+1) * TILESIZE) - 2, OFFSETMAP_X + ((x+1) * TILESIZE), OFFSETMAP_Y + ((y+1) * TILESIZE) + 2, c505);
            }
            
            if (bombadeetle->map[tileIndex] & GOAL)
            {
                drawWsgSimple(&bombadeetle->goal[bombadeetleGoalAnimation[bombadeetle->goalFrame]],OFFSETMAP_X + (x * TILESIZE), OFFSETMAP_Y + (y * TILESIZE));
            }
        }
    }
    
    for( int idx =0; idx < BOMBADEETLE_COUNT; idx++)
    {
        switch (bombadeetle->bombadeetles[idx].direction)
        {
            case DIRECTION_E:
            drawWsgSimple(&bombadeetle->bombadeetleSprites[bombadeetleBeetleFrames[bombadeetle->bombadeetles[idx].frame]],OFFSETMAP_X + (bombadeetle->bombadeetles[idx].tileX * TILESIZE) + bombadeetle->characterMoveAmount, OFFSETMAP_Y + (bombadeetle->bombadeetles[idx].tileY * TILESIZE));
            break;
            case DIRECTION_N:
            drawWsgSimple(&bombadeetle->bombadeetleSprites[bombadeetleBeetleFrames[bombadeetle->bombadeetles[idx].frame] + 3],OFFSETMAP_X + (bombadeetle->bombadeetles[idx].tileX * TILESIZE), OFFSETMAP_Y + (bombadeetle->bombadeetles[idx].tileY * TILESIZE) - bombadeetle->characterMoveAmount);
            break;
            case DIRECTION_S:
            drawWsgSimple(&bombadeetle->bombadeetleSprites[bombadeetleBeetleFrames[bombadeetle->bombadeetles[idx].frame] + 6],OFFSETMAP_X + (bombadeetle->bombadeetles[idx].tileX * TILESIZE), OFFSETMAP_Y + (bombadeetle->bombadeetles[idx].tileY * TILESIZE) + bombadeetle->characterMoveAmount);
            break;
            
            case DIRECTION_W:
            drawWsg(&bombadeetle->bombadeetleSprites[bombadeetleBeetleFrames[bombadeetle->bombadeetles[idx].frame]],OFFSETMAP_X + (bombadeetle->bombadeetles[idx].tileX * TILESIZE) - bombadeetle->characterMoveAmount, OFFSETMAP_Y + (bombadeetle->bombadeetles[idx].tileY * TILESIZE), true, false, 0);
            break;
            
            case GOAL:
            //drawRectFilled(OFFSETMAP_X + (bombadeetle->bombadeetles[idx].tileX * TILESIZE), OFFSETMAP_Y + (bombadeetle->bombadeetles[idx].tileY * TILESIZE), OFFSETMAP_X + (bombadeetle->bombadeetles[idx].tileX * TILESIZE) + 16, OFFSETMAP_Y + (bombadeetle->bombadeetles[idx].tileY * TILESIZE) + 16, c505);
            break;
            
            default:
            break;
        }
    }
    
    drawWsgSimple(&bombadeetle->tools, 2, 31);

    char buffer[32];
    sprintf(buffer,  "%d" , bombadeetle->mapFile.right );
    drawText(&bombadeetle->mainFont, 112, buffer, 16,70);
    
    sprintf(buffer,  "%d" , bombadeetle->mapFile.left );
    drawText(&bombadeetle->mainFont, 112, buffer, 16,105);
    
    sprintf(buffer,  "%d" , bombadeetle->mapFile.down );
    drawText(&bombadeetle->mainFont, 112, buffer, 16,140);
    
    sprintf(buffer,  "%d" , bombadeetle->mapFile.up );
    drawText(&bombadeetle->mainFont, 112, buffer, 16,175);

    drawWsgSimple(&bombadeetle->levelNameBackground, BG_LEVELNAME_X, BG_LEVELNAME_Y);
    drawText(&bombadeetle->mainFont, 112, bombadeetle->mapFile.name, BG_LEVELNAME_X + 10, BG_LEVELNAME_Y + 8);
    
    
    if (bombadeetle->state == STATE_PLACING) 
    {
        if (bombadeetle->building)
        {
            drawWsgSimple(&bombadeetle->acursor[bombadeetleCursorFrames[bombadeetle->cursorFrame]], OFFSETMAP_X + (bombadeetle->cursorX * TILESIZE) - 2, OFFSETMAP_Y + (bombadeetle->cursorY * TILESIZE) - 2);
        }
        else
        {
            drawWsgSimple(&bombadeetle->cursor[bombadeetleCursorFrames[bombadeetle->cursorFrame]], OFFSETMAP_X + (bombadeetle->cursorX * TILESIZE) - 2, OFFSETMAP_Y + (bombadeetle->cursorY * TILESIZE) - 2);
        }
    }
    
    if (bombadeetle->state == STATE_WIN) 
    {
        drawWsgSimple(&bombadeetle->success, 33,61);
    }
    
}

static void bombadeetleExitMode()
{
    heap_caps_free(bombadeetle);
}