/**
 * @file artillery_paint.c
 * @author gelakinetic (gelakinetic@gmail.com)
 * @brief The screen to pick a paint color for Vector Tanks
 * @date 2025-11-26
 */

//==============================================================================
// Includes
//==============================================================================

#include "hdw-nvs.h"

#include "artillery.h"
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
 * @brief Load the paint color from NVS
 *
 * @param ad All the artillery mode data
 * @return true if the color was loaded, false if it wasn't
 */
bool artilleryPaintLoadColor(artilleryData_t* ad)
{
    if (!readNvs32(key_tankColor, &ad->myColorIdx))
    {
        ad->myColorIdx = 0;
        return false;
    }
    return true;
}

/**
 * @brief Process button input for the paint screen
 *
 * @param ad All the artillery mode data
 * @param evt The button event to process
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
            case PB_START:
            case PB_SELECT:
            case PB_A:
            case PB_B:
            {
                // Write the value, reload it, and return to the menu
                writeNvs32(key_tankColor, ad->myColorIdx);
                artilleryPaintLoadColor(ad);
                ad->mState = AMS_MENU;
                setDrawBody(ad->mRenderer, true);
                break;
            }
            case PB_UP:
            case PB_DOWN:
            default:
            {
                // Do nothing
                break;
            }
        }
    }
}

/**
 * @brief Draw the paint selection screen
 *
 * @param ad All the artillery mode data
 * @param elapsedUs The time since this function was last called
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

    vecFl_t wheelOffVert = {
        .x = 0,
        .y = 1,
    };

    float radius         = 40;
    vecFl_t relBarrelTip = {
        .x = sinf(M_PIf / 4) * radius * 2,
        .y = -cosf(M_PIf / 4) * radius * 2,
    };

    drawTank(TFT_WIDTH / 2, TFT_HEIGHT / 2 + 15, radius, bCol, hCol, 5, wheelOffVert, relBarrelTip);
}

/**
 * @brief Return the number of possible paint color pairs
 *
 * @return The number of possible paint colors
 */
int32_t artilleryGetNumTankColors(void)
{
    return ARRAY_SIZE(tankPaints);
}

/**
 * @brief Get the tank color pair at a given index
 *
 * @param idx The index to get a paint color pair from
 * @param base [OUT] The base color
 * @param accent [OUT] the accent color
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
 * @brief Draw a tank
 *
 * @param x The X point to center the tank body at
 * @param y The Y point to center the tank body at
 * @param r The radius of the tank body
 * @param baseColor The tank's base color (body)
 * @param accentColor The tank's accent color (wheels, treads, barrel)
 * @param barrelWidth The width of the barrel, 0 through 5
 * @param wheelOffVert A normalized vector pointing from the center of the tank down to the ground
 * @param relBarrelTip Where the tip of the tank's barrel is
 */
void drawTank(int32_t x, int32_t y, int32_t r, paletteColor_t baseColor, paletteColor_t accentColor,
              int32_t barrelWidth, vecFl_t wheelOffVert, vecFl_t relBarrelTip)
{
    circleFl_t c = {
        .pos    = {.x = x, .y = y},
        .radius = r,
    };

    // Draw a barrel with the given thickness
    vecFl_t absBarrelTip = addVecFl2d(c.pos, relBarrelTip);

    // Offsets to draw a line at
    const vecFl_t offsets[] = {
        {-1, -1}, {-1, 0}, {0, 0}, {0, 1}, {1, 1},
    };

    // Draw lines at the offsets for the width
    int32_t start = (ARRAY_SIZE(offsets) / 2) - (barrelWidth / 2);
    int32_t end   = start + barrelWidth;
    for (int32_t oIdx = start; oIdx < end; oIdx++)
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