//==============================================================================
// Includes
//==============================================================================

#include "portableDance.h"
#include <string.h>

//==============================================================================
// Prototypes
//==============================================================================

void portableDanceLoadSetting(portableDance_t* dance);

//==============================================================================
// Const Variables
//==============================================================================

static const char nvsKeyDanceSplit[]        = "dance_split";
static const char nvsKeyDanceUseColor[]     = "dance_use_color";
static const char nvsKeyDanceWingUseColor[] = "dance_wing_use_color";
static const char nvsKeyDanceIndex[]        = "dance_index";
static const char nvsKeyDanceSpeed[]        = "dance_speed";
static const char nvsKeyDanceColor[]        = "dance_color";
static const char nvsKeyWingDanceIndex[]    = "wing_dance_index";
static const char nvsKeyWingDanceSpeed[]    = "wing_dance_speed";
static const char nvsKeyDanceWingColor[]    = "wing_dance_color";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Returns a pointer to a portableDance_t.
 *
 * @param nvsNs The namespace where the dance index and speed will be loaded and saved, if not NULL
 * @return A pointer to a new portableDance_t
 */
portableDance_t* initPortableDance(const char* nvsNs)
{
    portableDance_t* dance = heap_caps_calloc(1, sizeof(portableDance_t), MALLOC_CAP_8BIT);
    dance->dances          = heap_caps_calloc(1, sizeof(ledDanceOpt_t) * getNumDances(), MALLOC_CAP_8BIT);
    for (uint8_t i = 0; i < getNumDances(); i++)
    {
        dance->dances[i].dance  = ledDances + i;
        dance->dances[i].enable = true;
    }

    dance->resetDance = true;
    dance->speed      = DANCE_SPEED_MULT;

    if (nvsNs != NULL)
    {
        dance->nvsNs = nvsNs;
        portableDanceLoadSetting(dance);
    }

    return dance;
}

/**
 * @brief Deletes the given portableDance_t.
 *
 * @param dance A pointer to the portableDance_t to delete
 */
void freePortableDance(portableDance_t* dance)
{
    if (dance != NULL)
    {
        if (dance->dances != NULL)
        {
            heap_caps_free(dance->dances);
        }
        heap_caps_free(dance);
    }
}

/**
 * @brief Runs the current LED dance. Should be called from the main loop every frame.
 *
 * @param dance The portableDance_t pointer
 * @param elapsedUs The number of microseconds since the last frame
 */
void portableDanceMainLoop(portableDance_t* dance, int64_t elapsedUs)
{
    if (dance->split) // yup, we can remove this whole block in a future year when there are not two distinct groups of
                      // LEDs.
    { // but in all likelyhood, "Fairy Split" may simply be renamed to fit the design. i.e. "Hotdog Split" with
      // "Condiment" & "Bun" menu strings.
        led_t leds[CONFIG_NUM_LEDS] = {{0}};

        uint32_t updateCount = getLedUpdateCount();

        dance->dances[dance->danceIndex].dance->func(
            (int32_t)elapsedUs * DANCE_SPEED_MULT / dance->speed,
            (dance->dances[dance->danceIndex].dance->userColor && dance->useColor)
                ? dance->color
                : dance->dances[dance->danceIndex].dance->arg,
            dance->resetDance, dance->useColor);

        if (updateCount != getLedUpdateCount())
        {
            const led_t* currentLeds = getLedState();
            for (uint8_t i = 0; i < 6; i++)
            {
                dance->bodyLeds[i] = currentLeds[i];
            }
        }

        updateCount = getLedUpdateCount();
        dance->dances[dance->wingDanceIndex].dance->func(
            (int32_t)elapsedUs * DANCE_SPEED_MULT / dance->wingSpeed,
            (dance->dances[dance->wingDanceIndex].dance->userColor && dance->wingUseColor)
                ? dance->wingColor
                : dance->dances[dance->wingDanceIndex].dance->arg,
            dance->resetDance, dance->wingUseColor);

        if (updateCount != getLedUpdateCount())
        {
            const led_t* currentLeds = getLedState();
            for (uint8_t i = 6; i < CONFIG_NUM_LEDS; i++)
            {
                dance->wingLeds[i] = currentLeds[i];
            }
        }

        for (uint8_t i = 0; i < 6; i++)
        {
            leds[i] = dance->bodyLeds[i];
        }
        for (uint8_t i = 6; i < CONFIG_NUM_LEDS; i++)
        {
            leds[i] = dance->wingLeds[i];
        }

        setLeds(leds, CONFIG_NUM_LEDS);
    }
    else
    {
        dance->dances[dance->danceIndex].dance->func(
            (int32_t)elapsedUs * DANCE_SPEED_MULT / dance->speed,
            (dance->dances[dance->danceIndex].dance->userColor && dance->useColor)
                ? dance->color
                : dance->dances[dance->danceIndex].dance->arg,
            dance->resetDance, dance->useColor);
    }
    dance->resetDance = false;
}

