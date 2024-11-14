/**
 * @file accelTest.c
 * @author dylwhich (dylan@whichard.com)
 * @brief A test mode to view accelerometer data
 * @date 2023-08-02
 *
 * TODO smoothing, derivative / integral, etc.
 */

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"
#include "accelTest.h"
#include "esp_log.h"
#include "trigonometry.h"
#include "shapes.h"
#include "fill.h"
#include "linked_list.h"
#include "font.h"
#include "bunny.h"

//==============================================================================
// Defines
//==============================================================================

#define ACCEL_FMT "(%03" PRId16 ", %03" PRId16 ", %03" PRId16 ")"
///< The total size of the sample buffer
#define SAMPLES 512
///< The current number of stored samples
#define SAMPLE_COUNT                                                              \
    ((accelTest->first <= accelTest->last) ? (accelTest->last - accelTest->first) \
                                           : (accelTest->last + (SAMPLES - accelTest->first)))
///< Whether or not the samples buffer is full
#define SAMPLES_FULL (((accelTest->last + 1) % SAMPLES) == accelTest->first)

// Graph area definitions
#define GRAPH_TOP    0
#define GRAPH_BOTTOM TFT_HEIGHT
#define GRAPH_LEFT   0
#define GRAPH_RIGHT  (TFT_WIDTH - 31)

// Various graph colors
#define GRAPH_BG_COLOR   c000
#define GRAPH_AXIS_COLOR c555
#define GRAPH_TEXT_COLOR c222

#define GRAPH_X_COLOR_POINT c400
#define GRAPH_Y_COLOR_POINT c040
#define GRAPH_Z_COLOR_POINT c004

#define GRAPH_X_COLOR_LINE c500
#define GRAPH_Y_COLOR_LINE c050
#define GRAPH_Z_COLOR_LINE c005

// Picked after a very short amount of testing, could be pretty wrong
#define ONE_G 242

//==============================================================================
// Structs
//==============================================================================

/// @brief A struct for holding a single sample from the accelerometer
typedef struct
{
    int16_t x, y, z;
} accelSample_t;

/// @brief The struct that holds all the state for the accelerometer test mode
typedef struct
{
    font_t ibm; ///< The font used to display text

    uint16_t btnState; ///< The button state

    int16_t x; ///< The latest X accelerometer reading
    int16_t y; ///< The latest Y accelerometer reading
    int16_t z; ///< The latest Z accelerometer reading

    accelSample_t min; ///< The minimum reading for each axis
    accelSample_t max; ///< The maximum reading for each axis
    accelSample_t avg; ///< The average reading for each axis

    bool writeTextLabels; ///< Whether or not to show the statistics in text on the screen

    uint32_t first;                 ///< Index of the first sample in the circular buffer
    uint32_t last;                  ///< Index of the last sample in the circular buffer
    accelSample_t samples[SAMPLES]; ///< Circular buffer of samples
} accelTest_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void accelTestMainLoop(int64_t elapsedUs);
static void accelTestEnterMode(void);
static void accelTestExitMode(void);

static void accelTestReset(void);
static void accelTestSample(int16_t x, int16_t y, int16_t z);
static void accelTestHandleInput(void);
static void accelDrawBunny(void);

static void accelTestBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void accelTestDrawGraph(void);

//==============================================================================
// Strings
//==============================================================================

static const char accelTestName[] = "Accel Test";

static const char accelTestFormatCur[]     = "Cur: " ACCEL_FMT;
static const char accelTestFormatMin[]     = "Min: " ACCEL_FMT;
static const char accelTestFormatMax[]     = "Max: " ACCEL_FMT;
static const char accelTestFormatAvg[]     = "Avg: " ACCEL_FMT;
static const char accelTestFormatSamples[] = "Samples: %" PRId32;

//==============================================================================
// Variables
//==============================================================================

