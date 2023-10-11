#ifndef _TOUCH_TEST_MODE_H_
#define _TOUCH_TEST_MODE_H_

#include "swadge2024.h"

extern swadgeMode_t touchTestMode;

/**
 * @brief
 *
 * @param x
 * @param y
 * @param r
 * @param segs
 */
static void touchDrawCircle(font_t* font, const char* label, int16_t x, int16_t y, int16_t r, int16_t segs, bool center,
                            bool touched, touchJoystick_t val)
{
    drawText(font, c555, label, x - textWidth(font, label) / 2,
             y - r - font->height - 5);

    // Draw outer circle
    drawCircle(x, y, r, c222);

    int16_t centerR = center ? 10 : 0;
    int16_t offset  = 360 - (360 / segs) / 2;

    // Draw the segment lines
    for (uint8_t sector = 0; sector < segs; sector++)
    {
        int16_t angle = (offset + (360 * sector / segs)) % 360;
        drawLineFast(x + getCos1024(angle) * centerR / 1024, y + getSin1024(angle) * centerR / 1024,
                     x + getCos1024(angle) * r / 1024, y + getSin1024(angle) * r / 1024, c222);
    }

    // Draw center circle?
    if (center)
    {
        drawCircle(x, y, centerR - 1, c222);
    }

    if (touched)
    {
        int16_t angle = 0;
        int16_t fillR = r / 2;

        switch (val)
        {
            case TB_CENTER:
                angle = 0;
                fillR = 0;
                break;

            case TB_RIGHT:
                angle = 0;
                break;

            case TB_UP | TB_RIGHT:
                angle = 45;
                break;

            case TB_UP:
                angle = 90;
                break;

            case TB_UP | TB_LEFT:
                angle = 135;
                break;

            case TB_LEFT:
                angle = 180;
                break;

            case TB_DOWN | TB_LEFT:
                angle = 225;
                break;

            case TB_DOWN:
                angle = 270;
                break;

            case TB_DOWN | TB_RIGHT:
                angle = 315;
                break;
        }

        // Fill in the segment
        floodFill(x + getCos1024(angle) * fillR / 1024, y - getSin1024(angle) * fillR / 1024, c555, x - r - 1,
                  y - r - 1, x + r + 1, y + r + 1);
    }
}

#endif