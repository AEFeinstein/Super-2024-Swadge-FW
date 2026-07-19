//==============================================================================
// Includes
//==============================================================================
#include "bombadeetle.h"


//==============================================================================
// Defines
//==============================================================================
#define TAG                         "BOMBADEETLE"
#define OFFSETMAP_X                 50
#define OFFSETMAP_Y                 5
#define ANIMATIONSPEED              125
#define MOVETIME                    75
#define TILESIZE                    16

#define GRIDHEIGHT                  9
#define GRIDWIDTH                   12

#define WALL_W                       1
#define WALL_S                       2
#define WALL_E                       4
#define WALL_N                       8

#define DIRECTION_NONE                    0
#define DIRECTION_W                       1
#define DIRECTION_S                       2
#define DIRECTION_E                       4
#define DIRECTION_N                       8

#define BOMBADEETLE_FRAMECOUNT       3
#define BOMBADEETLE_COUNT            100

const char bombadeetleModeName[] = "Bombadeetle";

typedef enum
{
    MENU,
    PLACING,
    RUNNING
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
    BOMB_ARROW_RIGHT_WSG, BOMB_ARROW_DOWN_WSG,BOMB_ARROW_LEFT_WSG,BOMB_ARROW_UP_WSG,
};

static const cnfsFileIdx_t bombadeetleCursor[] ={
    BOMB_SBOX_001_WSG, BOMB_SBOX_002_WSG, BOMB_SBOX_003_WSG,
};

static const cnfsFileIdx_t bombadeetleWall[] = {
    BOMB_WALL_V_WSG, BOMB_WALL_H_WSG
};

static const cnfsFileIdx_t bombadeetleBombadeetleSprite[] = {
    BOMB_BEETLETES_101_WSG, BOMB_BEETLETES_102_WSG, BOMB_BEETLETES_103_WSG, BOMB_BEETLETES_201_WSG, BOMB_BEETLETES_202_WSG, BOMB_BEETLETES_203_WSG, BOMB_BEETLETES_301_WSG, BOMB_BEETLETES_302_WSG, BOMB_BEETLETES_303_WSG,
};

static const int bombadeetleCursorFrames[] ={
    0, 1, 2, 1,
};


static void bombadeetleEnterMode(void);
static void bombadeetleExitMode(void);
static void bombadeetleMainLoop(int64_t elapsedUs);
static void bombadeetleLoadMap(int64_t index);


