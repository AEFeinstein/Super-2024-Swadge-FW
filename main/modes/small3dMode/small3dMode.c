/**
 * @file small3dMode.c
 * @author dylwhich (dylan@whichard.com)
 * @brief A swadge version of the small3dlib "level.c" demo
 * @date 2024-02-28
 */

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"
#include "small3dMode.h"
#include "hdw-imu.h"
#include "esp_log.h"
#include "trigonometry.h"
#include "quaternions.h"
#include "matrixMath.h"
#include "shapes.h"
#include "fill.h"
#include "linked_list.h"
#include "font.h"
#include "model.h"
#include "spiffs_model.h"
#include "buttonUtils.h"
#include "macros.h"

/// @brief The struct that holds all the state for the small3dlib test mode
typedef struct
{
    font_t ibm; ///< The font used to display text
    int64_t avgFrameTime;
    int64_t nextDebug;

    int textureCount;
    wsg_t textures[3];

    object3dInfo_t levelModelData;

    S3L_Scene scene; ///< The 3D scene for the renderer
    S3L_Model3D levelModel;
    S3L_Vec4 teleportPoint;
    uint32_t previousTriangle;
    S3L_Vec4 uv0, uv1, uv2;

    uint16_t btnState;               ///< The button state
    buttonRepeatState_t repeatState; ///< The button repeat state

    bool touchState;
    int32_t touchDragStartX;
    int32_t touchDragStartY;
    S3L_Vec4 touchDragStartOrient;
    touchSpinState_t touchSpinState;
    float touchSpinStartOrient[4];
} small3dMode_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void small3dModeMainLoop(int64_t elapsedUs);
static void small3dModeEnterMode(void);
static void small3dModeExitMode(void);

static void small3dModeHandleInput(void);
static void small3dModeDrawScene(void);

static void small3dModeBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void drawTeleport(int16_t x, int16_t y, S3L_ScreenCoord size);
static paletteColor_t sampleTexture(const wsg_t* tex, int32_t u, int32_t v);
static void small3dModeDrawPixelCb(const S3L_PixelInfo* pixelInfo);

//==============================================================================
// Strings
//==============================================================================

static const char small3dModeName[] = "small3dlib Demo";

//==============================================================================
// Variables
//==============================================================================

#define TEMP_TEXT_TIME (10 * 1000000)

