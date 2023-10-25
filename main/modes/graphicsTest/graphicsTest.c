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
#include "shapes.h"
#include "fill.h"
#include "linked_list.h"
#include "font.h"
#include "model.h"
#include "spiffs_model.h"
#include "buttonUtils.h"
#include "menu.h"
#include "menuLogbookRenderer.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    TRANSLATE_XY,
    TRANSLATE_ZY,
    ROTATE_Z,
    ROTATE_Y,
    ROTATE_X,
} touchAction_t;

//==============================================================================
// Structs
//==============================================================================

/// @brief The struct that holds all the state for the graphics test mode
typedef struct
{
    font_t ibm; ///< The font used to display text
    int64_t avgFrameTime;

    menu_t* menu; ///< The menu for adding a model
    menuLogbookRenderer_t* menuRenderer; ///< The menu's renderer
    bool showMenu; ///< Whether or not the menu is active

    model_t bunny; ///< The bunny 3D model
    model_t donut; ///< The donut 3D model
    model_t funkus; ///< The big funkus 3D model
    model_t cube;

    scene_t scene; ///< The 3D scene for the renderer

    uint8_t selectedModel; ///< The index of the selected model

    uint16_t btnState; ///< The button state
    buttonRepeatState_t repeatState; ///< The button repeat state

    bool touchState;
    touchAction_t touchAction;
    int32_t touchDragStartX;
    int32_t touchDragStartY;
    float touchDragStartPos[3];
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

static void graphicsTestBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

//==============================================================================
// Strings
//==============================================================================

static const char graphicsTestName[] = "Graphics Test";

static const char graphicsMenuItemCopy[] = "Copy";
static const char graphicsMenuItemDonut[] = "Donut";
static const char graphicsMenuItemFunkus[] = "Funkus";
static const char graphicsMenuItemBunny[] = "Bunny";;
static const char graphicsMenuItemBox[] = "Cube";


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

    // Load all the 3D models!
#define LOAD_MODEL(name, var) ESP_LOGI("Model", "loadModel(" name ") returned %s", \
                                       loadModel(name, &graphicsTest->var, true) \
                                       ? "true" : "false")
    LOAD_MODEL("bigfunkus.mdl", funkus);
    LOAD_MODEL("donut.mdl", donut);
    LOAD_MODEL("bunny.mdl", bunny);
    LOAD_MODEL("cube.mdl", cube);
#undef LOAD_MODEL

    graphicsTest->scene.modelCount = 3;
    graphicsTest->scene.models[0].model = &graphicsTest->donut;
    graphicsTest->scene.models[0].scale = 1.0;
    graphicsTest->scene.models[0].translate[0] = -25.0;
    graphicsTest->scene.models[0].translate[1] = -25.0;
    graphicsTest->scene.models[0].translate[2] = 0.0;

    graphicsTest->scene.models[1].model = &graphicsTest->funkus;
    graphicsTest->scene.models[1].scale = 1.0;
    graphicsTest->scene.models[1].translate[0] = 25.0;
    graphicsTest->scene.models[1].translate[1] = 25.0;
    graphicsTest->scene.models[1].translate[2] = 0.0;

    graphicsTest->scene.models[2].model = &graphicsTest->bunny;
    graphicsTest->scene.models[2].scale = 1.0;
    graphicsTest->scene.models[2].translate[0] = 60.0;
    graphicsTest->scene.models[2].translate[1] = 60.0;
    graphicsTest->scene.models[2].translate[2] = 0.0;

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

    // Free the bunny 3D model
    freeModel(&graphicsTest->bunny);

    // Free the big funkus 3D model
    freeModel(&graphicsTest->funkus);

    // Free the king donut 3D model
    freeModel(&graphicsTest->donut);

    // Free the cube model
    freeModel(&graphicsTest->cube);

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
        if (graphicsTest->scene.modelCount > 0)
        {
            const char* touchActionStr = (graphicsTest->touchAction == TRANSLATE_XY) ? "X/Y" : "Z/Y";
            snprintf(buffer, sizeof(buffer), "Touch: Move #%" PRIu8 " %s", graphicsTest->selectedModel, touchActionStr);
            drawText(&graphicsTest->ibm, c555, buffer, 30, 5);
        }

        // Draw FPS Counter
        graphicsTest->avgFrameTime = (graphicsTest->avgFrameTime * 9 / 10) + (elapsedUs * 1 / 10);

        int32_t framesPerSecond = (1000000 / (graphicsTest->avgFrameTime + 1));
        int32_t tenthFramesPerSecond = (10000000 / (graphicsTest->avgFrameTime + 1)) % 10;

        snprintf(buffer, sizeof(buffer), "%" PRId32 ".%" PRId32, framesPerSecond, tenthFramesPerSecond);
        drawText(&graphicsTest->ibm, c550, buffer, TFT_WIDTH - 30 - textWidth(&graphicsTest->ibm, buffer), 5);
    }
}