swadgeMode_t bombadeetleMode = {
    .modeName                   = bombadeetleModeName,
    .wifiMode                   = NO_WIFI,

    .overrideUsb                = false,
    .usesAccelerometer          = false,
    .usesThermometer            = false,
    .overrideSelectBtn          = false,
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

typedef struct
{
    int8_t direction; 
    int8_t tileX;
    int8_t tileY;
    int8_t frame;
} bombadeetleBombadeetleData_t;

typedef struct
{
    bombState_t state;

    wsg_t* arrows;
    wsg_t* cursor;
    wsg_t* walls;
    wsg_t* bombadeetleSprites;

    bool building;

    bombadeetleBackground_t background;
    int8_t cursorFrame;
    int16_t cursorTime;
    int8_t cursorX;
    int8_t cursorY;

    int16_t cursorMoveTime;

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

    bombadeetle->cursorMoveTime = 0;

    bombadeetle->bombadeetles = (bombadeetleBombadeetleData_t*)heap_caps_calloc(BOMBADEETLE_COUNT, sizeof(bombadeetleBombadeetleData_t), MALLOC_CAP_8BIT);

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

    //Fake map loading
    bombadeetle->map[0] |= WALL_E;
    bombadeetle->map[1] |= WALL_W | WALL_E;
    bombadeetle->map[2] |= WALL_W |WALL_S;
    bombadeetle->map[14] |=WALL_N |WALL_S;
    bombadeetle->map[26] |=WALL_N | WALL_E;
    bombadeetle->map[27] |= WALL_W | WALL_E;
    bombadeetle->map[28] |= WALL_W;

    bombadeetle->bombadeetles[0].direction = DIRECTION_W;
    bombadeetle->bombadeetles[0].tileY = 0;
    bombadeetle->bombadeetles[0].tileX = 3;
    

    bombadeetle->bombadeetles[1].direction = DIRECTION_E;
    bombadeetle->bombadeetles[1].tileY = 1;
    bombadeetle->bombadeetles[1].tileX = 3;

    

    bombadeetle->bombadeetles[2].direction = DIRECTION_N;
    bombadeetle->bombadeetles[2].tileY = 2;
    bombadeetle->bombadeetles[2].tileX = 3;

    

    bombadeetle->bombadeetles[3].direction = DIRECTION_S;
    bombadeetle->bombadeetles[3].tileY = 3;
    bombadeetle->bombadeetles[3].tileX = 3;
    //bombadeetleLoadMap(0);
}

static void bombadeetleLoadMap(int64_t index)
{

}

static void bombadeetleMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    int16_t tick = elapsedUs / 1000;
    int8_t gridIndex = bombadeetle->cursorX + (bombadeetle->cursorY * GRIDWIDTH);
    int8_t tileIndex = 0;
    bombadeetle->cursorTime += tick;
    if (bombadeetle->cursorTime > ANIMATIONSPEED)
    {
        bombadeetle->cursorTime -= ANIMATIONSPEED;
        bombadeetle->cursorFrame ++;
        bombadeetle->cursorFrame %=4;
    }

    if (bombadeetle->cursorMoveTime > 0)
    {
        bombadeetle->cursorMoveTime -= tick;
    }
    
    for(int i = 0; i < BOMBADEETLE_COUNT; i++)
    {
        if (bombadeetle->bombadeetles[i].direction != 0)
        {
            bombadeetle->bombadeetles[i].frame++;
            bombadeetle->bombadeetles[i].frame %= BOMBADEETLE_FRAMECOUNT;
        }
    }

    //Controls
    switch (bombadeetle->state)
    {
        case PLACING:
        default:
            while(checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {

                    if (bombadeetle->building)
                    {
                        if (evt.button & PB_DOWN)
                        {
                            bombadeetle->grid[gridIndex] = 2;                       
                        }
                        else if (evt.button & PB_UP)
                        {
                            bombadeetle->grid[gridIndex] = 4;
                        }
                        else if (evt.button & PB_LEFT)
                        {
                            bombadeetle->grid[gridIndex] = 3;
                        }
                        else if (evt.button & PB_RIGHT)
                        {
                            bombadeetle->grid[gridIndex] = 1;
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
                        bombadeetle->grid[gridIndex] = 0;
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
                drawWsgSimple(&bombadeetle->arrows[bombadeetle->grid[tileIndex] - 1],  OFFSETMAP_X + (x * 16 + 1), OFFSETMAP_Y + (y * 16) + 1);
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
        }
    }
    
    for( int idx =0; idx < BOMBADEETLE_COUNT; idx++)
    {
        switch (bombadeetle->bombadeetles[idx].direction)
        {
            case DIRECTION_E:
                drawWsgSimple(&bombadeetle->bombadeetleSprites[bombadeetle->bombadeetles[idx].frame],OFFSETMAP_X + (bombadeetle->bombadeetles[idx].tileX * TILESIZE), OFFSETMAP_Y + (bombadeetle->bombadeetles[idx].tileY * TILESIZE));
                break;
            case DIRECTION_N:
                drawWsgSimple(&bombadeetle->bombadeetleSprites[bombadeetle->bombadeetles[idx].frame + 3],OFFSETMAP_X + (bombadeetle->bombadeetles[idx].tileX * TILESIZE), OFFSETMAP_Y + (bombadeetle->bombadeetles[idx].tileY * TILESIZE));
                break;
            case DIRECTION_S:
                drawWsgSimple(&bombadeetle->bombadeetleSprites[bombadeetle->bombadeetles[idx].frame + 6],OFFSETMAP_X + (bombadeetle->bombadeetles[idx].tileX * TILESIZE), OFFSETMAP_Y + (bombadeetle->bombadeetles[idx].tileY * TILESIZE));
                break;
                
            case DIRECTION_W:
                drawWsg(&bombadeetle->bombadeetleSprites[bombadeetle->bombadeetles[idx].frame],OFFSETMAP_X + (bombadeetle->bombadeetles[idx].tileX * TILESIZE), OFFSETMAP_Y + (bombadeetle->bombadeetles[idx].tileY * TILESIZE), true, false, 0);
                break;

            default:
                break;
        }
    }

    //drawWsgSimple(&bombadeetle->bombadeetleSprites[0],OFFSETMAP_X, OFFSETMAP_Y);

    drawWsgSimple(&bombadeetle->cursor[bombadeetleCursorFrames[bombadeetle->cursorFrame]], OFFSETMAP_X + (bombadeetle->cursorX * TILESIZE) - 2, OFFSETMAP_Y + (bombadeetle->cursorY * TILESIZE) - 2);

}

static void bombadeetleExitMode()
{
    heap_caps_free(bombadeetle);
}