/// The Swadge mode for small3dMode
swadgeMode_t small3dMode = {
    .modeName                 = small3dModeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = small3dModeEnterMode,
    .fnExitMode               = small3dModeExitMode,
    .fnMainLoop               = small3dModeMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = small3dModeBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

/// All state information for the Graphics Test mode.
small3dMode_t* s3dLevel = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter Graphics Test mode, allocate required memory, and initialize required variables
 *
 */
static void small3dModeEnterMode(void)
{
    // Allocate and clear all memory for this mode. All the variables are contained in a single struct for convenience.
    // calloc() is used instead of malloc() because calloc() also initializes the allocated memory to zeros.
    s3dLevel = calloc(1, sizeof(small3dMode_t));

    // Load a font
    loadFont("ibm_vga8.font", &s3dLevel->ibm, false);

    // Load the textures
    s3dLevel->textureCount = 3;
    loadWsg("level1_texture.wsg", &s3dLevel->textures[0], true);
    loadWsg("level2_texture.wsg", &s3dLevel->textures[1], true);
    loadWsg("level3_texture.wsg", &s3dLevel->textures[2], true);

    loadObjInfo("levelModel.mdl", &s3dLevel->levelModelData, true);
    S3L_model3DInit(s3dLevel->levelModelData.verts, s3dLevel->levelModelData.vertCount, s3dLevel->levelModelData.tris,
                    s3dLevel->levelModelData.triCount, &s3dLevel->levelModel);

    S3L_sceneInit(&s3dLevel->levelModel, 1, &s3dLevel->scene);

    // Repeat any of the arrow buttons every .1s after first holding for .3s
    s3dLevel->repeatState.repeatMask     = PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT;
    s3dLevel->repeatState.repeatDelay    = 300000;
    s3dLevel->repeatState.repeatInterval = 60000;

    configureS3dCallback(TFT_WIDTH, TFT_HEIGHT, small3dModeDrawPixelCb);

    s3dLevel->teleportPoint.x = 6 * S3L_F;
    s3dLevel->teleportPoint.y = -3 * S3L_F;
    s3dLevel->teleportPoint.z = 3 * S3L_F / 2;
    s3dLevel->teleportPoint.w = S3L_F;

    // We shold go as fast as we can.
    setFrameRateUs(0);
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void small3dModeExitMode(void)
{
    // Free renderer memory
    // deinitRenderer();

    // Free the 3D models
    freeObjInfo(&s3dLevel->levelModelData);

    for (int i = 0; i < s3dLevel->textureCount; i++)
    {
        freeWsg(&s3dLevel->textures[i]);
    }

    // Free the font
    freeFont(&s3dLevel->ibm);
    free(s3dLevel);
}

/**
 * @brief This function is called periodically and frequently. It will either read inpust and draw the screen.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void small3dModeMainLoop(int64_t elapsedUs)
{
    small3dModeDrawScene();

    // Do update each loop
    small3dModeHandleInput();

    // Draw the models

    char buffer[32];

    // Draw FPS Counter
    s3dLevel->avgFrameTime = (s3dLevel->avgFrameTime * 9 / 10) + (elapsedUs * 1 / 10);

    int32_t framesPerSecond      = (1000000 / (s3dLevel->avgFrameTime + 1));
    int32_t tenthFramesPerSecond = (10000000 / (s3dLevel->avgFrameTime + 1)) % 10;

    snprintf(buffer, sizeof(buffer), "%" PRId32 ".%" PRId32, framesPerSecond, tenthFramesPerSecond);
    drawText(&s3dLevel->ibm, c000, buffer, TFT_WIDTH - 30 - textWidth(&s3dLevel->ibm, buffer) + 1, 6);
    int16_t textX = drawText(&s3dLevel->ibm, c550, buffer, TFT_WIDTH - 30 - textWidth(&s3dLevel->ibm, buffer), 5);

    if (s3dLevel->nextDebug > elapsedUs)
    {
        s3dLevel->nextDebug -= elapsedUs;
    }
    else if (s3dLevel->nextDebug <= elapsedUs)
    {
        S3L_logTransform3D(s3dLevel->scene.camera.transform);
        s3dLevel->nextDebug = 1000000;
    }
}

/**
 * @brief Perform the accelerometer reading and record the sample
 */
static void small3dModeHandleInput(void)
{
    // Vectors for movement relative to camera
    S3L_Vec4 camF, camR;
    S3L_rotationToDirections(s3dLevel->scene.camera.transform.rotation, 3200, &camF, &camR, NULL);

    // Handle Touchpad Input
    int32_t angle, radius, intensity;
    if (getTouchJoystick(&angle, &radius, &intensity))
    {
        // Handle Translate Modes
        int32_t x, y;
        getTouchCartesian(angle, radius, &x, &y);

        if (!s3dLevel->touchState)
        {
            // Touch start!
            s3dLevel->touchState      = true;
            s3dLevel->touchDragStartX = x;
            s3dLevel->touchDragStartY = y;
            memcpy(&s3dLevel->touchDragStartOrient, &s3dLevel->scene.camera.transform.rotation, sizeof(S3L_Vec4));
        }

        S3L_Unit minX = -S3L_F / 2;
        S3L_Unit maxX = S3L_F / 2;
        S3L_Unit minY = -S3L_F / 2;
        S3L_Unit maxY = S3L_F / 2;

        // LOOK LEFT: Y += 1
        // LOOK RIGHT: Y -= 1
        // LOOK UP: X -= 1
        // LOOK DOWN: X += 1
        s3dLevel->scene.camera.transform.rotation.y
            = s3dLevel->touchDragStartOrient.y - ((maxY - minY) * (x - s3dLevel->touchDragStartX) / 1023);
        s3dLevel->scene.camera.transform.rotation.x
            = s3dLevel->touchDragStartOrient.x - ((maxX - minX) * (s3dLevel->touchDragStartY - y) / 1023);

        /*
        // Rotation Actions
        getTouchSpins(&graphicsTest->touchSpinState, angle, radius);

        if (!graphicsTest->touchState)
        {
            // Save the selected model's orientation on the first touch
            graphicsTest->touchState = true;
            memcpy(graphicsTest->touchSpinStartOrient, curTransform->rotate, sizeof(float[4]));
        }

        // Apply the rotation to the saved orientation
        float eulerRot[3] = {0};
        float newRot[4];

        // Construct an euler angle vector for the single axis we are rotating
        float rot = graphicsTest->touchSpinState.remainder * 3.1415926535 / 180;
        eulerRot[graphicsTest->touchAction - ROTATE_X] = rot;

        // Convert euler angle to quaternion
        mathEulerToQuat(newRot, eulerRot);

        ESP_LOGI("GraphicsTest", "Rotating by %.1f, %.1f, %.1f", eulerRot[0], eulerRot[1], eulerRot[2]);

        // Update the object's orient to its original orient rotated by the new rotation
        mathQuatApply(curTransform->rotate, graphicsTest->touchSpinStartOrient, newRot);

        rotateUpdated = true;
        */
    }
    else if (s3dLevel->touchState)
    {
        s3dLevel->touchState              = false;
        s3dLevel->touchSpinState.startSet = false;
    }

    // Handle Button Presses
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueRepeat(&s3dLevel->repeatState, &evt))
    {
        // Save the button state
        s3dLevel->btnState = evt.state;
    }

    if (s3dLevel->btnState & PB_UP)
    {
        S3L_vec3Add(&s3dLevel->scene.camera.transform.translation, camF);
    }
    else if (s3dLevel->btnState & PB_DOWN)
    {
        S3L_vec3Sub(&s3dLevel->scene.camera.transform.translation, camF);
    }

    if (s3dLevel->btnState & PB_LEFT)
    {
        S3L_vec3Sub(&s3dLevel->scene.camera.transform.translation, camR);
    }
    else if (s3dLevel->btnState & PB_RIGHT)
    {
        S3L_vec3Add(&s3dLevel->scene.camera.transform.translation, camR);
    }
}

/**
 * @brief Draw the bunny
 */
static void small3dModeDrawScene(void)
{
    S3L_newFrame();

    S3L_drawScene(s3dLevel->scene);

    S3L_Vec4 screenPoint;

    S3L_project3DPointToScreen(s3dLevel->teleportPoint, s3dLevel->scene.camera, &screenPoint);

    if (screenPoint.w > 0 && screenPoint.x >= 0 && screenPoint.x < S3L_RESOLUTION_X && screenPoint.y >= 0
        && screenPoint.y < S3L_RESOLUTION_Y && screenPoint.z < S3L_zBufferRead(screenPoint.x, screenPoint.y))
        drawTeleport(screenPoint.x, screenPoint.y, screenPoint.w);
}

static void drawTeleport(int16_t x, int16_t y, S3L_ScreenCoord size)
{
    int16_t halfSize = size / 2;

    S3L_ScreenCoord x0 = S3L_max(0, x - halfSize);
    S3L_ScreenCoord x1 = S3L_min(TFT_WIDTH, x + halfSize);
    S3L_ScreenCoord y0 = S3L_max(0, y - halfSize);
    S3L_ScreenCoord y1 = S3L_min(TFT_HEIGHT, y + halfSize);

    S3L_ScreenCoord row = y0 - (y - halfSize);

    for (S3L_ScreenCoord j = y0; j < y1; ++j)
    {
        S3L_ScreenCoord i0, i1;

        if (row <= halfSize)
        {
            i0 = S3L_max(x0, x - row);
            i1 = S3L_min(x1, x + row);
        }
        else
        {
            i0 = S3L_max(x0, x - size + row);
            i1 = S3L_min(x1, x + size - row);
        }

        for (S3L_ScreenCoord i = i0; i < i1; ++i)
        {
            if (rand() % 8 == 0)
            {
                setPxTft(i, j, c500);
            }
        }

        row++;
    }
}

/**
 * This function is called when the display driver wishes to update a
 * section of the display.
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void small3dModeBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    accelIntegrate();
    fillDisplayArea(x, y, x + w, y + h, c000);
}

static paletteColor_t sampleTexture(const wsg_t* tex, int32_t u, int32_t v)
{
    u = S3L_wrap(u, tex->w);
    v = S3L_wrap(v, tex->h);

    return tex->px[v * tex->w + u];
}

static void small3dModeDrawPixelCb(const S3L_PixelInfo* pixelInfo)
{
    paletteColor_t col;

    static wsg_t* texture = NULL;

    // TEXTURES:
    if (!texture || pixelInfo->triangleID != s3dLevel->previousTriangle)
    {
        uint8_t material = s3dLevel->levelModelData.triMtls[pixelInfo->triangleIndex];

        texture = &s3dLevel->textures[material];

        S3L_getIndexedTriangleValues(pixelInfo->triangleIndex, s3dLevel->levelModelData.triUvs,
                                     s3dLevel->levelModelData.uvs, 2, &s3dLevel->uv0, &s3dLevel->uv1, &s3dLevel->uv2);
        s3dLevel->previousTriangle = pixelInfo->triangleID;
    }

    S3L_Unit u, v;

    u = S3L_interpolateBarycentric(s3dLevel->uv0.x, s3dLevel->uv1.x, s3dLevel->uv2.x, pixelInfo->barycentric) / 16;
    v = S3L_interpolateBarycentric(s3dLevel->uv0.y, s3dLevel->uv1.y, s3dLevel->uv2.y, pixelInfo->barycentric) / 16;
    col = sampleTexture(texture, u, v);
    // NO TEXTURES

     /*switch (material)
     {
       case 0: col = c500; break;
       case 1: col = c050; break;
       case 2:
       default: col = c005; break;
     }*/


#if FOG
    S3L_Unit fog = (p->depth *
    #if TEXTURES
                    8
    #else
                    16
    #endif
                    )
                   / S3L_F;

    r = S3L_clamp(((S3L_Unit)r) - fog, 0, 255);
    g = S3L_clamp(((S3L_Unit)g) - fog, 0, 255);
    b = S3L_clamp(((S3L_Unit)b) - fog, 0, 255);
#endif

    setPxTft(pixelInfo->x, pixelInfo->y, col);
}
