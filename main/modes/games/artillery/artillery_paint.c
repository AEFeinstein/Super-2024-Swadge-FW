//==============================================================================
// Includes
//==============================================================================

#include "artillery_paint.h"

//==============================================================================
// Static Const Variables
//==============================================================================

static const char key_tankColor[] = "tankColor";

static const paletteColor_t tankPaints[][2] = {
    {c433, c322}, {c554, c211}, {c441, c220}, {c254, c032}, {c234, c123}, {c405, c304}, {c411, c300}, {c322, c211},
    {c431, c320}, {c241, c030}, {c513, c402}, {c151, c030}, {c515, c404}, {c104, c003}, {c131, c020}, {c451, c230},
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 * @param ad
 */
void artilleryPaintLoadColor(artilleryData_t* ad)
{
    if (!readNvs32(key_tankColor, &ad->myColorIdx))
    {
        ad->myColorIdx = 0;
    }
}

/**
 * @brief TODO doc
 *
 * @param ad
 * @param evt
 */
void artilleryPaintInput(artilleryData_t* ad, buttonEvt_t* evt)
{
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_LEFT:
            {
                if (ad->myColorIdx > 0)
                {
                    ad->myColorIdx--;
                }
                else
                {
                    ad->myColorIdx = ARRAY_SIZE(tankPaints) - 1;
                }
                break;
            }
            case PB_RIGHT:
            {
                ad->myColorIdx = (ad->myColorIdx + 1) % ARRAY_SIZE(tankPaints);
                break;
            }
            default:
            {
                writeNvs32(key_tankColor, ad->myColorIdx);
                artilleryPaintLoadColor(ad);
                ad->mState = AMS_MENU;
                setDrawBody(ad->mRenderer, true);
                break;
            }
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param ad
 * @param elapsedUs
 */
void artilleryPaintLoop(artilleryData_t* ad, uint32_t elapsedUs)
{
// Draw sky and ground
#define HORIZON 176
    fillDisplayArea(0, 0, TFT_WIDTH, HORIZON, COLOR_SKY);
    fillDisplayArea(0, HORIZON, TFT_WIDTH, TFT_HEIGHT, COLOR_GROUND);

    // Draw the menu text
    drawText(ad->mRenderer->titleFont, c555, str_paintSelect, 20, 13);
    // Outline the menu text
    drawText(ad->mRenderer->titleFontOutline, c000, str_paintSelect, 20, 13);

    // Blink arrows
    RUN_TIMER_EVERY(ad->paintArrowBlinkTimer, 1000000, elapsedUs, {});
    if (ad->paintArrowBlinkTimer < 1000000 / 2)
    {
        font_t* font = ad->mRenderer->titleFont;
        int xMargin  = 32;
        int yOff     = TFT_HEIGHT / 2 + 15 - (font->height / 2);

        const char lArrow[] = "<-";
        const char rArrow[] = "->";

        drawTextShadow(font, COLOR_TEXT, COLOR_TEXT_SHADOW, lArrow, xMargin, yOff);
        drawTextShadow(font, COLOR_TEXT, COLOR_TEXT_SHADOW, rArrow, TFT_WIDTH - xMargin - textWidth(font, rArrow),
                       yOff);
    }

    // Draw tank
    paletteColor_t bCol = tankPaints[ad->myColorIdx][0];
    paletteColor_t hCol = tankPaints[ad->myColorIdx][1];

    drawTank(TFT_WIDTH / 2, TFT_HEIGHT / 2 + 15, 40, bCol, hCol, 5);
}

/**
 * @brief TODO
 *
 * @return int32_t
 */
int32_t artilleryGetNumTankColors(void)
{
    return ARRAY_SIZE(tankPaints);
}

/**
 * @brief TODO doc
 *
 * @param idx
 * @param base
 * @param accent
 */
void artilleryGetTankColors(int32_t idx, paletteColor_t* base, paletteColor_t* accent)
{
    if (idx < 0 || idx >= ARRAY_SIZE(tankPaints))
    {
        idx = 0;
    }

    *base   = tankPaints[idx][0];
    *accent = tankPaints[idx][1];
}

/**
 * @brief TODO doc
 *
 * @param x
 * @param y
 * @param r
 * @param baseColor
 * @param accentColor
 * @param barrelWidth 0 through 5
 */
void drawTank(int32_t x, int32_t y, int32_t r, paletteColor_t baseColor, paletteColor_t accentColor,
              int32_t barrelWidth)
{
    circleFl_t c = {
        .pos    = {.x = x, .y = y},
        .radius = r,
    };

    // Draw a thick barrel
    vecFl_t relBarrelTip = {
        .x = sinf(M_PIf / 4) * c.radius * 2,
        .y = -cosf(M_PIf / 4) * c.radius * 2,
    };
    vecFl_t absBarrelTip = addVecFl2d(c.pos, relBarrelTip);
    vecFl_t offsets[]    = {
        {-1, -1}, {-1, 0}, {0, 0}, {0, 1}, {1, 1},
    };
    int32_t mid = (ARRAY_SIZE(offsets) / 2) + 1;
    for (int32_t oIdx = mid - (barrelWidth / 2); oIdx < mid + (barrelWidth / 2); oIdx++)
    {
        drawLineFast(c.pos.x + offsets[oIdx].x,        //
                     c.pos.y + offsets[oIdx].y,        //
                     absBarrelTip.x + offsets[oIdx].x, //
                     absBarrelTip.y + offsets[oIdx].y, //
                     accentColor);
    }

    // Draw tank body
    drawCircleFilled(c.pos.x, //
                     c.pos.y, //
                     c.radius, baseColor);

    // and some wheels too
    float wheelR = c.radius / 2.0f;
    float wheelY = c.radius - wheelR;

    // Find the vector pointing from the center of the tank to the floor
    vecFl_t wheelOffVert = {
        .x = 0,
        .y = 1,
    };

    // Rotate by 90 deg, doesn't matter which way
    vecFl_t wheelOffHorz = {
        .x = wheelOffVert.y,
        .y = -wheelOffVert.x,
    };

    // Scale vectors to place the wheels
    vecFl_t treadOff = mulVecFl2d(wheelOffVert, wheelR);
    wheelOffVert     = mulVecFl2d(wheelOffVert, wheelY);
    wheelOffHorz     = mulVecFl2d(wheelOffHorz, c.radius);

    // Draw first wheel
    vecFl_t w1 = addVecFl2d(c.pos, addVecFl2d(wheelOffVert, wheelOffHorz));
    drawCircleFilled(w1.x, w1.y, wheelR, accentColor);

    // Draw second wheel
    vecFl_t w2 = addVecFl2d(c.pos, subVecFl2d(wheelOffVert, wheelOffHorz));
    drawCircleFilled(w2.x, w2.y, wheelR, accentColor);

    // Draw top tread
    drawLineFast(w1.x + treadOff.x, //
                 w1.y + treadOff.y, //
                 w2.x + treadOff.x, //
                 w2.y + treadOff.y, //
                 accentColor);
    drawLineFast(w1.x - treadOff.x, //
                 w1.y - treadOff.y, //
                 w2.x - treadOff.x, //
                 w2.y - treadOff.y, //
                 accentColor);
}