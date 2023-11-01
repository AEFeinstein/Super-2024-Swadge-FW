/**
 * @file graphicsTest.c
 * @author dylwhich (dylan@whichard.com)
 * @brief A test mode for the graphics API
 * @date 2023-08-02
 */

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"
#include "graphicsTest.h"
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
#include "menu.h"
#include "menuLogbookRenderer.h"
#include "macros.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    TRANSLATE_XY,
    TRANSLATE_ZY,
    ROTATE_X,
    ROTATE_Y,
    ROTATE_Z,
} touchAction_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    const char* label;
    const char* filename;
} testModelInfo_t;

static const testModelInfo_t graphicsTestModels[] =
{
    {"Donut",  "donut.mdl"},
    {"Funkus", "bigfunkus.mdl"},
    {"Bunny",  "bunny.mdl"},
    //{"Cube", "cube.mdl"},
    //{"Beetle", "beetle.mdl"},
    //{"Cow", "cow.mdl"},
};

/// @brief Struct defining separated translation, rotation, and scaling
typedef struct
{
    float translate[3];
    float rotate[4];
    float scale[3];
} transformBase_t;

/// @brief The struct that holds all the state for the graphics test mode
typedef struct
{
    font_t ibm; ///< The font used to display text
    int64_t avgFrameTime;

    menu_t* menu; ///< The menu for adding a model
    menuLogbookRenderer_t* menuRenderer; ///< The menu's renderer
    bool showMenu; ///< Whether or not the menu is active

    model_t models[ARRAY_SIZE(graphicsTestModels)];
    bool modelLoaded[ARRAY_SIZE(graphicsTestModels)];

    float worldTranslate[3];
    float worldOrient[4];
    float worldScale[3];

    transformBase_t worldTransform;
    transformBase_t objTransforms[SCENE_MAX_OBJECTS];

    scene_t scene; ///< The 3D scene for the renderer

    int8_t selectedModel; ///< The index of the selected model

    uint16_t btnState; ///< The button state
    buttonRepeatState_t repeatState; ///< The button repeat state

    bool touchState;
    touchAction_t touchAction;
    int32_t touchDragStartX;
    int32_t touchDragStartY;
    float touchDragStartPos[3];
    touchSpinState_t touchSpinState;
    float touchSpinStartOrient[4];
} graphicsTest_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void graphicsTestMainLoop(int64_t elapsedUs);
static void graphicsTestEnterMode(void);
static void graphicsTestExitMode(void);

static void graphicsTestMenuCb(const char* label, bool selected, uint32_t settingVal);

static void graphicsTestHandleInput(void);
static void graphicsTestDrawScene(void);
static void graphicsTestSetupMenu(void);
static void graphicsTestReset(void);

static const char* getTouchActionDesc(touchAction_t action);
static void transformBaseToMatrix(float mat[4][4], const transformBase_t* base);

static void graphicsTestBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

//==============================================================================
// Strings
//==============================================================================

static const char graphicsTestName[] = "Graphics Test";

static const char graphicsMenuItemCopy[] = "Copy";
static const char graphicsMenuItemReset[] = "Reset";
static const char graphicsMenuItemCancel[] = "Cancel";

static const char actionTranslateXYStr[] = "Move XY";
static const char actionTranslateZYStr[] = "Move ZY";
static const char actionRotateZStr[] = "Spin Z";
static const char actionRotateYStr[] = "Spin Y";
static const char actionRotateXStr[] = "Spin Z";

//==============================================================================
// Variables
//==============================================================================

