//==============================================================================
// Includes
//==============================================================================

#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>

#include "ext_motion.h"
#include "emu_ext.h"
#include "emu_args.h"
#include "esp_random.h"
#include "hdw-imu_emu.h"
#include "macros.h"
#include "trigonometry.h"
#include "rawdraw_sf.h"

//==============================================================================
// Macros
//==============================================================================

#define ONE_G 242

/**
 * @brief Macro for generating vertices for a polygon approximating a circle with `tris` sides
 *
 * @param buf  A buffer of RDPoint[] with at least `tris` of space
 * @param tris The number of triangles to use to approximate a circle. Best if `(360 % tris) == 0`
 * @param xo   The X-offset of the center of the circle
 * @param yo   The Y-offset of the center of the circle
 * @param r    The radius of the circle
 */
#define CALC_CIRCLE_POLY(buf, tris, xo, yo, r)                           \
    do                                                                   \
    {                                                                    \
        for (int i = 0; i < (tris); i++)                                 \
        {                                                                \
            buf[i].x = (xo) + getCos1024(i * 360 / (tris)) * (r) / 1024; \
            buf[i].y = (yo) + getSin1024(i * 360 / (tris)) * (r) / 1024; \
        }                                                                \
    } while (false)

/**
 * @brief The threshold for whether an indicator should still be visible, despite being on the other side of the sphere.
 *
 * This is to take into account the 3D-ness of the sphere
 */
#define ROT_THRESH 10

// Some macros to check whether the sine or cosine of a given angle are negative or positive, within a threshold of
// ROT_THRESH This threshold means that SIN_NEG(x) != !SIN_POS(x) for some values of x, so keep that in mind.
#define COS_POS(rot) (rot <= (180 + ROT_THRESH) || rot >= (360 - ROT_THRESH))
#define COS_NEG(rot) (rot <= (0 + ROT_THRESH) || rot >= (180 - ROT_THRESH))
#define SIN_POS(rot) (rot <= (90 + ROT_THRESH) || rot >= (270 - ROT_THRESH))
#define SIN_NEG(rot) (rot >= (90 - ROT_THRESH) && rot <= (270 + ROT_THRESH))

//==============================================================================
// Function Prototypes
//==============================================================================

static bool motionInit(emuArgs_t* emuArgs);
static void motionPreFrame(uint64_t frame);
static bool motionHandleMove(int32_t x, int32_t y, mouseButton_t buttonMask);
static bool motionHandleButton(int32_t x, int32_t y, mouseButton_t button, bool down);
static void motionRender(uint32_t winW, uint32_t winH, const emuPane_t* panes, uint8_t numPanes);

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief Stores all the state needed for motion emulation and its UI
 *
 */
typedef struct
{
    emuArgs_t* emuArgs; ///< Reference to the emulator command-line args

    // Window pane bounds
    uint32_t paneX; ///< The X offset of the left of the pane
    uint32_t paneY; ///< The Y offset of the top of the pane
    uint32_t paneW; ///< The width of the UI pane
    uint32_t paneH; ///< The height of the UI pane

    // Mouse drag state
    bool mouseDragging;      ///< Whether the left mouse button is being held
    int32_t dragStartX;      ///< The X coordinate where the mouse drag started
    int32_t dragStartY;      ///< The Y coordinate where the mouse drag started
    uint16_t dragYawStart;   ///< The yaw value when the mouse drag started
    uint16_t dragPitchStart; ///< The pitch value when the mouse drag started
    uint16_t dragRollStart;  ///< The roll value when the mouse drag started

    // Last simulated motion control state
    uint16_t mag;   ///< The magnitude of the rotation
    uint16_t yaw;   ///< CCW rotation about z-axis, in degrees [0, 360)
    uint16_t pitch; ///< CCW rotation about x-axis, in degrees [0, 360)
    uint16_t roll;  ///< CCW rotation about y-axis, in degrees [0, 360)
} emuMotion_t;

//==============================================================================
// Variables
//==============================================================================

