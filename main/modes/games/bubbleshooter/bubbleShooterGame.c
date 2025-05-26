#include "bubbleShooterGame.h"
#include "bubbleShooter.h"

#include <esp_log.h>
#include <stdlib.h>
#include "cnfs.h"


#define ROWS            12
#define COLS            8
#define NUM_COLORS      7
#define OFFSET_X        14
#define OFFSET_Y        12
#define LEVEL_OFFSET_X  76
#define LEVEL_OFFSET_Y  20

#define BUB_TAG         "BUBBLE"
void bubbleShooterDestroyGame(void);
void bubbleShooterGameMainLoop(int64_t elapsedUs);

bool bubbleShooterLoadLevel();

bubbleShooterGame_t* bsg;

void bubbleShooterInitGame()
{
    ESP_LOGI(BUB_TAG, "Here we go!");
    bsg = calloc(1, sizeof(bubbleShooterGame_t));

    loadWsg(BUBBLE_BUBBLE_SPRITE_1_WSG, &bsg->bubbleSprite[0], true);
    loadWsg(BUBBLE_BUBBLE_SPRITE_2_WSG, &bsg->bubbleSprite[1], true);
    loadWsg(BUBBLE_BUBBLE_SPRITE_3_WSG, &bsg->bubbleSprite[2], true);
    loadWsg(BUBBLE_BUBBLE_SPRITE_4_WSG, &bsg->bubbleSprite[3], true);
    loadWsg(BUBBLE_BUBBLE_SPRITE_5_WSG, &bsg->bubbleSprite[4], true);
    loadWsg(BUBBLE_BUBBLE_SPRITE_6_WSG, &bsg->bubbleSprite[5], true);
    loadWsg(BUBBLE_BUBBLE_SPRITE_7_WSG, &bsg->bubbleSprite[6], true);


    bubbleShooterLoadLevel();
    ESP_LOGI(BUB_TAG,"Loaded");

}

bool bubbleShooterLoadLevel()
{    

    size_t ms;
    uint8_t* buffer = cnfsReadFile(BUBBLE_SHOOTER_LEVEL_1_BIN, &ms, true);

    ESP_LOGI(BUB_TAG, "%d",(int)ms);

    for(int i = 20; i < 116; i++)
    {
        int index = i - 20;
        int x = ((index) % COLS);
        int y = ((index) / COLS);
        ESP_LOGI(BUB_TAG, "index %d == %d at %d,%d", index, buffer[i], x, y);
        bsg->grid[y][x] = buffer[i] - 1; //This minus one kills me :(
    }
    
    for (int y = 0; y < ROWS; y++) {
        
        for (int x = 0; x < COLS; x++) {

            ESP_LOGI(BUB_TAG,"%d", bsg->grid[y][x]);
        }
    }

    return true;
}

void bubbleShooterGameDraw()
{
    
    fillDisplayArea(0, 0, 280, 240, c134);
    
    int xOffset = 0;
    int bubbleIndex = 0;
    
    for (int y = 0; y < ROWS; y++) {
        xOffset = y % 2 == 1 ? 8 : 0;
        for (int x = 0; x < COLS; x++) {
            bubbleIndex = bsg->grid[y][x];
            
            if (bubbleIndex >= 0 && bubbleIndex < NUM_COLORS + 1)
            {
                drawWsgSimple(&bsg->bubbleSprite[bubbleIndex], LEVEL_OFFSET_X + (x * OFFSET_X)+ xOffset, LEVEL_OFFSET_Y + (y * OFFSET_Y));
            }
       }
    }
}

void bubbleShooterGameMainLoop(int64_t elapsedUs)
{

}

void bubbleShooterDestroyGame()
{
    //Unload sprites
}