/**
 * @brief Read the portable dance index from NVS
 *
 * @param dance The portableDance_t to set an index in
 */
void portableDanceLoadSetting(portableDance_t* dance)
{
    int32_t split = 0;
    if (!readNamespaceNvs32(dance->nvsNs, nvsKeyDanceSplit, &split))
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceSplit, split);
    }
    dance->split = (bool)split;

    int32_t useColor = 0;
    if (!readNamespaceNvs32(dance->nvsNs, nvsKeyDanceUseColor, &useColor))
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceUseColor, useColor);
    }
    dance->useColor = (bool)useColor;

    int32_t wingUseColor = 0;
    if (!readNamespaceNvs32(dance->nvsNs, nvsKeyDanceWingUseColor, &wingUseColor))
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceWingUseColor, wingUseColor);
    }
    dance->wingUseColor = (bool)wingUseColor;

    int32_t color = 0;
    if (!readNamespaceNvs32(dance->nvsNs, nvsKeyDanceColor, &color))
    {
        // which is pinkish for a default start
        color = 0xFF33BB;
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceColor, color);
    }
    dance->color = color;

    color = 0;
    if (!readNamespaceNvs32(dance->nvsNs, nvsKeyDanceWingColor, &color))
    {
        //(255<<16) + (136<<8) + (0<<0)
        // which is orange for a default start
        color = 0xFF8800;
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceWingColor, color);
    }
    dance->wingColor = color;

    int32_t danceIndex = 0;
    if (!readNamespaceNvs32(dance->nvsNs, nvsKeyDanceIndex, &danceIndex))
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceIndex, danceIndex);
    }
    danceIndex        = CLAMP(danceIndex, 0, getNumDances() - 1);
    dance->danceIndex = (uint8_t)danceIndex;

    int32_t wingDanceIndex = 0;
    if (!readNamespaceNvs32(dance->nvsNs, nvsKeyWingDanceIndex, &wingDanceIndex))
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyWingDanceIndex, wingDanceIndex);
    }
    wingDanceIndex        = CLAMP(wingDanceIndex, 0, getNumDances() - 1);
    dance->wingDanceIndex = (uint8_t)wingDanceIndex;

    int32_t speed = DANCE_SPEED_MULT;
    if (!readNamespaceNvs32(dance->nvsNs, nvsKeyDanceSpeed, &speed))
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceSpeed, speed);
    }
    dance->speed = speed;

    int32_t wingSpeed = DANCE_SPEED_MULT;
    if (!readNamespaceNvs32(dance->nvsNs, nvsKeyWingDanceSpeed, &wingSpeed))
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyWingDanceSpeed, wingSpeed);
    }
    dance->wingSpeed = wingSpeed;
}

/**
 * @brief Sets the current LED split to the one specified, and updates the saved split. This works even
 * if a dance is disabled.
 *
 * @param dance The portableDance_t pointer to update
 * @param split The split value of the dance to update
 */
void portableDanceSetSplit(portableDance_t* dance, bool split)
{
    dance->split      = split;
    dance->resetDance = true;
    memset(dance->bodyLeds, 0, sizeof(dance->bodyLeds));
    memset(dance->wingLeds, 0, sizeof(dance->wingLeds));
    if (dance->nvsNs != NULL)
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceSplit, dance->split);
    }
}

void portableDanceSetUseColor(portableDance_t* dance, bool useColor)
{
    dance->useColor   = useColor;
    dance->resetDance = true;
    memset(dance->bodyLeds, 0, sizeof(dance->bodyLeds));
    memset(dance->wingLeds, 0, sizeof(dance->wingLeds));
    if (dance->nvsNs != NULL)
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceUseColor, dance->useColor);
    }
}

void portableDanceSetWingUseColor(portableDance_t* dance, bool useColor)
{
    dance->wingUseColor = useColor;
    dance->resetDance   = true;
    memset(dance->bodyLeds, 0, sizeof(dance->bodyLeds));
    memset(dance->wingLeds, 0, sizeof(dance->wingLeds));
    if (dance->nvsNs != NULL)
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceWingUseColor, dance->wingUseColor);
    }
}

/**
 * @brief Sets the current LED dance to the one specified, if within range, and updates the saved index. This works even
 * if a dance is disabled.
 *
 * @param dance The portableDance_t pointer to update
 * @param danceIndex The index of the dance to select
 * @param wings Whether or not this is specific to the wing LEDS
 * @return true if the dance was within the array range, false if not
 */