emuExtension_t motionEmuExtension = {
    .name            = "motion",
    .fnInitCb        = motionInit,
    .fnPreFrameCb    = motionPreFrame,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = NULL,
    .fnMouseMoveCb   = motionHandleMove,
    .fnMouseButtonCb = motionHandleButton,
    .fnRenderCb      = motionRender,
};

static emuMotion_t motion = {0};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Stores a reference to emuArgs and initializes the motion system
 *
 */
static bool motionInit(emuArgs_t* emuArgs)
{
    motion.emuArgs = emuArgs;

    motion.mag   = 256;
    motion.yaw   = 0;
    motion.pitch = 0;
    motion.roll  = 0;

    if (emuArgs->emulateMotion)
    {
        requestPane(&motionEmuExtension, PANE_BOTTOM, 128, 128);
    }

    return emuArgs->emulateMotion;
}

/**
 * @brief Updates the emulator's accelerometer readings for the next frame
 *
 * @param frame The next frame number
 */
void motionPreFrame(uint64_t frame)
{
    if (motion.emuArgs->emulateMotion)
    {
        if (motion.emuArgs->motionDrift)
        {
            // Have a 1-in-8 chance each of increasing or decreasing the pitch
            switch (esp_random() % 8)
            {
                case 0:
                    motion.pitch = (motion.pitch + 1) % 360;
                    break;

                case 1:
                    motion.pitch = (motion.pitch + 359) % 360;
                    break;

                default:
                    break;
            }

            // Have a 1-in-8 chance each of increasing or decreasing the yaw
            switch (esp_random() % 8)
            {
                case 0:
                    motion.yaw = (motion.yaw + 1) % 360;
                    break;

                case 1:
                    motion.yaw = (motion.yaw + 359) % 360;
                    break;

                default:
                    break;
            }

            // Have a 1-in-8 chance each of increasing or decreasing the roll
            switch (esp_random() % 8)
            {
                case 0:
                    motion.roll = (motion.roll + 1) % 360;
                    break;

                case 1:
                    motion.roll = (motion.roll + 359) % 360;
                    break;

                default:
                    break;
            }

            emulatorSetAccelerometerRotation(ONE_G, motion.yaw, motion.pitch, motion.roll);
        }

        if (motion.emuArgs->motionJitter)
        {
// Add jitter by updating the calculated x, y, z accelerometer vector by a random amount
// This keeps the "true" value kept by this extension consistent while randomizing the real value
// (esp_random() % (emulatorArgs.motionJitterAmount * 2 + 1) - emulatorArgs.motionJitterAmount - 1);
#define RAND_IRANGE(min, max) (esp_random() % (max + 1 - min) + (min))
#define RAND_JITTER           RAND_IRANGE(-motion.emuArgs->motionJitterAmount, motion.emuArgs->motionJitterAmount)

            // We can't use esp_random() with CLAMP() directly, so use a temporary variable here
            int16_t jitterX = RAND_JITTER;
            int16_t jitterY = RAND_JITTER;
            int16_t jitterZ = RAND_JITTER;

#undef RAND_IRANGE
#undef RAND_JITTER

            int16_t x, y, z;
            // Get the raw accelerometer vector
            emulatorGetAccelerometer(&x, &y, &z);

            int16_t min, max;
            // Get the range of the raw accelerometer readings
            emulatorGetAccelerometerRange(&min, &max);

            // Calculate the raw accelerometer readings +/- jitter, clamped to their limits
            x = CLAMP(x + jitterX, min, max);
            y = CLAMP(y + jitterY, min, max);
            z = CLAMP(z + jitterZ, min, max);

            // Update the raw accelerometer reading with our new values
            emulatorSetAccelerometer(x, y, z);
        }
    }
}

/**
 * @brief Handle mouse button events to change the simulated motion direction.
 *
 * @param x The X-position of the mouse when the button was pressed or released
 * @param y The Y-position of the mouse when the button was pressed or released
 * @param button The button pressed or relesaed
 * @param down true if the button was released,
 * @return true if the event was consumed by the motion system
 * @return false if the event was not consumed by the motion system
 */
