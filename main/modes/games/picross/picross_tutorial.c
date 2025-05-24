#include <string.h>
#include <stdlib.h>

#include "swadgeMode.h"
#include "aabb_utils.h"
#include "esp_log.h"

#include "picross_menu.h"
#include "picross_tutorial.h"

//function primitives
void drawTutorial(void);
void drawNavigation(void);
void drawPicrossQRCode(void);

//variables
picrossTutorial_t* tut;

void picrossStartTutorial(display_t* disp, font_t* font)
{
    tut = calloc(1, sizeof(picrossTutorial_t));

    tut->d = disp;
    tut->titleFont = *font;
    loadWsg("tut.wsg",&tut->qrlink);
    loadFont("ibm_vga8.font", &tut->smallFont);

    tut->pageIndex = 0;
    tut->totalPages = 1;

    tut->prevBtn = 0;
}
void picrossTutorialLoop(int64_t elapsedUs)
{
    //user input
    //exit on hitting select
    if((tut->btn & (SELECT | BTN_B)) && !(tut->prevBtn & SELECT))
    {
        //by convention of the rest of the code base, freeing memory and going back to menu in different functions
        //(maybe that shouldnt be how it is *cough*)
        picrossExitTutorial();
        exitTutorial();
        return;
    }else if((tut->btn & LEFT) && !(tut->prevBtn & LEFT))
    {
        //by convention of the rest of the code base, freeing memory and going back to menu in different functions
        //(maybe that shouldnt be how it is *cough*)
        if(tut->pageIndex == 0)
        {
            tut->pageIndex = tut->totalPages - 1;
        }else{
            tut->pageIndex--;
        }
    }else if((tut->btn & RIGHT) && !(tut->prevBtn & RIGHT))
    {
        //by convention of the rest of the code base, freeing memory and going back to menu in different functions
        //(maybe that shouldnt be how it is *cough*)
        if(tut->pageIndex == tut->totalPages - 1)
        {
            tut->pageIndex = 0;
        }else{
            tut->pageIndex++;
        }
    }

    //draw screen
    drawTutorial();

    tut->prevBtn = tut->btn;
}

void drawTutorial()
{
    tut->d->clearPx();
    //draw page tut->pageIndex of tutorial
    drawNavigation();
    drawPicrossQRCode();
    if(tut->pageIndex == 0)
    {
      
    }
    ///QR codes?
}

void drawNavigation()
{
    char textBuffer1[12];
    sprintf(textBuffer1, "How To Play");
    int16_t t = textWidth(&tut->titleFont,textBuffer1);
    t = ((tut->d->w) - t)/2;
    drawText(tut->d,&tut->titleFont,c555,textBuffer1,t, 16);
    
    char textBuffer2[20];
    sprintf(textBuffer2, "swadge.com/picross/");
    int16_t x = textWidth(&tut->smallFont,textBuffer2);
    x = ((tut->d->w) - x)/2;
    drawText(tut->d,&tut->smallFont,c555,textBuffer2,x,tut->d->h - 20);
}

void picrossTutorialButtonCb(buttonEvt_t* evt)
{
    tut->btn = evt->button;
}

void picrossExitTutorial(void)
{
    //Is this function getting called twice?
    freeWsg(&(tut->qrlink));
    freeFont(&(tut->smallFont));
    // freeFont(&(tut->titleFont));
    // free(&(tut->d));
    free(tut);
}

void drawPicrossQRCode()
{
    uint16_t pixelPerPixel = 6;
    uint16_t xOff = tut->d->w/2 - (tut->qrlink.w*pixelPerPixel/2);
    uint16_t yOff = tut->d->h/2 - (tut->qrlink.h*pixelPerPixel/2) + 10;

    for(int16_t srcY = 0; srcY < tut->qrlink.h; srcY++)
    {
        for(int16_t srcX = 0; srcX < tut->qrlink.w; srcX++)
        {
            // Draw if not transparent
            if (cTransparent != tut->qrlink.px[(srcY * tut->qrlink.w) + srcX])
            {
                // Transform this pixel's draw location as necessary
                int16_t dstX = srcX*pixelPerPixel+xOff;
                int16_t dstY = srcY*pixelPerPixel+yOff;
                
                // Check bounds
                if(0 <= dstX && dstX < tut->d->w && 0 <= dstY && dstY <= tut->d->h)
                {
                    //root pixel

                    // Draw the pixel
                    for(int i = 0;i<pixelPerPixel;i++)
                    {
                        for(int j = 0;j<pixelPerPixel;j++)
                        {
                            tut->d->setPx(dstX+i, dstY+j, tut->qrlink.px[(srcY * tut->qrlink.w) + srcX]);
                        }
                    }
                }
            }
        }
    }
}