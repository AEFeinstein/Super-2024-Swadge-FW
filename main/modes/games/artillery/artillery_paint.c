//==============================================================================
// Includes
//==============================================================================

#include "artillery_paint.h"

//==============================================================================
// Static Const Variables
//==============================================================================

static const char key_tankColor[] = "tankColor";

static const paletteColor_t cOpts[][2] = {
    {c401, c301}, {c102, c001}, {c004, c003}, {c505, c413}, {c033, c022}, {c305, c214}, {c521, c421},
    {c503, c402}, {c241, c131}, {c541, c431}, {c203, c102}, {c333, c444}, {c000, c222}, {c545, c535},
    {c543, c433}, {c554, c253}, {c455, c054}, {c233, c122}, {c544, c533}, {c435, c425},
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
                    ad->myColorIdx = ARRAY_SIZE(cOpts) - 1;
                }
                break;
            }
            case PB_RIGHT:
            {
                ad->myColorIdx = (ad->myColorIdx + 1) % ARRAY_SIZE(cOpts);
                break;
            }
            default:
            {
                writeNvs32(key_tankColor, ad->myColorIdx);
                artilleryPaintLoadColor(ad);
                ad->mState = AMS_MENU;
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
    // Draw background
    drawMenuMega(ad->blankMenu, ad->mRenderer, elapsedUs);

    // Blink arrows
    RUN_TIMER_EVERY(ad->paintArrowBlinkTimer, 1000000, elapsedUs, {});
    if (ad->paintArrowBlinkTimer < 1000000 / 2)
    {
        font_t* font        = ad->mRenderer->titleFont;
        font_t* fontOutline = ad->mRenderer->titleFontOutline;
        int xMargin         = 32;
        int yOff            = TFT_HEIGHT / 2 + 15 - (font->height / 2);

        const char lArrow[] = "<-";
        const char rArrow[] = "->";

        drawText(font, c555, lArrow, xMargin, yOff);
        drawText(fontOutline, c000, lArrow, xMargin, yOff);
        drawText(font, c555, rArrow, TFT_WIDTH - xMargin - textWidth(font, rArrow), yOff);
        drawText(fontOutline, c000, rArrow, TFT_WIDTH - xMargin - textWidth(font, rArrow), yOff);
    }

    // Draw tank
    paletteColor_t bCol = cOpts[ad->myColorIdx][0];
    paletteColor_t hCol = cOpts[ad->myColorIdx][1];

    drawTank(TFT_WIDTH / 2, TFT_HEIGHT / 2 + 15, 40, bCol, hCol, 5);
}

/**
 * @brief TODO
 *
 * @return int32_t
 */
int32_t artilleryGetNumTankColors(void)
{
    return ARRAY_SIZE(cOpts);
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
    if (idx < 0 || idx >= ARRAY_SIZE(cOpts))
    {
        idx = 0;
    }

    *base   = cOpts[idx][0];
    *accent = cOpts[idx][1];
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
        .x = sinf(M_PI / 4) * c.radius * 2,
        .y = -cosf(M_PI / 4) * c.radius * 2,
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