static bool motionHandleButton(int32_t x, int32_t y, mouseButton_t button, bool down)
{
    if (motion.emuArgs->emulateMotion)
    {
        switch (button)
        {
            case EMU_MOUSE_LEFT:
            case EMU_MOUSE_RIGHT:
            {
                // We accept both left and right for dragging
                // It doesn't matter for this part, but left click will be pitch/roll, with right-click being pitch/yaw
                if (down && !motion.mouseDragging)
                {
                    // We're not already dragging, and this is a left-click down
                    // We only want to capture the drag start if there's no pane, or if the click was inside the pane's
                    // bounds
                    if ((motion.paneW == 0 || motion.paneH == 0)
                        || (motion.paneX <= x && x <= (motion.paneX + motion.paneW) && motion.paneY <= y
                            && y <= (motion.paneY + motion.paneH)))
                    {
                        motion.mouseDragging = true;

                        motion.dragStartX = x;
                        motion.dragStartY = y;

                        motion.dragYawStart   = motion.yaw;
                        motion.dragPitchStart = motion.pitch;
                        motion.dragRollStart  = motion.roll;

                        // Consume the event
                        return true;
                    }
                }
                else if (!down && motion.mouseDragging)
                {
                    // We're currently dragging, and this is a left-click release
                    // Just reset the dragging flag and we're good
                    // No need to check the bounds because the drag may continue outside our pane once it starts
                    motion.mouseDragging = false;

                    // Consume the event
                    return true;
                }
                break;
            }

            case EMU_MOUSE_MIDDLE:
            {
                // Middle-click
                // We handle this on the release because maybe it overlaps with tracpoint scrolling otherwise?
                if (!down)
                {
                    if (motion.mouseDragging)
                    {
                        // If we're already dragging and middle-clicked, cancel the drag and return to the starting
                        // position
                        motion.mouseDragging = false;
                        motion.yaw           = motion.dragYawStart;
                        motion.pitch         = motion.dragPitchStart;
                        motion.roll          = motion.dragRollStart;
                        emulatorSetAccelerometerRotation(ONE_G, motion.yaw, motion.pitch, motion.roll);

                        // Consume the event
                        return true;
                    }
                    else if ((motion.paneW == 0 || motion.paneH == 0)
                             || (motion.paneX <= x && x <= (motion.paneX + motion.paneW) && motion.paneY <= y
                                 && y <= (motion.paneY + motion.paneH)))
                    {
                        // If we're not dragging and the middle-click was inside our area, just reset all the axes to
                        // zero
                        motion.yaw   = 0;
                        motion.pitch = 0;
                        motion.roll  = 0;
                        emulatorSetAccelerometerRotation(ONE_G, motion.yaw, motion.pitch, motion.roll);

                        // Consume the event
                        return true;
                    }
                }
                break;
            }

            case EMU_SCROLL_UP:
            {
                // Roll the ball backwards on scrolling up
                if (down)
                {
                    motion.pitch = (motion.pitch + 1) % 360;
                    emulatorSetAccelerometerRotation(ONE_G, motion.yaw, motion.pitch, motion.roll);
                    return true;
                }
                break;
            }

            case EMU_SCROLL_DOWN:
            {
                // Roll the ball forwards on scrolling down
                if (down)
                {
                    motion.pitch = (motion.pitch + 359) % 360;
                    emulatorSetAccelerometerRotation(ONE_G, motion.yaw, motion.pitch, motion.roll);
                    return true;
                }
                break;
            }

            case EMU_SCROLL_LEFT:
            {
                // Spin the ball clockwise on scrolling left
                if (down)
                {
                    // TODO: [roll the ball counterclockwise]
                    // motion.roll = (motion.roll + 1) % 360;
                    motion.yaw = (motion.yaw + 359) % 360;
                    emulatorSetAccelerometerRotation(ONE_G, motion.yaw, motion.pitch, motion.roll);
                    return true;
                }
                break;
            }

            case EMU_SCROLL_RIGHT:
            {
                // Spin the ball counterclockwise on scrolling right
                if (down)
                {
                    // TODO (): [roll the boll clockwise]
                    // motion.roll = (motion.roll + 359) % 360;
                    motion.yaw = (motion.yaw + 1) % 360;
                    emulatorSetAccelerometerRotation(ONE_G, motion.yaw, motion.pitch, motion.roll);
                    return true;
                }
                break;
            }

            default:
                break;
        }
    }

    return false;
}

