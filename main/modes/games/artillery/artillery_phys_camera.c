/**
 * @file artillery_phys_camera.c
 * @author gelakinetic (gelakinetic@gmail.com)
 * @brief TODO file summary
 * @date 2025-11-26
 */

//==============================================================================
// Includes
//==============================================================================

#include "hdw-tft.h"
#include "macros.h"
#include "artillery_phys_camera.h"

//==============================================================================
// Defines
//==============================================================================

#define CAMERA_BTN_MOVE_INTERVAL 2
#define CAMERA_MARGIN            64

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Save the button state in order to pan the camera
 *
 * @param phys The physics simulation to pan
 * @param btn The current button state
 */
void physSetCameraButton(physSim_t* phys, buttonBit_t btn)
{
    phys->cameraBtn = btn;
}

/**
 * @brief Move the camera according to input buttons or object tracking
 *
 * This is run every PHYS_TIME_STEP_US
 *
 * @param phys The physics simulation to pan
 * @param menuShowing True if the in-game menu is showing and the camera should focus above it
 * @return true if the camera moved, false if it didn't
 */
bool physAdjustCameraTimer(physSim_t* phys, bool menuShowing)
{
    bool cameraMoved = false;

    // If we're doing a camera tour
    if (phys->cameraTour.length)
    {
        vecFl_t* flTarget = phys->cameraTour.first->val;
        vec_t target      = {
                 .x = flTarget->x - (TFT_WIDTH / 2),
                 .y = flTarget->y - (TFT_HEIGHT / 2),
        };

        // Move camera a fraction of the way to desired camera
        phys->camera = addVec2d(phys->camera, divVec2d(subVec2d(target, phys->camera), 48));

        // If the camera is close enough to the destination
        if (ABS(phys->camera.x - target.x) < 64 && //
            ABS(phys->camera.y - target.y) < 64)
        {
            // Remove it
            shift(&phys->cameraTour);
        }
    }
    // Else if there's no camera target
    else if (0 == phys->cameraTargets.length)
    {
        // move according to button input, vertically
        if (PB_UP & phys->cameraBtn)
        {
            phys->camera.y -= CAMERA_BTN_MOVE_INTERVAL;
            cameraMoved = true;
        }
        else if (PB_DOWN & phys->cameraBtn)
        {
            phys->camera.y += CAMERA_BTN_MOVE_INTERVAL;
            cameraMoved = true;
        }

        // Horizontally
        if (PB_LEFT & phys->cameraBtn)
        {
            phys->camera.x -= CAMERA_BTN_MOVE_INTERVAL;
            cameraMoved = true;
        }
        else if (PB_RIGHT & phys->cameraBtn)
        {
            phys->camera.x += CAMERA_BTN_MOVE_INTERVAL;
            cameraMoved = true;
        }
    }
    else
    {
        // view bounds
        vec_t vbStart = {
            .x = INT32_MAX,
            .y = INT32_MAX,
        };
        vec_t vbEnd = {
            .x = INT32_MIN,
            .y = INT32_MIN,
        };

        // Make a box containing all objects that should be on screen
        node_t* ctNode = phys->cameraTargets.first;
        while (ctNode)
        {
            vecFl_t ct = ((physCirc_t*)ctNode->val)->c.pos;

            // Find box mins
            if (ct.x < vbStart.x)
            {
                vbStart.x = ct.x;
            }
            if (ct.y < vbStart.y)
            {
                vbStart.y = ct.y;
            }

            // Find box maxs
            if (ct.x > vbEnd.x)
            {
                vbEnd.x = ct.x;
            }
            if (ct.y > vbEnd.y)
            {
                vbEnd.y = ct.y;
            }

            // Iterate
            ctNode = ctNode->next;
        }

        // Adjust viewbox to be inside the camera margin
        vbStart.x -= CAMERA_MARGIN;
        vbStart.y -= CAMERA_MARGIN;
        vbEnd.x -= (TFT_WIDTH - CAMERA_MARGIN);
        if (menuShowing)
        {
            vbEnd.y -= 110;
        }
        else
        {
            vbEnd.y -= (TFT_HEIGHT - CAMERA_MARGIN);
        }

        // Desired camera starts as the current camera
        vec_t desiredCamera = phys->camera;

        // Adjust the desired camera so that it contains the viewbox (X)
        if (desiredCamera.x > vbStart.x)
        {
            desiredCamera.x = vbStart.x;
        }
        else if (desiredCamera.x < vbEnd.x)
        {
            desiredCamera.x = vbEnd.x;
        }

        // Adjust the desired camera so that it contains the viewbox (Y)
        if (desiredCamera.y > vbStart.y)
        {
            desiredCamera.y = vbStart.y;
        }
        else if (desiredCamera.y < vbEnd.y)
        {
            desiredCamera.y = vbEnd.y;
        }

        // Move camera a third of the way to desired camera
        phys->camera = addVec2d(phys->camera, divVec2d(subVec2d(desiredCamera, phys->camera), 3));
    }

    return cameraMoved;
}
