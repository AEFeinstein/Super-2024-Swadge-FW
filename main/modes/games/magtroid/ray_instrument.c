//==============================================================================
// Includes
//==============================================================================

#include "ray_instrument.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Check for button input while showing instrument and either show the next part of the instrument or return to
 * the game loop
 *
 * @param ray The entire game state
 */
void rayInstrumentCheckButtons(ray_t* ray)
{
    // Check the button queue
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            // Any button returns to the game
            raySwitchToScreen(RAY_GAME);
            globalMidiPlayerResumeAll();
            return;
        }
    }

    // Read touchpads
    linearTouch_t touches[2] = {0};
    getTouchLinear(touches, ARRAY_SIZE(touches));
    const linearTouch_t* lTouch = &touches[0];
    const linearTouch_t* rTouch = &touches[1];

    if (lTouch)
    {
        if (lTouch->touched)
        {
            int32_t lTouchZone = lTouch->position / (1024 / 4);
            if (lTouchZone != ray->is.lTouchZone)
            {
                if (ray->is.lTouchZone >= 0)
                {
                    printf("Stop %" PRId8 "\n", ray->is.lTouchZone);
                }
                ray->is.lTouchZone = lTouchZone;
                printf("Play %" PRId32 "\n", lTouchZone);

                ray->is.notesActive &= 0xF0;
                ray->is.notesActive |= (1 << lTouchZone);
            }
        }
        else
        {
            if (ray->is.lTouchZone >= 0)
            {
                printf("Stop %" PRId8 "\n", ray->is.lTouchZone);
                ray->is.notesActive &= 0xF0;
                ray->is.lTouchZone = -1;
            }
        }
    }

    if (rTouch)
    {
        if (rTouch->touched)
        {
            int32_t rTouchZone = rTouch->position / (1024 / 4);
            if (rTouchZone != ray->is.rTouchZone)
            {
                if (ray->is.rTouchZone >= 0)
                {
                    printf("Stop %" PRId8 "\n", 4 + ray->is.rTouchZone);
                }
                ray->is.rTouchZone = rTouchZone;
                printf("Play %" PRId32 "\n", 4 + rTouchZone);

                ray->is.notesActive &= 0x0F;
                ray->is.notesActive |= (1 << (4 + rTouchZone));
            }
        }
        else
        {
            if (ray->is.rTouchZone >= 0)
            {
                printf("Stop %" PRId8 "\n", 4 + ray->is.rTouchZone);
                ray->is.notesActive &= 0x0F;
                ray->is.rTouchZone = -1;
            }
        }
    }
}

/**
 * @brief Render the current instrument box
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayInstrumentRender(ray_t* ray, uint32_t elapsedUs)
{
    drawText(&ray->ibm, c005, "INSTRUMENT TIME", 20, (TFT_HEIGHT - ray->ibm.height) / 2);

    const int step = (TFT_WIDTH / 8);
    for (int i = 0; i < 8; i++)
    {
        if (ray->is.notesActive & (1 << i))
        {
            fillDisplayArea(i * step, 200, (i + 1) * step, TFT_HEIGHT, c050);
        }
    }
}