/**
 * @brief Handle the mouse move event and update the motion angles in the event of a drag
 *
 * @param x The new mouse X location
 * @param y The new mouse Y location
 * @param buttonMask The buttons being held during the move
 * @return true  If the event was consumed and the motion angles updated
 * @return false If the event was not consumed and no action was taken
 */
static bool motionHandleMove(int32_t x, int32_t y, mouseButton_t buttonMask)
{
    // We handle both a left-click and a right-click drag mostly the same here, so check for either
    if (motion.mouseDragging && (buttonMask & (EMU_MOUSE_LEFT | EMU_MOUSE_RIGHT)))
    {
        int diffX = (x - motion.dragStartX);
        int diffY = (y - motion.dragStartY);

        // Consider the mouse to be not moved in either direction if the difference is less than 5 pixels
        // This will make it much easier to test rotation in only a single direction

        if (diffX < 5 && diffX > -5)
        {
            diffX = 0;
        }

        if (diffY < 5 && diffY > -5)
        {
            diffY = 0;
        }

        // To calculate the angle change, we just convert the distance in pixels directly to degrees

        // The Y distance determines the pitch change for both left-and-right-click drags
        motion.pitch = POS_MODULO_ADD(motion.dragPitchStart, diffY, 360);

        // For left-click drag, the X distance will determine the roll change
        // For right-click drag, the X distance will determine the yaw change
        if (buttonMask & EMU_MOUSE_LEFT)
        {
            motion.roll = POS_MODULO_ADD(motion.dragRollStart, diffX, 360);
        }
        else if (buttonMask & EMU_MOUSE_RIGHT)
        {
            motion.yaw = POS_MODULO_ADD(motion.dragYawStart, diffX, 360);
        }

        // Apply the updated rotations
        emulatorSetAccelerometerRotation(motion.mag, motion.yaw, motion.pitch, motion.roll);

        // Consume the event
        return true;
    }

    // By default, do nothing and don't consume the event
    return false;
}

/**
 * @brief Render the motion control indicator hat inside the pane, if there is one
 *
 * The X axis lies within the plane of the swadge's circuit board, parallel with the line
 * passing through the Select and Start buttons, with +X being towards the A and B buttons
 * on the right and -X being towards the D-pad to the left.
 * The Y axis lies within the plane of the swadge's circuit board, perpendicular to the
 * X axis, with +Y being towards the lanyard hole at the top of the swadge, and with -Y
 * being towards the microphone at the bottom of the swadge.
 * The Z axis is perpendicular to the plane of the swadge's circuit board, with +Z being
 * on the front side, and -Z being the back side.
 *
 * Imagine this diagram with a swadge instead of a phone: https://developer.android.com/images/axis_device.png
 *
 * For pitch, roll, and yaw, imagine the swadge as an airplane where +Y (lanyard-hole-wards) is the nose.
 * https://upload.wikimedia.org/wikipedia/commons/c/c1/Yaw_Axis_Corrected.svg
 *
 * That makes X the pitch axis, Y the roll axis, and Z the yaw axis, and let's say always clockwise.
 *
 * @param winW The overall window width
 * @param winH The overall window height
 * @param pane A pointer to the pane to draw in
 * @param numPanes The number of items in \c pane, or 0 if no pane is assigned.
 */
