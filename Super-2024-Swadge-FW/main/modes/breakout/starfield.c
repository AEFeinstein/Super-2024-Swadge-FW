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
void initializeStarfield(starfield_t *self, bool randomColors){
    for(uint16_t i=0; i<NUM_STARS; i++){
        self->stars[i].x = randomInt(-TFT_WIDTH / 2, TFT_WIDTH / 2);
        self->stars[i].y = randomInt(-TFT_HEIGHT / 2, TFT_HEIGHT / 2);
        self->stars[i].z = 1 + esp_random() % 1023;
        self->randomColors = randomColors;
        self->stars[i].color = esp_random() % cTransparent;
    }
}

void updateStarfield(starfield_t *self, int32_t scale){
    for(uint16_t i = 0; i < NUM_STARS; i++)
    {   
        self->stars[i].z -= scale;
        if(self->stars[i].z <= 0)
        {
            self->stars[i].x = randomInt(-TFT_WIDTH / 2, TFT_WIDTH / 2);
            self->stars[i].y = randomInt(-TFT_HEIGHT / 2, TFT_HEIGHT / 2);
            self->stars[i].z += 1024;
            self->stars[i].color = esp_random() % cTransparent;
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
        paletteColor_t col = self->stars[i].color;
        if( self->stars[i].z < 205)
        {
            if(!self->randomColors)
            {
                col = c555;
            }
            fillDisplayArea(temp[0] - 3, temp[1] - 1, temp[0] + 3, temp[1] + 1, col);
            fillDisplayArea(temp[0] - 1, temp[1] - 3, temp[0] + 1, temp[1] + 3, col);
            fillDisplayArea(temp[0] - 2, temp[1] - 2, temp[0] + 2, temp[1] + 2, col);
            setPxTft(temp[0], temp[1], col);
        }
        else if (self->stars[i].z < 410)
        {
            if(!self->randomColors)
            {
                col = c444;
            }
            fillDisplayArea(temp[0] - 2, temp[1] - 1, temp[0] + 2, temp[1] + 1, col);
            fillDisplayArea(temp[0] - 1, temp[1] - 2, temp[0] + 1, temp[1] + 2, col);
            setPxTft(temp[0], temp[1], col);
        }
        else if (self->stars[i].z < 614)
        {
            if(!self->randomColors)
            {
                col = c333;
            }
            fillDisplayArea(temp[0] - 1, temp[1], temp[0] + 2, temp[1] + 1, col);
            fillDisplayArea(temp[0], temp[1] - 1, temp[0] + 1, temp[1] + 2, col);
        }
        else if (self->stars[i].z < 819)
        {
            if(!self->randomColors)
            {
                col = c222;
            }
            fillDisplayArea(temp[0], temp[1], temp[0] + 1, temp[1] + 1, col);
        }
        else
        {
            if(!self->randomColors)
            {
                col = c222;
            }
            setPxTft(temp[0], temp[1], col);
        }
    }
}