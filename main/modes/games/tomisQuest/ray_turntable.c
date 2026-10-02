//==============================================================================
// Includes
//==============================================================================

#include "ray_turntable.h"
#include "ray_script.h"
#include "ray_tex_manager.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Check for button input while showing turntable and either show the next part of the turntable or return to
 * the game loop
 *
 * @param ray The entire game state
 */
void rayTurntableCheckButtons(ray_t* ray)
{
    // Check the button queue
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        // TODO @jarettmillard: handle turntable button inputs
    }

    // Read touchpads
    linearTouch_t touches[2] = {0};
    getTouchLinear(touches, ARRAY_SIZE(touches));
    // TODO @jarettmillard: handle turntable touch inputs
}

/**
 * @brief Render the current turntable box
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayTurntableRender(ray_t* ray, uint32_t elapsedUs)
{
    // TODO @jarettmillard: implement turntable minigame main loop here. Everything below is example

    // 33.3 RPM is 12000 degrees every 60000000uS, or 1 degree every 5000 uS
    RUN_TIMER_EVERY(ray->ts.rotationTimer, 5000, elapsedUs, {
        ray->ts.angle++;
        if (360 == ray->ts.angle)
        {
            ray->ts.angle = 0;

            // Increment count for full turns
            ray->ts.turns++;
            // After two full turns, execute the script and return to the main game
            if (2 == ray->ts.turns)
            {
                // TODO @jarettmillard: on success call these two lines
                executeScriptEvent(ray, ray->ts.turntableScript, getTexByType(ray, OBJ_SCENERY_TURNTABLE));
                raySwitchToScreen(RAY_GAME);
            }
        }
    });

#define T_RAD (TFT_WIDTH / 4)

    // Draw the turntable deck
    fillDisplayArea(0,                             //
                    (TFT_HEIGHT / 2) - T_RAD - 20, //
                    TFT_WIDTH,                     //
                    (TFT_HEIGHT / 2) + T_RAD + 20, c444);

    // Calculate the endpoint of the rotating line
    uint32_t lineEndX = (T_RAD * getSin1024(ray->ts.angle)) / 1024;
    uint32_t lineEndY = -(T_RAD * getCos1024(ray->ts.angle)) / 1024;

    // Draw one record
    drawCircleFilled(T_RAD, TFT_HEIGHT / 2, T_RAD, c000);
    drawCircleFilled(T_RAD, TFT_HEIGHT / 2, T_RAD / 4, c555);
    drawLine(T_RAD, TFT_HEIGHT / 2, T_RAD + lineEndX, (TFT_HEIGHT / 2) + lineEndY, c555, 0);

    // Draw another record
    drawCircleFilled(3 * T_RAD, TFT_HEIGHT / 2, T_RAD, c000);
    drawCircleFilled(3 * T_RAD, TFT_HEIGHT / 2, T_RAD / 4, c555);
    drawLine(3 * T_RAD, TFT_HEIGHT / 2, (3 * T_RAD) + lineEndX, (TFT_HEIGHT / 2) + lineEndY, c555, 0);
}