static void motionRender(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes)
{
    /*
    plotMotionControl(MOTION_CONTROL_WIDTH, MOTION_CONTROL_HEIGHT, (window_w - MOTION_CONTROL_WIDTH) / 2,
                          window_h - MOTION_CONTROL_HEIGHT,
                          emulatorArgs.motionZ % 360, emulatorArgs.motionX % 360, 0);
                          --> motionRender()
    */
   if (numPanes == 0)
   {
        return;
   }

    // Save the current dimensions for later
    uint32_t paneW = motion.paneW = pane->paneW;
    uint32_t paneH = motion.paneH = pane->paneH;
    uint32_t paneX = motion.paneX = pane->paneX;
    uint32_t paneY = motion.paneY = pane->paneY;

    // Don't actually render if we have no space
    if (paneW == 0 || paneH == 0)
    {
        return;
    }

    // TODO: support roll, oops, turns out that's actually necessary

    // Have a 5px margin on either side of the circle
    int16_t circleR = (MIN(paneW, paneH) - 10) / 2;
    int16_t centerX = paneX + (paneW / 2);
    int16_t centerY = paneY + (paneH / 2);

    CNFGColor(0xCCCCCCFF);
    CNFGTackRectangle(paneX, paneY, paneX + paneW, paneY + paneH);

    RDPoint circle[72];
    CALC_CIRCLE_POLY(circle, 72, centerX, centerY, circleR);

    CNFGColor(0xFFFFFFFF);
    CNFGTackPoly(circle, 72);

// The Z axis, which by default would be just a dot in the middle of the circle, is pushed from the center towards
// the bottom with a +X rotation, and towards the top with a -X rotation Then, with a Z rotation, the position of
// the Z dot simply follows the rotation So, to draw the +Z dot...
// 1. Get the distance from the middle of the circle, which is based on the X rotation so zPosRadius = (h/2) *
// sin(xRotation) / 1024
// 2. Get the X and Y from the rotation of that, which is zPosX = (x + h/2) + zPosRadius * cos(zRotation) / 1024,
// zPosY = zPosradius * sin(zRotation) / 1024
// 3. Note that these values are for when xRotation <= 90 || xRotation >= 270; for the other cases, invert and swap
// cos and sin, making zPos = (x + w / 2) - zPosRadius * sin(zRotation) / 1024, zPosY = (y + h / 2) - zPosRadius *
// cos(zRotation) / 1024

    // rot Z: flat rotation, +Z screen rotates clockwise, -Z screen rotates counterclockwise
    // rot X: tilt, +X towards viewer, -X away from viewer

    // Z axis: up/down
    // X axis: east/west
    // Y axis: north/south

    // The top-down offset of the Z-axis from its center position
    //

    //===========
    //    Z
    //===========
    // First: Calculate the Z axis radius -- that is, the projection of the distance from the center of the drawing
    // upon which the ends of the Z axis will emerge
    int16_t zRadius = circleR * getSin1024(motion.pitch) / 1024;

    // So now we know the distance from the center to draw the Z axes
    // Next we just calculate the rotation that's applied to the Z axes
    // and we calculate the position for both the +Z and -Z points.
    // These are the same locations, just rotated 180 degrees about Z
    int16_t zPosOffX = zRadius * getCos1024((motion.yaw + 270) % 360) / 1024;
    int16_t zPosOffY = -zRadius * getSin1024((motion.yaw + 270) % 360) / 1024;
    int16_t zNegOffX = zRadius * -getSin1024((motion.yaw + 90) % 360) / 1024;
    int16_t zNegOffY = -zRadius * -getCos1024((motion.yaw + 90) % 360) / 1024;

    // When sin(xRotation) > 0, Z+ is visible, and otherwise Z- is visible
    bool zPosVisible = SIN_POS(motion.pitch);
    bool zNegVisible = SIN_NEG(motion.pitch);

    int16_t zPosX     = centerX + zPosOffX;
    int16_t zPosY     = centerY + zPosOffY;
    int16_t zNegX     = centerX + zNegOffY;
    int16_t zNegY     = centerY + zNegOffX;
    int16_t zNegXPlus = centerX + zNegOffY * 12 / 10;
    int16_t zNegYPlus = centerY + zNegOffX * 12 / 10;

    //===========
    //    X
    //===========
    // X: This is the east/west axis
    // literally not affected at all by X rotation?
    int16_t xPosRadius = circleR * getCos1024(motion.pitch) / 1024;
    int16_t xPosOffX   = xPosRadius * getCos1024((motion.yaw + 270) % 360) / 1024;
    int16_t xPosOffY   = -xPosRadius * getSin1024((motion.yaw + 270) % 360) / 1024;
    int16_t xNegOffX   = xPosRadius * getCos1024((motion.yaw + 90) % 360) / 1024;
    int16_t xNegOffY   = -xPosRadius * getSin1024((motion.yaw + 90) % 360) / 1024;

    bool xPosVisible = COS_NEG(motion.pitch);
    bool xNegVisible = COS_POS(motion.pitch);

    int16_t xPosX     = centerX + xPosOffX;
    int16_t xPosY     = centerY + xPosOffY;
    int16_t xNegX     = centerX + xNegOffX;
    int16_t xNegY     = centerY + xNegOffY;
    int16_t xNegXPlus = centerX + xNegOffX * 12 / 10;
    int16_t xNegYPlus = centerY + xNegOffY * 12 / 10;

    //===========
    //    Y
    //===========
    // Y: This is the north/south axis (screen up/down)
    // When Z is facing out and rotates, Y rotates around the perimeter of the circle, the same way as X
    // But it's 90 ahead of X
    // Then, rotating X causes the radius of Y to decrease?
    // When there is only a Z-rotation applied, the radius does not change, only the direction
    // When there is only an X-rotation applied, the radius does not change, nor does the direction
    // When there is a Z-rotation applied, and then an X-rotation... the radius never changes, lol
    int16_t yPosRadius = circleR;
    int16_t yPosOffX   = yPosRadius * getCos1024(motion.yaw) / 1024;
    int16_t yPosOffY   = -yPosRadius * getSin1024(motion.yaw) / 1024;

    int16_t yNegOffX = yPosRadius * getCos1024((motion.yaw + 180) % 360) / 1024;
    int16_t yNegOffY = -yPosRadius * getSin1024((motion.yaw + 180) % 360) / 1024;

    bool yPosVisible = true;
    bool yNegVisible = true;

    int16_t yPosX     = centerX + yPosOffX;
    int16_t yPosY     = centerY + yPosOffY;
    int16_t yNegX     = centerX + yNegOffX;
    int16_t yNegY     = centerY + yNegOffY;
    int16_t yNegXPlus = centerX + yNegOffX * 12 / 10;
    int16_t yNegYPlus = centerY + yNegOffY * 12 / 10;

    // Draw smaller circle with less precision
    // +X axis (EAST pole)
    if (xPosVisible)
    {
        CALC_CIRCLE_POLY(circle, 4, xPosX, xPosY, 3);
        CNFGColor(0xFF0000FF);
        CNFGTackPoly(circle, 4);
        CNFGTackPixel(xPosX, xPosY);
    }

    // -X axis (WEST pole)
    if (xNegVisible)
    {
        CNFGColor(0xCC0000FF);
        CNFGTackSegment(xNegX, xNegY, xNegXPlus, xNegYPlus);
    }

    // +Y axis (NORTH pole)
    if (yPosVisible)
    {
        CALC_CIRCLE_POLY(circle, 4, yPosX, yPosY, 3);
        CNFGColor(0x00FF00FF);
        CNFGTackPoly(circle, 4);
        CNFGTackPixel(yPosX, yPosY);
    }

    // -Y axis (SOUTH pole)
    if (yNegVisible)
    {
        CNFGColor(0x00FF00FF);
        CNFGTackSegment(yNegX, yNegY, yNegXPlus, yNegYPlus);
    }

    // +Z axis (UP pole)
    if (zPosVisible)
    {
        CALC_CIRCLE_POLY(circle, 4, zPosX, zPosY, 3);
        CNFGColor(0x0000FFFF);
        CNFGTackPoly(circle, 4);
        CNFGTackPixel(zPosX, zPosY);
    }

    // -Z axis (DOWN pole)
    if (zNegVisible)
    {
        CNFGColor(0x0000CCFF);
        CNFGTackSegment(zNegX, zNegY, zNegXPlus, zNegYPlus);
    }
}
