//==============================================================================
// Includes
//==============================================================================

#include "artillery_help.h"

//==============================================================================
// Static Const Variables
//==============================================================================

const char helpTitle[] = "Help";
const char loremIpsum[]
    = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur malesuada mattis eros. Duis non blandit "
      "arcu. Praesent viverra eget tortor eget interdum. Sed id vulputate ante. Etiam ut ligula a libero iaculis "
      "pretium ut ut eros. Integer justo nulla, sagittis quis congue quis, mattis ac est. Quisque posuere velit metus. "
      "Donec nulla lectus, posuere ut urna cursus, tempus sagittis eros. Maecenas at scelerisque purus, vitae accumsan "
      "ipsum. Ut ante erat, semper in eleifend quis, viverra et quam. Vivamus ut mi vel lectus tempor interdum. "
      "Integer ac lacus mauris. Sed a neque massa. Class aptent taciti sociosqu ad litora torquent per conubia nostra, "
      "per inceptos himenaeos. Donec non suscipit ex. Etiam molestie ultricies ante, sit amet pharetra lorem tempor "
      "quis.";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 * @param ad
 * @param evt
 */
void artilleryHelpInput(artilleryData_t* ad, buttonEvt_t* evt)
{
    if (evt->down)
    {
        switch (evt->button)
        {
            default:
            {
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
void artilleryHelpLoop(artilleryData_t* ad, uint32_t elapsedUs)
{
    // Draw background
    ad->blankMenu->title = helpTitle;
    drawMenuMega(ad->blankMenu, ad->mRenderer, elapsedUs);

#define TEXT_MARGIN   4
#define TEXT_MARGIN_L (20 + TEXT_MARGIN)
#define TEXT_MARGIN_R (24 + TEXT_MARGIN)
#define TEXT_MARGIN_U (54 + TEXT_MARGIN)
#define TEXT_MARGIN_D (23 + TEXT_MARGIN)

    font_t* f = ad->mRenderer->menuFont;

    int16_t xOff = TEXT_MARGIN_L + 1;
    int16_t yOff = TEXT_MARGIN_U + 1;
    drawTextWordWrap(f, COLOR_TEXT_SHADOW, loremIpsum, &xOff, &yOff, TFT_WIDTH - TEXT_MARGIN_R + 1,
                     TFT_HEIGHT - TEXT_MARGIN_D + 1);

    xOff = TEXT_MARGIN_L;
    yOff = TEXT_MARGIN_U;
    drawTextWordWrap(f, COLOR_TEXT, loremIpsum, &xOff, &yOff,
                     // TEXT_MARGIN, OFFSET_Y + TEXT_MARGIN,
                     TFT_WIDTH - TEXT_MARGIN_R, TFT_HEIGHT - TEXT_MARGIN_D);
}
