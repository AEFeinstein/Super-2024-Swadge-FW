//==============================================================================
// Includes
//==============================================================================

#include "artillery_paint.h"

//==============================================================================
// Static Const Variables
//==============================================================================

static const char key_tankColor[] = "tankColor";

<<<<<<< HEAD
static const paletteColor_t cOpts[][2] = {
    {c401, c301}, {c102, c001}, {c004, c003}, {c505, c413}, {c033, c022}, {c305, c214}, {c521, c421},
    {c503, c402}, {c241, c131}, {c541, c431}, {c203, c102}, {c333, c444}, {c000, c222}, {c545, c535},
    {c543, c433}, {c554, c253}, {c455, c054}, {c233, c122}, {c544, c533}, {c435, c425},
=======
static const paletteColor_t tankPaints[][2] = {
    {c433, c322}, {c554, c211}, {c441, c220}, {c254, c032}, {c234, c123}, {c405, c304}, {c411, c300}, {c322, c211},
    {c431, c320}, {c241, c030}, {c513, c402}, {c151, c030}, {c515, c404}, {c104, c003}, {c131, c020}, {c451, c230},
>>>>>>> origin/main
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 * @param ad
 */
<<<<<<< HEAD
void artilleryPaintLoadColor(artilleryData_t* ad)
=======
bool artilleryPaintLoadColor(artilleryData_t* ad)
>>>>>>> origin/main
{
    if (!readNvs32(key_tankColor, &ad->myColorIdx))
    {
        ad->myColorIdx = 0;
<<<<<<< HEAD
    }
=======
        return false;
    }
    return true;
>>>>>>> origin/main
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
<<<<<<< HEAD
                    ad->myColorIdx = ARRAY_SIZE(cOpts) - 1;
=======
                    ad->myColorIdx = ARRAY_SIZE(tankPaints) - 1;
>>>>>>> origin/main
                }
                break;
            }
            case PB_RIGHT:
            {
<<<<<<< HEAD
                ad->myColorIdx = (ad->myColorIdx + 1) % ARRAY_SIZE(cOpts);
=======
                ad->myColorIdx = (ad->myColorIdx + 1) % ARRAY_SIZE(tankPaints);
>>>>>>> origin/main
                break;
            }
            default:
            {
                writeNvs32(key_tankColor, ad->myColorIdx);
                artilleryPaintLoadColor(ad);
                ad->mState = AMS_MENU;
<<<<<<< HEAD
=======
                setDrawBody(ad->mRenderer, true);
>>>>>>> origin/main
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
<<<<<<< HEAD
    // Draw background
    drawMenuMega(ad->blankMenu, ad->mRenderer, elapsedUs);
=======
// Draw sky and ground
#define HORIZON 176
    fillDisplayArea(0, 0, TFT_WIDTH, HORIZON, COLOR_SKY);
    fillDisplayArea(0, HORIZON, TFT_WIDTH, TFT_HEIGHT, COLOR_GROUND);

    // Draw the menu text
    drawText(ad->mRenderer->titleFont, c555, str_paintSelect, 20, 13);
    // Outline the menu text
    drawText(ad->mRenderer->titleFontOutline, c000, str_paintSelect, 20, 13);
>>>>>>> origin/main

    // Blink arrows
    RUN_TIMER_EVERY(ad->paintArrowBlinkTimer, 1000000, elapsedUs, {});
    if (ad->paintArrowBlinkTimer < 1000000 / 2)
    {
<<<<<<< HEAD
        font_t* font        = ad->mRenderer->titleFont;
        font_t* fontOutline = ad->mRenderer->titleFontOutline;
        int xMargin         = 32;
        int yOff            = TFT_HEIGHT / 2 + 15 - (font->height / 2);
=======
        font_t* font = ad->mRenderer->titleFont;
        int xMargin  = 32;
        int yOff     = TFT_HEIGHT / 2 + 15 - (font->height / 2);
>>>>>>> origin/main

        const char lArrow[] = "<-";
        const char rArrow[] = "->";

<<<<<<< HEAD
        drawText(font, c555, lArrow, xMargin, yOff);
        drawText(fontOutline, c000, lArrow, xMargin, yOff);
        drawText(font, c555, rArrow, TFT_WIDTH - xMargin - textWidth(font, rArrow), yOff);
        drawText(fontOutline, c000, rArrow, TFT_WIDTH - xMargin - textWidth(font, rArrow), yOff);
    }

    // Draw tank
    paletteColor_t bCol = cOpts[ad->myColorIdx][0];
    paletteColor_t hCol = cOpts[ad->myColorIdx][1];
=======
        drawTextShadow(font, COLOR_TEXT, COLOR_TEXT_SHADOW, lArrow, xMargin, yOff);
        drawTextShadow(font, COLOR_TEXT, COLOR_TEXT_SHADOW, rArrow, TFT_WIDTH - xMargin - textWidth(font, rArrow),
                       yOff);
    }

    // Draw tank
    paletteColor_t bCol = tankPaints[ad->myColorIdx][0];
    paletteColor_t hCol = tankPaints[ad->myColorIdx][1];
>>>>>>> origin/main

    drawTank(TFT_WIDTH / 2, TFT_HEIGHT / 2 + 15, 40, bCol, hCol, 5);
}

/**
 * @brief TODO
 *
 * @return int32_t
 */
int32_t artilleryGetNumTankColors(void)
{
<<<<<<< HEAD
    return ARRAY_SIZE(cOpts);
=======
    return ARRAY_SIZE(tankPaints);
>>>>>>> origin/main
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
<<<<<<< HEAD
    if (idx < 0 || idx >= ARRAY_SIZE(cOpts))
=======
    if (idx < 0 || idx >= ARRAY_SIZE(tankPaints))
>>>>>>> origin/main
    {
        idx = 0;
    }

<<<<<<< HEAD
    *base   = cOpts[idx][0];
    *accent = cOpts[idx][1];
=======
    *base   = tankPaints[idx][0];
    *accent = tankPaints[idx][1];
>>>>>>> origin/main
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