/// The Swadge mode for accelTest
swadgeMode_t accelTestMode = {
    .modeName                 = accelTestName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = accelTestEnterMode,
    .fnExitMode               = accelTestExitMode,
    .fnMainLoop               = accelTestMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = accelTestBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

/// All state information for the Accelerometer Test mode.
accelTest_t* accelTest = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter Accelerometer Test mode, allocate required memory, and initialize required variables
 *
 */
static void accelTestEnterMode(void)
{
    // Allocate and clear all memory for this mode. All the variables are contained in a single struct for convenience.
    // calloc() is used instead of malloc() because calloc() also initializes the allocated memory to zeros.
    accelTest = calloc(1, sizeof(accelTest_t));

    // Load a font
    loadFont("ibm_vga8.font", &accelTest->ibm, false);

    // writeTextlabels doesn't get reset by accelTestReset(), so initialize that here
    accelTest->writeTextLabels = true;

    // We shold go as fast as we can.
    setFrameRateUs(0);

    // Then, accelTestReset() takes care of everything else
    accelTestReset();
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void accelTestExitMode(void)
{
    // Free the font
    freeFont(&accelTest->ibm);
    free(accelTest);
}

/**
 * @brief This function is called periodically and frequently. It will either read inpust and draw the screen.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void accelTestMainLoop(int64_t elapsedUs)
{
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        accelTest->btnState = evt.state;

        // Check if the pause button was pressed
        if (evt.down && (PB_START == evt.button))
        {
            accelTestReset();
        }

        if (evt.down && (PB_A == evt.button))
        {
            accelTest->writeTextLabels = !accelTest->writeTextLabels;
        }
    }

    // Do update each loop
    accelTestHandleInput();

    // Draw the bunny
    accelDrawBunny();

    // Draw the field
    accelTestDrawGraph();
}

/**
 * @brief Records a sample into the circular buffer
 *
 * @param x The X-axis accelerometer reading
 * @param y The Y-axis accelerometer reading
 * @param z The Z-axis accelerometer reading
 */
static void accelTestSample(int16_t x, int16_t y, int16_t z)
{
    // Add a new sample at the end
    accelTest->samples[accelTest->last].x = x;
    accelTest->samples[accelTest->last].y = y;
    accelTest->samples[accelTest->last].z = z;

    if (SAMPLES_FULL)
    {
        // There's no more free space after the last, which means we just overwrote
        // the first sample! So, we need to move the first pointer to the next slot
        accelTest->first = (accelTest->first + 1) % SAMPLES;
    }

    // This puts the last pointer one after the sample we just added, where it should be
    accelTest->last = (accelTest->last + 1) % SAMPLES;
}

/**
 * @brief Perform the accelerometer reading and record the sample
 */
static void accelTestHandleInput(void)
{
    // Declare variables to receive acceleration
    // Get the current acceleration
    if (ESP_OK == accelGetAccelVecRaw(&(accelTest->x), &(accelTest->y), &(accelTest->z)))
    {
        accelTestSample(accelTest->x, accelTest->y, accelTest->z);
    }
}

/**
 * @brief Draw the bunny
 */
static void accelDrawBunny(void)
{
    // Produce a model matrix from a quaternion.
    float plusx_out[3] = {1, 0, 0};
    float plusy_out[3] = {0, 1, 0};
    float plusz_out[3] = {0, 0, 1};

    mathRotateVectorByQuaternion(plusy_out, LSM6DSL.fqQuat, plusy_out);
    mathRotateVectorByQuaternion(plusx_out, LSM6DSL.fqQuat, plusx_out);
    mathRotateVectorByQuaternion(plusz_out, LSM6DSL.fqQuat, plusz_out);

    int16_t bunny_verts_out[numBunnyVerts() / 3 * 3] = {0};
    int i, vertices = 0;
    for (i = 0; i < numBunnyVerts(); i += 3)
    {
        // Performingthe transform this way is about 700us.
        float bx                          = bunny_verts[i + 2];
        float by                          = bunny_verts[i + 1];
        float bz                          = -bunny_verts[i + 0];
        float bunnyvert[3]                = {bx * plusx_out[0] + by * plusx_out[1] + bz * plusx_out[2],
                                             bx * plusy_out[0] + by * plusy_out[1] + bz * plusy_out[2],
                                             bx * plusz_out[0] + by * plusz_out[1] + bz * plusz_out[2]};
        bunny_verts_out[vertices * 3 + 0] = bunnyvert[0] / 250 + 280 / 2;
        bunny_verts_out[vertices * 3 + 1]
            = -bunnyvert[1] / 250 + 240 / 2; // Convert from right-handed to left-handed coordinate frame.
        bunny_verts_out[vertices * 3 + 2] = bunnyvert[2];
        vertices++;
    }

    int lines = 0;
    for (i = 0; i < numBunnyLines(); i += 2)
    {
        int v1    = bunny_lines[i] * 3;
        int v2    = bunny_lines[i + 1] * 3;
        float col = bunny_verts_out[v1 + 2] / 2000 + 8;
        if (col > 5)
            col = 5;
        else if (col < 0)
            continue;
        drawLineFast(bunny_verts_out[v1], bunny_verts_out[v1 + 1], bunny_verts_out[v2], bunny_verts_out[v2 + 1], col);
        lines++;
    }
}

/**
 * @brief Reset the accelerometer test mode variables
 */
static void accelTestReset(void)
{
    accelSetRegistersAndReset();

    accelTest->first = 0;
    accelTest->last  = 0;

    accelTest->avg.x = 0;
    accelTest->avg.y = 0;
    accelTest->avg.z = 0;

    accelTest->min.x = -1;
    accelTest->min.y = -1;
    accelTest->min.z = -1;

    accelTest->max.x = 1;
    accelTest->max.y = 1;
    accelTest->max.z = 1;
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
static void accelTestBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    accelIntegrate();
    fillDisplayArea(x, y, x + w, y + h, GRAPH_BG_COLOR);
}

/**
 * @brief Draw the text and graphs, etc.
 */
static void accelTestDrawGraph(void)
{
    accelSample_t min = {
        .x = -1,
        .y = -1,
        .z = -1,
    };
    accelSample_t max = {
        .x = 1,
        .y = 1,
        .z = 1,
    };
    accelSample_t avg = {0};

// Make a macro for mapping a graph value onto a screen Y-coordinate
#define GRAPH_PLOT(value) (GRAPH_BOTTOM - (GRAPH_BOTTOM - GRAPH_TOP) / 2 - (value / 5))

    // Get the screen Y value of the origin (X-axis)
    uint16_t originY = GRAPH_PLOT(0);

    // Draw the X-axis line (y=0)
    drawLine(GRAPH_LEFT, originY, GRAPH_RIGHT, originY, GRAPH_AXIS_COLOR, 0);

    // Draw dotted line at 1G
    drawLine(GRAPH_LEFT, GRAPH_PLOT(ONE_G), GRAPH_RIGHT, GRAPH_PLOT(ONE_G), GRAPH_AXIS_COLOR, 2);

    // Draw dotted line at -1G
    drawLine(GRAPH_LEFT, GRAPH_PLOT(-ONE_G), GRAPH_RIGHT, GRAPH_PLOT(-ONE_G), GRAPH_AXIS_COLOR, 2);

    // Draw a box plot thingy for X
    // The box outline goes around the min and max
    drawRect(TFT_WIDTH - 30, GRAPH_PLOT(accelTest->max.x), TFT_WIDTH - 20, GRAPH_PLOT(accelTest->min.x),
             GRAPH_X_COLOR_LINE);
    // The filled area is the current value, starting from 0
    // Annoyingly, we have to make sure y0 <= y1 for fillDisplayArea(), which we do with MIN() and MAX()
    fillDisplayArea(TFT_WIDTH - 29, MIN(originY, GRAPH_PLOT(accelTest->x)), TFT_WIDTH - 21,
                    MAX(originY, GRAPH_PLOT(accelTest->x)), GRAPH_X_COLOR_POINT);
    // And then, the solid line across the bar indicates the average value
    drawLineFast(TFT_WIDTH - 29, GRAPH_PLOT(accelTest->avg.x), TFT_WIDTH - 21, GRAPH_PLOT(accelTest->avg.x),
                 GRAPH_AXIS_COLOR);

    // Same thing for Y
    drawRect(TFT_WIDTH - 20, GRAPH_PLOT(accelTest->max.y), TFT_WIDTH - 10, GRAPH_PLOT(accelTest->min.y),
             GRAPH_Y_COLOR_LINE);
    fillDisplayArea(TFT_WIDTH - 19, MIN(originY, GRAPH_PLOT(accelTest->y)), TFT_WIDTH - 11,
                    MAX(originY, GRAPH_PLOT(accelTest->y)), GRAPH_Y_COLOR_POINT);
    drawLineFast(TFT_WIDTH - 19, GRAPH_PLOT(accelTest->avg.y), TFT_WIDTH - 11, GRAPH_PLOT(accelTest->avg.y),
                 GRAPH_AXIS_COLOR);

    // Same thing for Z
    drawRect(TFT_WIDTH - 10, GRAPH_PLOT(accelTest->max.z), TFT_WIDTH, GRAPH_PLOT(accelTest->min.z), GRAPH_Z_COLOR_LINE);
    fillDisplayArea(TFT_WIDTH - 9, MIN(originY, GRAPH_PLOT(accelTest->z)), TFT_WIDTH - 1,
                    MAX(originY, GRAPH_PLOT(accelTest->z)), GRAPH_Z_COLOR_POINT);
    drawLineFast(TFT_WIDTH - 9, GRAPH_PLOT(accelTest->avg.z), TFT_WIDTH - 1, GRAPH_PLOT(accelTest->avg.z),
                 GRAPH_AXIS_COLOR);

    // Declare some big ints for the averages
    int64_t xSum = 0, ySum = 0, zSum = 0;

    // This loop will plot all samples, and also calculate the new min, max, and sum, which will be used when drawing
    // the next frame
    for (uint32_t i = accelTest->first; i != accelTest->last; i = (i + 1) % SAMPLES)
    {
        //
        uint32_t sampleNum    = (i < accelTest->first) ? (SAMPLES - accelTest->first - 1 + i) : (i - accelTest->first);
        accelSample_t* sample = accelTest->samples + i;
        accelSample_t* nextSample = NULL;

        // only have a "next" sample if we have more than 1 sample and we aren't on the last one
        if (SAMPLE_COUNT > 1 && ((i + 1) % SAMPLES != accelTest->last))
        {
            nextSample = accelTest->samples + ((i + 1) % SAMPLES);
        }

        // Update sums for average
        xSum += sample->x;
        ySum += sample->y;
        zSum += sample->z;

        // Update X min/max
        if (sample->x > max.x)
        {
            max.x = sample->x;
        }
        else if (sample->x < min.x)
        {
            min.x = sample->x;
        }

        // Update Y min/max
        if (sample->y > max.y)
        {
            max.y = sample->y;
        }
        else if (sample->y < min.y)
        {
            min.y = sample->y;
        }

        // Update Z min/max
        if (sample->z > max.z)
        {
            max.z = sample->z;
        }
        else if (sample->z < min.z)
        {
            min.z = sample->z;
        }

        // Calculate the screen X
        uint16_t dotHoriz = sampleNum * (GRAPH_RIGHT - GRAPH_LEFT) / SAMPLE_COUNT;
        // Calculate all the screen Ys
        uint16_t xDotVert = GRAPH_PLOT(sample->x);
        uint16_t yDotVert = GRAPH_PLOT(sample->y);
        uint16_t zDotVert = GRAPH_PLOT(sample->z);

        // If we have a next sample, draw a line connecting this sample to it
        if (nextSample != NULL)
        {
            // Calculate the screen X of the next sample
            uint16_t nextDotHoriz = (sampleNum + 1) * (GRAPH_RIGHT - GRAPH_LEFT) / SAMPLE_COUNT;
            // Calculate all the screen Ys of the next sample
            uint16_t nextXDotVert = GRAPH_PLOT(nextSample->x);
            uint16_t nextYDotVert = GRAPH_PLOT(nextSample->y);
            uint16_t nextZDotVert = GRAPH_PLOT(nextSample->z);

            // Draw a line connecting this sample to the next one
            drawLineFast(dotHoriz, xDotVert, nextDotHoriz, nextXDotVert, GRAPH_X_COLOR_LINE);
            drawLineFast(dotHoriz, yDotVert, nextDotHoriz, nextYDotVert, GRAPH_Y_COLOR_LINE);
            drawLineFast(dotHoriz, zDotVert, nextDotHoriz, nextZDotVert, GRAPH_Z_COLOR_LINE);
        }

        // Plot the dots on the graph for this sample
        setPxTft(dotHoriz, xDotVert, GRAPH_X_COLOR_POINT);
        setPxTft(dotHoriz, yDotVert, GRAPH_Y_COLOR_POINT);
        setPxTft(dotHoriz, zDotVert, GRAPH_Z_COLOR_POINT);
    }

    // Done with the calculate / plot loop

    // Calculate the actual averages
    avg.x = xSum / SAMPLE_COUNT;
    avg.y = ySum / SAMPLE_COUNT;
    avg.z = zSum / SAMPLE_COUNT;

    // Label some axes
    // "0" above the X axis
    drawText(&accelTest->ibm, GRAPH_AXIS_COLOR, "0", GRAPH_LEFT + 2, originY - accelTest->ibm.height - 2);

    // "+1G" and "-1G" labels above/below their respective lines (drawn before plotting)
    drawText(&accelTest->ibm, GRAPH_AXIS_COLOR, "+1G", GRAPH_LEFT + 2, GRAPH_PLOT(ONE_G) - accelTest->ibm.height - 2);
    drawText(&accelTest->ibm, GRAPH_AXIS_COLOR, "-1G", GRAPH_LEFT + 2, GRAPH_PLOT(-ONE_G) + 2);

    // And the same for "+2G" and "-2G"
    drawText(&accelTest->ibm, GRAPH_AXIS_COLOR, "+2G", GRAPH_LEFT + 2,
             GRAPH_PLOT(2 * ONE_G) - accelTest->ibm.height - 2);
    drawText(&accelTest->ibm, GRAPH_AXIS_COLOR, "-2G", GRAPH_LEFT + 2, GRAPH_PLOT(-2 * ONE_G) + 2);

    if (accelTest->writeTextLabels)
    {
        char strBuf[32] = {0};
        uint16_t textY  = TFT_HEIGHT - accelTest->ibm.height - 1 - 10;

        // Format and draw the CURRENT reading
        snprintf(strBuf, sizeof(strBuf) - 1, accelTestFormatCur, accelTest->x, accelTest->y, accelTest->z);
        drawText(&accelTest->ibm, GRAPH_TEXT_COLOR, strBuf, 30, textY);
        textY -= accelTest->ibm.height + 1;

        // Format and draw the MINIMUM reading
        snprintf(strBuf, sizeof(strBuf) - 1, accelTestFormatMin, min.x, min.y, min.z);
        drawText(&accelTest->ibm, GRAPH_TEXT_COLOR, strBuf, 30, textY);
        textY -= accelTest->ibm.height + 1;

        // Format and draw the MAXIMUM reading
        snprintf(strBuf, sizeof(strBuf) - 1, accelTestFormatMax, max.x, max.y, max.z);
        drawText(&accelTest->ibm, GRAPH_TEXT_COLOR, strBuf, 30, textY);
        textY -= accelTest->ibm.height + 1;

        // Format and draw the AVERAGE reading
        snprintf(strBuf, sizeof(strBuf) - 1, accelTestFormatAvg, avg.x, avg.y, avg.z);
        drawText(&accelTest->ibm, GRAPH_TEXT_COLOR, strBuf, 30, textY);
        textY -= accelTest->ibm.height + 1;

        // Format and draw the number of samples
        snprintf(strBuf, sizeof(strBuf) - 1, accelTestFormatSamples, SAMPLE_COUNT);
        drawText(&accelTest->ibm, GRAPH_TEXT_COLOR, strBuf, 30, textY);
    }

    // Replace the old min/max/avg values with the newly calculated ones
    accelTest->min = min;
    accelTest->max = max;
    accelTest->avg = avg;
}