/// The Swadge mode for graphicsTest
swadgeMode_t graphicsTestMode = {
    .modeName                 = graphicsTestName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = graphicsTestEnterMode,
    .fnExitMode               = graphicsTestExitMode,
    .fnMainLoop               = graphicsTestMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = graphicsTestBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

/// All state information for the Graphics Test mode.
graphicsTest_t* graphicsTest = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter Graphics Test mode, allocate required memory, and initialize required variables
 *
 */
static void graphicsTestEnterMode(void)
{
    // Allocate and clear all memory for this mode. All the variables are contained in a single struct for convenience.
    // calloc() is used instead of malloc() because calloc() also initializes the allocated memory to zeros.
    graphicsTest = calloc(1, sizeof(graphicsTest_t));

    // Load a font
    loadFont("ibm_vga8.font", &graphicsTest->ibm, false);

    // Init the menu
    graphicsTest->menu = initMenu("Add Object", graphicsTestMenuCb);
    graphicsTestSetupMenu();

    // and the menu renderer
    graphicsTest->menuRenderer = initMenuLogbookRenderer(&graphicsTest->ibm);

    // Load (some of) the 3D models!
    for (int i = 0; i < 3; i++)
    {
        ESP_LOGI("Model", "loadModel(%s) returned %s",
                 graphicsTestModels[i].filename,
                 loadModel(graphicsTestModels[i].filename, &graphicsTest->models[i], true) ? "true" : "false");
        graphicsTest->modelLoaded[i] = true;
    }

    graphicsTest->worldTransform.scale[0] = 1.0;
    graphicsTest->worldTransform.scale[1] = 1.0;
    graphicsTest->worldTransform.scale[2] = 1.0;
    graphicsTest->worldTransform.rotate[0] = 1.0;
    transformBaseToMatrix(graphicsTest->scene.transform, &graphicsTest->worldTransform);

    graphicsTest->scene.objectCount = 3;
    graphicsTest->scene.objects[0].model = &graphicsTest->models[0];
    graphicsTest->objTransforms[0].scale[0] = 1.0;
    graphicsTest->objTransforms[0].scale[1] = 1.0;
    graphicsTest->objTransforms[0].scale[2] = 1.0;
    graphicsTest->objTransforms[0].rotate[0] = 1.0;
    graphicsTest->objTransforms[0].translate[0] = -25.0;
    graphicsTest->objTransforms[0].translate[1] = -25.0;
    graphicsTest->objTransforms[0].translate[2] = 0.0;
    transformBaseToMatrix(graphicsTest->scene.objects[0].transform, &graphicsTest->objTransforms[0]);

    graphicsTest->scene.objects[1].model = &graphicsTest->models[1];
    graphicsTest->objTransforms[1].scale[0] = 1.0;
    graphicsTest->objTransforms[1].scale[1] = 1.0;
    graphicsTest->objTransforms[1].scale[2] = 1.0;
    graphicsTest->objTransforms[1].rotate[0] = 1.0;
    graphicsTest->objTransforms[1].translate[0] = 25.0;
    graphicsTest->objTransforms[1].translate[1] = 25.0;
    graphicsTest->objTransforms[1].translate[2] = 0.0;
    transformBaseToMatrix(graphicsTest->scene.objects[1].transform, &graphicsTest->objTransforms[1]);

    graphicsTest->scene.objects[2].model = &graphicsTest->models[2];
    graphicsTest->objTransforms[2].scale[0] = 1.0;
    graphicsTest->objTransforms[2].scale[1] = 1.0;
    graphicsTest->objTransforms[2].scale[2] = 1.0;
    graphicsTest->objTransforms[2].rotate[0] = 1.0;
    graphicsTest->objTransforms[2].translate[0] = 60.0;
    graphicsTest->objTransforms[2].translate[1] = 60.0;
    graphicsTest->objTransforms[2].translate[2] = 0.0;
    transformBaseToMatrix(graphicsTest->scene.objects[2].transform, &graphicsTest->objTransforms[2]);

    // Repeat any of the arrow buttons every .1s after first holding for .3s
    graphicsTest->repeatState.repeatMask = PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT;
    graphicsTest->repeatState.repeatDelay = 300000;
    graphicsTest->repeatState.repeatInterval = 60000;

    // Ensure there's sufficient space to draw both models
    initRendererScene(&graphicsTest->scene);

    // We shold go as fast as we can.
    setFrameRateUs(0);
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void graphicsTestExitMode(void)
{
    // Free renderer memory
    deinitRenderer();

    // Free the 3D models
    for (int i = 0; i < ARRAY_SIZE(graphicsTestModels); i++)
    {
        if (graphicsTest->modelLoaded[i])
        {
            freeModel(&graphicsTest->models[i]);
        }
    }

    // Free menu related things
    deinitMenuLogbookRenderer(graphicsTest->menuRenderer);
    deinitMenu(graphicsTest->menu);

    // Free the font
    freeFont(&graphicsTest->ibm);
    free(graphicsTest);
}

/**
 * @brief This function is called periodically and frequently. It will either read inpust and draw the screen.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void graphicsTestMainLoop(int64_t elapsedUs)
{
    // Do update each loop
    graphicsTestHandleInput();

    if (graphicsTest->showMenu)
    {
        // Draw the menu
        drawMenuLogbook(graphicsTest->menu, graphicsTest->menuRenderer, elapsedUs);
    }
    else
    {
        // Draw the models
        graphicsTestDrawScene();

        char buffer[32];

        // Draw control type
        const char* touchActionStr = getTouchActionDesc(graphicsTest->touchAction);
        if (graphicsTest->selectedModel == -1)
        {
            snprintf(buffer, sizeof(buffer), "Sel: World  Touch: %s", touchActionStr);
            drawText(&graphicsTest->ibm, c555, buffer, 30, 5);
        }
        else if (graphicsTest->scene.objectCount > 0)
        {
            snprintf(buffer, sizeof(buffer), "Sel: #%" PRIu8 "/%" PRIu8 "  Touch: %s", graphicsTest->selectedModel + 1, graphicsTest->scene.objectCount, touchActionStr);
            drawText(&graphicsTest->ibm, c555, buffer, 30, 5);
        }

        // Draw FPS Counter
        graphicsTest->avgFrameTime = (graphicsTest->avgFrameTime * 9 / 10) + (elapsedUs * 1 / 10);

        int32_t framesPerSecond = (1000000 / (graphicsTest->avgFrameTime + 1));
        int32_t tenthFramesPerSecond = (10000000 / (graphicsTest->avgFrameTime + 1)) % 10;

        snprintf(buffer, sizeof(buffer), "%" PRId32 ".%" PRId32, framesPerSecond, tenthFramesPerSecond);
        int16_t textX = drawText(&graphicsTest->ibm, c550, buffer, TFT_WIDTH - 30 - textWidth(&graphicsTest->ibm, buffer), 5);

        // Draw a '!' after the FPS if we ran out of memory for drawing triangles
        // This should only happen if we used initRendererCustom() with fewer than
        // the total number of triangles to draw a bunch of objects
        if (*frameClipped)
        {
            drawText(&graphicsTest->ibm, c500, "!", textX, 5);
        }
    }
}

static void graphicsTestMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected && label == graphicsMenuItemReset)
    {
        graphicsTestReset();
        graphicsTest->showMenu = false;
    }
    else if (selected && label == graphicsMenuItemCancel)
    {
        graphicsTest->showMenu = false;
    }
    else if (selected && graphicsTest->scene.objectCount < SCENE_MAX_OBJECTS)
    {
        model_t* model = NULL;
        bool copy = false;
        if (label == graphicsMenuItemCopy && graphicsTest->scene.objectCount > 0)
        {
            // Copy the current model data to a new one
            memcpy(&graphicsTest->scene.objects[graphicsTest->scene.objectCount],
                    &graphicsTest->scene.objects[graphicsTest->selectedModel],
                    sizeof(obj3d_t));
            copy = true;
        }
        else
        {
            for (int i = 0; i < ARRAY_SIZE(graphicsTestModels); i++)
            {
                if (label == graphicsTestModels[i].label)
                {
                    if (!graphicsTest->modelLoaded[i])
                    {
                        ESP_LOGI("Model", "Loading unloaded model %s", graphicsTestModels[i].filename);
                        if (loadModel(graphicsTestModels[i].filename, &graphicsTest->models[i], true))
                        {
                            graphicsTest->modelLoaded[i] = true;
                        }
                    }

                    if (graphicsTest->modelLoaded[i])
                    {
                        model = &graphicsTest->models[i];
                    }
                    break;
                }
            }

            if (NULL == model)
            {
                return;
            }
        }

        if (!copy)
        {
            obj3d_t* obj = &graphicsTest->scene.objects[graphicsTest->scene.objectCount];
            obj->model = model;
            const float translate[3] = {0};
            float rotate[4];
            const float scale[3] = {1.0, 1.0, 1.0};
            accelGetQuaternion(rotate);
            createTransformMatrix(obj->transform, translate, rotate, scale);
        }

        // Switch to the newly created model
        graphicsTest->selectedModel = graphicsTest->scene.objectCount++;

        // And reallocate the scene
        initRendererScene(&graphicsTest->scene);
        graphicsTest->showMenu = false;
    }
}

/**
 * @brief Perform the accelerometer reading and record the sample
 */
static void graphicsTestHandleInput(void)
{
    if (graphicsTest->showMenu)
    {
        buttonEvt_t evt;
        while (checkButtonQueueWrapper(&evt))
        {
            if (evt.down && evt.button == PB_B)
            {
                graphicsTest->showMenu = false;
            }
            else
            {
                graphicsTest->menu = menuButton(graphicsTest->menu, evt);
            }
        }
        return;
    }
    else
    {
        transformBase_t* curTransform = (graphicsTest->scene.objectCount == 0 || graphicsTest->selectedModel == -1)
                                        ? &graphicsTest->worldTransform
                                        : &graphicsTest->objTransforms[graphicsTest->selectedModel];
        bool transformUpdated = false;

        // Handle Touchpad Input
        if (graphicsTest->scene.objectCount > 0)
        {
            int32_t angle, radius, intensity;
            if (getTouchJoystick(&angle, &radius, &intensity))
            {
                if (graphicsTest->touchAction == TRANSLATE_XY || graphicsTest->touchAction == TRANSLATE_ZY)
                {
                    // Handle Translate Modes
                    int32_t x, y;
                    getTouchCartesian(angle, radius, &x, &y);

                    if (!graphicsTest->touchState)
                    {
                        // Touch start!
                        graphicsTest->touchState = true;
                        graphicsTest->touchDragStartX = x;
                        graphicsTest->touchDragStartY = y;
                        memcpy(graphicsTest->touchDragStartPos, curTransform->translate, sizeof(float[3]));
                    }

                    float minX = -126.0;
                    float maxX = 126.0;
                    float minY = -126.0;
                    float maxY = 126.0;

                    int otherAxis = (graphicsTest->touchAction == TRANSLATE_XY) ? 1 : 2;
                    if (otherAxis != 2)
                    {
                        curTransform->translate[0] = graphicsTest->touchDragStartPos[0] + ((maxX - minX) * (x - graphicsTest->touchDragStartX) / 1023.0);
                    }
                    curTransform->translate[otherAxis] = graphicsTest->touchDragStartPos[otherAxis] + ((maxY - minY) * (y - graphicsTest->touchDragStartY) / 1023.0);
                }
                else
                {
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

                    ESP_LOGI("GraphicsTest", "Rotating by %.2f, %.2f, %.2f",
                             eulerRot[0], eulerRot[1], eulerRot[2]);

                    // Update the object's orient to its original orient rotated by the new rotation
                    mathQuatApply(curTransform->rotate, graphicsTest->touchSpinStartOrient, newRot);
                }

                transformUpdated = true;
            }
            else if (graphicsTest->touchState)
            {
                graphicsTest->touchState = false;
                graphicsTest->touchSpinState.startSet = false;
            }
        }

        // Handle Button Presses
        // Always process button events, regardless of control scheme, so the main menu button can be captured
        buttonEvt_t evt = {0};
        while (checkButtonQueueRepeat(&graphicsTest->repeatState, &evt))
        {
            // Save the button state
            graphicsTest->btnState = evt.state;

            if (evt.down)
            {
                switch (evt.button)
                {
                    case PB_START:
                    {
                        // Add object -- go to the menu
                        graphicsTestSetupMenu();
                        graphicsTest->showMenu = true;

                        // Reset the touch so weird stuff doesn't happen later
                        graphicsTest->touchState = false;
                        break;
                    }

                    case PB_A:
                    {
                        switch (graphicsTest->touchAction)
                        {
                            case TRANSLATE_XY:
                                graphicsTest->touchAction = TRANSLATE_ZY;
                                break;

                            case TRANSLATE_ZY:
                                graphicsTest->touchAction = ROTATE_Z;
                                break;

                            case ROTATE_Z:
                                graphicsTest->touchAction = ROTATE_Y;
                                break;

                            case ROTATE_Y:
                                graphicsTest->touchAction = ROTATE_X;
                                break;

                            case ROTATE_X:
                            default:
                                graphicsTest->touchAction = TRANSLATE_XY;
                                break;
                        }
                        graphicsTest->touchState = false;
                        break;
                    }

                    case PB_B:
                    {
                        // Delete object
                        if (graphicsTest->selectedModel != -1 && graphicsTest->scene.objectCount > 0)
                        {
                            const model_t* modelToUnload = graphicsTest->scene.objects[graphicsTest->selectedModel].model;

                            for (int i = graphicsTest->selectedModel; i < graphicsTest->scene.objectCount - 1; i++)
                            {
                                // Shift down any models afterwards in the array
                                memcpy(&graphicsTest->scene.objects[i], &graphicsTest->scene.objects[i + 1], sizeof(obj3d_t));
                            }

                            // We can leave the last one there, who cares
                            graphicsTest->scene.objectCount--;
                            graphicsTest->selectedModel--;

                            bool unloadModel = true;
                            for (int i = 0; i < graphicsTest->scene.objectCount; i++)
                            {
                                if (graphicsTest->scene.objects[i].model == modelToUnload)
                                {
                                    unloadModel = false;
                                    break;
                                }
                            }

                            if (unloadModel)
                            {
                                for (int i = 0; i < ARRAY_SIZE(graphicsTestModels); i++)
                                {
                                    if (modelToUnload == &graphicsTest->models[i])
                                    {
                                        ESP_LOGI("Model", "Unloading model %s", graphicsTestModels[i].filename);
                                        freeModel(&graphicsTest->models[i]);
                                        graphicsTest->modelLoaded[i] = false;
                                        break;
                                    }
                                }
                            }

                            // Reset the touch so weird stuff doesn't happen
                            graphicsTest->touchState = false;
                        }
                        break;
                    }

                    case PB_UP:
                    {
                        curTransform->scale[0] += .05;
                        curTransform->scale[1] += .05;
                        curTransform->scale[2] += .05;
                        transformUpdated = true;
                        break;
                    }

                    case PB_DOWN:
                    {
                        if (curTransform->scale[0] > .0501)
                        {
                            curTransform->scale[0] -= .05;
                            curTransform->scale[1] -= .05;
                            curTransform->scale[2] -= .05;
                            transformUpdated = true;
                        }
                        break;
                    }

                    case PB_LEFT:
                    {
                        if (graphicsTest->scene.objectCount > 0)
                        {
                            if (graphicsTest->selectedModel >= 0)
                            {
                                graphicsTest->selectedModel--;
                            }
                            else
                            {
                                graphicsTest->selectedModel = graphicsTest->scene.objectCount - 1;
                            }
                        }
                        break;
                    }

                    case PB_RIGHT:
                    {
                        // Next object
                        if (graphicsTest->scene.objectCount > 0)
                        {
                            graphicsTest->selectedModel = (graphicsTest->selectedModel + 2) % (graphicsTest->scene.objectCount + 1) - 1;

                            // Reset the touch so weird stuff doesn't happen
                            graphicsTest->touchState = false;
                        }
                        break;
                    }

                    case PB_SELECT:
                    {
                        break;
                    }
                }
            }
        }

        if (transformUpdated)
        {
            transformBaseToMatrix((graphicsTest->selectedModel == -1)
                                  ? graphicsTest->scene.transform
                                  : graphicsTest->scene.objects[graphicsTest->selectedModel].transform,
                                  curTransform);
        }
    }
}

/**
 * @brief Draw the bunny
 */
static void graphicsTestDrawScene(void)
{
    // Get the orientation from the accelerometer
    float finalOrient[4] = {1, 0, 0, 0};
    accelGetQuaternion(finalOrient);
    mathQuatApply(finalOrient, graphicsTest->worldTransform.rotate, finalOrient);
    mathQuatNormalize(finalOrient, finalOrient);

    // FIXME this is definitely band-aiding something that's fundamentally incorrect
    finalOrient[0] *= -1;
    createViewMatrix(graphicsTest->scene.transform, graphicsTest->worldTransform.translate, finalOrient, graphicsTest->worldTransform.scale);

    drawScene(&graphicsTest->scene, 0, 0, TFT_WIDTH, TFT_HEIGHT);
}

static void graphicsTestSetupMenu(void)
{
    // Remove any existing items
    const node_t* node = graphicsTest->menu->items->first;
    while (node != NULL)
    {
        const char* label = ((menuItem_t*)node->val)->label;

        // Advance to the next node BEFORE we remove the current one
        // (because it will be free'd after that)
        node = node->next;

        if (label != NULL)
        {
            removeSingleItemFromMenu(graphicsTest->menu, label);
        }
    }

    // Add back the items
    if (graphicsTest->selectedModel != -1 && graphicsTest->scene.objectCount > 0 && graphicsTest->scene.objectCount < SCENE_MAX_OBJECTS)
    {
        addSingleItemToMenu(graphicsTest->menu, graphicsMenuItemCopy);
    }

    if (graphicsTest->scene.objectCount < SCENE_MAX_OBJECTS)
    {
        for (int i = 0; i < ARRAY_SIZE(graphicsTestModels); i++)
        {
            addSingleItemToMenu(graphicsTest->menu, graphicsTestModels[i].label);
        }
    }
    addSingleItemToMenu(graphicsTest->menu, graphicsMenuItemReset);
    addSingleItemToMenu(graphicsTest->menu, graphicsMenuItemCancel);
}

/**
 * @brief Reset the graphics test scene
 */
static void graphicsTestReset(void)
{
    graphicsTest->scene.objectCount = 0;
    graphicsTest->selectedModel = -1;

    graphicsTest->worldTransform.translate[0] = 0.0;
    graphicsTest->worldTransform.translate[1] = 0.0;
    graphicsTest->worldTransform.translate[2] = 0.0;

    graphicsTest->worldTransform.rotate[0] = 1.0;
    graphicsTest->worldTransform.rotate[1] = 0.0;
    graphicsTest->worldTransform.rotate[2] = 0.0;
    graphicsTest->worldTransform.rotate[3] = 0.0;

    graphicsTest->worldTransform.scale[0] = 1.0;
    graphicsTest->worldTransform.scale[1] = 1.0;
    graphicsTest->worldTransform.scale[2] = 1.0;
}

static const char* getTouchActionDesc(touchAction_t action)
{
    switch (action)
    {
        case TRANSLATE_XY:
            return actionTranslateXYStr;

        case TRANSLATE_ZY:
            return actionTranslateZYStr;

        case ROTATE_Z:
            return actionRotateZStr;

        case ROTATE_Y:
            return actionRotateYStr;

        case ROTATE_X:
            return actionRotateXStr;

        default:
            return NULL;
    }
}

static void transformBaseToMatrix(float mat[4][4], const transformBase_t* base)
{
    createTransformMatrix(mat, base->translate, base->rotate, base->scale);
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
static void graphicsTestBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    accelIntegrate();
    fillDisplayArea(x, y, x + w, y + h, c000);
}
