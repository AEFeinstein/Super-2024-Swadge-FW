//==============================================================================
// Includes
//==============================================================================
#include "starfield.h"
#include <esp_random.h>
#include "hdw-tft.h"
#include "palette.h"
#include "fill.h"

//==============================================================================
// Functions
//==============================================================================
void initializeStarfield(starfield_t *self){
    for(uint16_t i=0; i<NUM_STARS; i++){
        self->stars[i].x = randomInt(-TFT_WIDTH / 2, TFT_WIDTH / 2);
        self->stars[i].y = randomInt(-TFT_HEIGHT / 2, TFT_HEIGHT / 2);
        self->stars[i].z = 1 + esp_random() % 1023;
    }
}

void updateStarfield(starfield_t *self){
    for(uint16_t i = 0; i < NUM_STARS; i++)
    {   
        self->stars[i].z -= 5;
        if(self->stars[i].z <= 0)
        {
            self->stars[i].x = randomInt(-TFT_WIDTH / 2, TFT_WIDTH / 2);
            self->stars[i].y = randomInt(-TFT_HEIGHT / 2, TFT_HEIGHT / 2);
            self->stars[i].z += 1024;
        }
    }
}

int randomInt(int lowerBound, int upperBound)
{
    return esp_random() % (upperBound - lowerBound + 1) + lowerBound;
}

void drawStarfield(starfield_t *self){
    //clearDisplay();

    /* rendering */
    for(uint16_t i = 0; i < NUM_STARS; i++)
    {
        /* Move and size the star */
        int temp[2];

        temp[0] = ((1024 * self->stars[i].x) / self->stars[i].z) + TFT_WIDTH / 2;
        temp[1] = ((1024 * self->stars[i].y) / self->stars[i].z) + TFT_HEIGHT / 2;

        //translate(&temp, TFT_WIDTH / 2, TFT_HEIGHT / 2);

        /* Draw the star */
        if( self->stars[i].z < 205)
        {
            fillDisplayArea(temp[0] - 3, temp[1] - 1, temp[0] + 3, temp[1] + 1, c555);
            fillDisplayArea(temp[0] - 1, temp[1] - 3, temp[0] + 1, temp[1] + 3, c555);
            fillDisplayArea(temp[0] - 2, temp[1] - 2, temp[0] + 2, temp[1] + 2, c555);
            setPxTft(temp[0], temp[1], c555);
        }
        else if (self->stars[i].z < 410)
        {
            fillDisplayArea(temp[0] - 2, temp[1] - 1, temp[0] + 2, temp[1] + 1, c444);
            fillDisplayArea(temp[0] - 1, temp[1] - 2, temp[0] + 1, temp[1] + 2, c444);
            setPxTft(temp[0], temp[1], c555);
        }
        else if (self->stars[i].z < 614)
        {
            fillDisplayArea(temp[0] - 1, temp[1], temp[0] + 2, temp[1] + 1, c333);
            fillDisplayArea(temp[0], temp[1] - 1, temp[0] + 1, temp[1] + 2, c333);
        }
        else if (self->stars[i].z < 819)
        {
            fillDisplayArea(temp[0], temp[1], temp[0] + 1, temp[1] + 1, c222);
        }
        else
        {
            setPxTft(temp[0], temp[1], c222);
        }
    }
}