static void graphicsTestMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected && graphicsTest->scene.modelCount < SCENE_MAX_OBJECTS)
    {
        model_t* model = NULL;
        bool copy = false;
        if (label == graphicsMenuItemCopy && graphicsTest->scene.modelCount > 0)
        {
            // Copy the current model data to a new one
            memcpy(&graphicsTest->scene.models[graphicsTest->scene.modelCount],
                    &graphicsTest->scene.models[graphicsTest->selectedModel],
                    sizeof(modelPos_t));
            copy = true;
        }
        else if (label == graphicsMenuItemDonut)
        {
            model = &graphicsTest->donut;
        }
        else if (label == graphicsMenuItemFunkus)
        {
            model = &graphicsTest->funkus;
        }
        else if (label == graphicsMenuItemBunny)
        {
            model = &graphicsTest->bunny;
        }
        else if (label == graphicsMenuItemBox)
        {
            model = &graphicsTest->cube;
        }
        else
        {
            return;
        }

        if (!copy)
        {
            modelPos_t* obj = &graphicsTest->scene.models[graphicsTest->scene.modelCount];
            obj->model = model;
            accelGetQuaternion(obj->orient);
            obj->scale = 1.0;
            obj->translate[0] = 0;
            obj->translate[1] = 0;
            obj->translate[2] = 0;
        }

        // Switch to the newly created model
        graphicsTest->selectedModel = graphicsTest->scene.modelCount++;

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
            graphicsTest->menu = menuButton(graphicsTest->menu, evt);
        }
        return;
    }
    else
    {
        // Handle Touchpad Input
        if (graphicsTest->scene.modelCount > 0)
        {
            int32_t angle, radius, intensity;
            if (getTouchJoystick(&angle, &radius, &intensity))
            {
                if (graphicsTest->touchAction == TRANSLATE_XY || graphicsTest->touchAction == TRANSLATE_ZY)
                {
                    // Handle Translate Modes
                    int32_t x, y;
                    getTouchCartesian(angle, radius, &x, &y);

                    modelPos_t* curModel = &graphicsTest->scene.models[graphicsTest->selectedModel];

                    if (!graphicsTest->touchState)
                    {
                        // Touch start!
                        graphicsTest->touchState = true;
                        graphicsTest->touchDragStartX = x;
                        graphicsTest->touchDragStartY = y;
                        memcpy(graphicsTest->touchDragStartPos, curModel->translate, sizeof(float[3]));
                    }

                    float minX = -126.0 / curModel->scale;
                    float maxX = 126.0 / curModel->scale;
                    float minY = -126.0 / curModel->scale;
                    float maxY = 126.0 / curModel->scale;

                    int otherAxis = (graphicsTest->touchAction == TRANSLATE_XY) ? 1 : 2;

                    curModel->translate[0] = graphicsTest->touchDragStartPos[0] + ((maxX - minX) * (x - graphicsTest->touchDragStartX) / 1023.0);
                    curModel->translate[otherAxis] = graphicsTest->touchDragStartPos[otherAxis] + ((maxY - minY) * (y - graphicsTest->touchDragStartY) / 1023.0);
                }
                else
                {
                    // Rotation Actions
                    // TODO
                }
            }
            else if (graphicsTest->touchState)
            {
                graphicsTest->touchState = false;
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
                                graphicsTest->touchAction = TRANSLATE_XY;
                            break;

                            case ROTATE_Z:
                            case ROTATE_Y:
                            case ROTATE_X:
                            default:
                            break;
                        }
                        graphicsTest->touchState = false;
                        break;
                    }

                    case PB_B:
                    {
                        // Delete object
                        if (graphicsTest->scene.modelCount > 0)
                        {
                            for (int i = graphicsTest->selectedModel; i < graphicsTest->scene.modelCount - 1; i++)
                            {
                                // Shift down any models afterwards in the array
                                memcpy(&graphicsTest->scene.models[i], &graphicsTest->scene.models[i + 1], sizeof(modelPos_t));
                            }

                            // We can leave the last one there, who cares
                            graphicsTest->scene.modelCount--;

                            // Reset the touch so weird stuff doesn't happen
                            graphicsTest->touchState = false;
                        }
                        break;
                    }

                    case PB_UP:
                    {
                        graphicsTest->scene.models[graphicsTest->selectedModel].scale += .1;
                        break;
                    }

                    case PB_DOWN:
                    {
                        graphicsTest->scene.models[graphicsTest->selectedModel].scale -= .1;
                        break;
                    }

                    case PB_LEFT:
                    {
                        if (graphicsTest->scene.modelCount > 0)
                        {
                            if (graphicsTest->selectedModel > 0)
                            {
                                graphicsTest->selectedModel--;
                            }
                            else
                            {
                                graphicsTest->selectedModel = graphicsTest->scene.modelCount - 1;
                            }
                        }
                        break;
                    }

                    case PB_RIGHT:
                    {
                        // Next object
                        if (graphicsTest->scene.modelCount > 0)
                        {
                            graphicsTest->selectedModel = (graphicsTest->selectedModel + 1) % graphicsTest->scene.modelCount;

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
    }
}

/**
 * @brief Draw the bunny
 */
static void graphicsTestDrawScene(void)
{
    // Get the orientation from the accelerometer
    for (uint8_t i = 0; i < graphicsTest->scene.modelCount; i++)
    {
        accelGetQuaternion(graphicsTest->scene.models[i].orient);

        /*if (i == 0)
        {
            float objRot[4] = {0};
            objRot[3] = 600.0;
            mathQuatApply(graphicsTest->scene.models[i].orient, graphicsTest->scene.models[i].orient, objRot);
        }*/
    }

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
    if (graphicsTest->scene.modelCount > 0)
    {
        addSingleItemToMenu(graphicsTest->menu, graphicsMenuItemCopy);
    }
    addSingleItemToMenu(graphicsTest->menu, graphicsMenuItemDonut);
    addSingleItemToMenu(graphicsTest->menu, graphicsMenuItemFunkus);
    addSingleItemToMenu(graphicsTest->menu, graphicsMenuItemBunny);
    addSingleItemToMenu(graphicsTest->menu, graphicsMenuItemBox);
}

/**
 * @brief Reset the accelerometer test mode variables
 */
static void graphicsTestReset(void)
{
    graphicsTest->scene.modelCount = 0;
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