bool portableDanceSetByIndex(portableDance_t* dance, uint8_t danceIndex, bool wings)
{
    if (danceIndex >= getNumDances())
    {
        return false;
    }

    if (wings)
    {
        dance->wingDanceIndex = danceIndex;
    }
    else
    {
        dance->danceIndex = danceIndex;
    }
    dance->resetDance = true;

    if (dance->nvsNs != NULL)
    {
        writeNamespaceNvs32(dance->nvsNs, wings ? nvsKeyWingDanceIndex : nvsKeyDanceIndex, danceIndex);
    }
    return true;
}

/**
 * @brief Sets the current LED dance to the one specified, if it exists, and updates the saved index. This works even if
 * a dance is disabled.
 *
 * @param dance The portableDance_t pointer to update
 * @param danceName The name of the dance to select
 * @param wings Whether or not this is specific to the wing LEDS
 * @return true if the dance was found, false if not
 */
bool portableDanceSetByName(portableDance_t* dance, const char* danceName, bool wings)
{
    for (uint8_t i = 0; i < getNumDances(); i++)
    {
        if (!strcmp(dance->dances[i].dance->name, danceName))
        {
            return portableDanceSetByIndex(dance, i, wings);
        }
    }
    return false;
}

/**
 * @brief Sets the current dance color.
 *
 * @param dance The portableDance_t pointer to update
 * @param color bit packed color with values from 0-254 for R, G, and B.
 * @param wings Whether or not this is specific to the wing LEDS
 */
void portableDanceSetColor(portableDance_t* dance, int32_t color, bool wings)
{
    if (wings)
    {
        dance->wingColor = color;
    }
    else
    {
        dance->color = color;
    }
    if (dance->nvsNs != NULL)
    {
        writeNamespaceNvs32(dance->nvsNs, wings ? nvsKeyDanceWingColor : nvsKeyDanceColor,
                            wings ? dance->wingColor : dance->color);
    }
}

/**
 * @brief Sets the current LED dance speed and persists it to NVS if there is an NVS namespace set.
 *
 * @param dance The portableDance_t pointer to update
 * @param speed The dance speed to set. Dance will run at ::DANCE_SPEED_MULT / speed
 * @param wings Whether or not this is specific to the wing LEDS
 */
void portableDanceSetSpeed(portableDance_t* dance, int32_t speed, bool wings)
{
    if (wings)
    {
        dance->wingSpeed = speed;
    }
    else
    {
        dance->speed = speed;
    }
    if (dance->nvsNs != NULL)
    {
        writeNamespaceNvs32(dance->nvsNs, wings ? nvsKeyWingDanceSpeed : nvsKeyDanceSpeed, speed);
    }
}

/**
 * @brief Switches to the next enabled LED dance and updates the saved index.
 *
 * @param dance The portableDance_t pointer to update
 */
void portableDanceNext(portableDance_t* dance)
{
    uint8_t originalIndex = dance->danceIndex;

    do
    {
        if (dance->danceIndex + 1 >= getNumDances())
        {
            dance->danceIndex = 0;
        }
        else
        {
            dance->danceIndex++;
        }
    } while (!dance->dances[dance->danceIndex].enable && dance->danceIndex != originalIndex);

    dance->resetDance = true;

    if (dance->nvsNs != NULL)
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceIndex, dance->danceIndex);
    }
}

/**
 * @brief Switches to the previous enabled LED dance and updates the saved index.
 *
 * @param dance The portableDance_t pointer to update
 */
void portableDancePrev(portableDance_t* dance)
{
    uint8_t originalIndex = dance->danceIndex;
    do
    {
        if (dance->danceIndex == 0)
        {
            dance->danceIndex = getNumDances() - 1;
        }
        else
        {
            dance->danceIndex--;
        }
    } while (!dance->dances[dance->danceIndex].enable && dance->danceIndex != originalIndex);

    dance->resetDance = true;

    if (dance->nvsNs != NULL)
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceIndex, dance->danceIndex);
    }
}

/**
 * @brief Disables the specified dance, if it exists, causing it to be skipped by portableDanceNext()/Prev()
 *
 * @param dance The portableDance_t pointer to update
 * @param danceName The name of the dance to be disabled
 * @return true if the dance was found and disabled, false if not
 */
bool portableDanceDisableDance(portableDance_t* dance, const char* danceName)
{
    for (uint8_t i = 0; i < getNumDances(); i++)
    {
        if (!strcmp(dance->dances[i].dance->name, danceName))
        {
            dance->dances[i].enable = false;
            return true;
        }
    }

    return false;
}

/**
 * @brief Returns the name of the currently selected LED dance. The string does not need to be freed.
 *
 * @param dance The portableDance_t pointer
 * @return The name of the current dance
 */
const char* portableDanceGetName(portableDance_t* dance)
{
    return dance->dances[dance->danceIndex].dance->name;
}
