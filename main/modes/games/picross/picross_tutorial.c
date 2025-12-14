#include <string.h>
#include <stdlib.h>

#include "swadge2024.h"

#include "picross_menu.h"
#include "picross_tutorial.h"

// function primitives
void drawTutorial(void);
void drawNavigation(void);
void drawPicrossQRCode(void);

// variables
picrossTutorial_t* tut;

void picrossStartTutorial(font_t* font)
{
    tut = heap_caps_calloc(1, sizeof(picrossTutorial_t), MALLOC_CAP_8BIT);

    tut->titleFont = *font;
    loadWsg(TUT_WSG, &tut->qrlink, false);
    loadFont(IBM_VGA_8_FONT, &tut->smallFont, false);

    tut->pageIndex  = 0;
    tut->totalPages = 1;

    tut->prevBtn = 0;
}
void picrossTutorialLoop(int64_t elapsedUs)
{
    // user input
    // exit on hitting select
    if ((tut->btn & (PB_START | PB_B)) && !(tut->prevBtn & PB_START))
    {
        // by convention of the rest of the code base, freeing memory and going back to menu in different functions
        //(maybe that shouldnt be how it is *cough*)
        picrossExitTutorial();
        exitTutorial();
        return;
    }
    else if ((tut->btn & PB_LEFT) && !(tut->prevBtn & PB_LEFT))
    {
        // by convention of the rest of the code base, freeing memory and going back to menu in different functions
        //(maybe that shouldnt be how it is *cough*)
        if (tut->pageIndex == 0)
        {
            tut->pageIndex = tut->totalPages - 1;
        }
        else
        {
            tut->pageIndex--;
        }
    }
    else if ((tut->btn & PB_RIGHT) && !(tut->prevBtn & PB_RIGHT))
    {
        // by convention of the rest of the code base, freeing memory and going back to menu in different functions
        //(maybe that shouldnt be how it is *cough*)
        if (tut->pageIndex == tut->totalPages - 1)
        {
            tut->pageIndex = 0;
        }
        else
        {
            tut->pageIndex++;
        }
    }

    // draw screen
    drawTutorial();

    tut->prevBtn = tut->btn;
}

void drawTutorial()
{
    clearPxTft();
    // draw page tut->pageIndex of tutorial
    drawNavigation();
    drawPicrossQRCode();
    if (tut->pageIndex == 0)
    {
    }
    /// QR codes?
}

void drawNavigation()
{
    char textBuffer1[12];
    sprintf(textBuffer1, "How To Play");
    int16_t t = textWidth(&tut->titleFont, textBuffer1);
    t         = ((TFT_WIDTH)-t) / 2;
    drawText(&tut->titleFont, c555, textBuffer1, t, 16);

    char textBuffer2[20];
    sprintf(textBuffer2, "swadge.com/picross/");
    int16_t x = textWidth(&tut->smallFont, textBuffer2);
    x         = ((TFT_WIDTH)-x) / 2;
    drawText(&tut->smallFont, c555, textBuffer2, x, TFT_HEIGHT - 20);
}

void picrossTutorialButtonCb(buttonEvt_t* evt)
{
    tut->btn = evt->button;
}

void picrossExitTutorial(void)
{
    // Is this function getting called twice?
    freeWsg(&(tut->qrlink));
    freeFont(&(tut->smallFont));
    // freeFont(&(tut->titleFont));
    // heap_caps_free(&(tut->d));
    heap_caps_free(tut);
}

void drawPicrossQRCode()
{
    uint16_t pixelPerPixel = 6;
    uint16_t xOff          = TFT_WIDTH / 2 - (tut->qrlink.w * pixelPerPixel / 2);
    uint16_t yOff          = TFT_HEIGHT / 2 - (tut->qrlink.h * pixelPerPixel / 2) + 10;

    for (int16_t srcY = 0; srcY < tut->qrlink.h; srcY++)
    {
        for (int16_t srcX = 0; srcX < tut->qrlink.w; srcX++)
        {
            // Draw if not transparent
            if (cTransparent != tut->qrlink.px[(srcY * tut->qrlink.w) + srcX])
            {
                // Transform this pixel's draw location as necessary
                int16_t dstX = srcX * pixelPerPixel + xOff;
                int16_t dstY = srcY * pixelPerPixel + yOff;

                // Check bounds
                if (0 <= dstX && dstX < TFT_WIDTH && 0 <= dstY && dstY <= TFT_HEIGHT)
                {
                    // root pixel

                    // Draw the pixel
                    for (int i = 0; i < pixelPerPixel; i++)
                    {
                        for (int j = 0; j < pixelPerPixel; j++)
                        {
                            setPxTft(dstX + i, dstY + j, tut->qrlink.px[(srcY * tut->qrlink.w) + srcX]);
                        }
                    }
                }
            }
        }
    }
}