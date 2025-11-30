//==============================================================================
// Includes
//==============================================================================

#include "portableDance.h"

//==============================================================================
// Prototypes
//==============================================================================

void portableDanceLoadSetting(portableDance_t* dance);

//==============================================================================
// Const Variables
//==============================================================================

static const char nvsKeyDanceIndex[] = "dance_index";
static const char nvsKeyDanceSpeed[] = "dance_speed";

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
    dance->dances[dance->danceIndex].dance->func((int32_t)elapsedUs * DANCE_SPEED_MULT / dance->speed,
                                                 dance->dances[dance->danceIndex].dance->arg, dance->resetDance);
    dance->resetDance = false;
}

/**
 * @brief Read the portable dance index from NVS
 *
 * @param dance The portableDance_t to set an index in
 */
void portableDanceLoadSetting(portableDance_t* dance)
{
    int32_t danceIndex = 0;
    if (!readNamespaceNvs32(dance->nvsNs, nvsKeyDanceIndex, &danceIndex))
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceIndex, danceIndex);
    }

    if (danceIndex < 0)
    {
        danceIndex = 0;
    }
    else if (danceIndex >= getNumDances())
    {
        danceIndex = getNumDances() - 1;
    }

    dance->danceIndex = (uint8_t)danceIndex;

    int32_t speed = DANCE_SPEED_MULT;
    if (!readNamespaceNvs32(dance->nvsNs, nvsKeyDanceSpeed, &speed))
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceSpeed, speed);
    }
    dance->speed = speed;
}

/**
 * @brief Sets the current LED dance to the one specified, if within range, and updates the saved index. This works even
 * if a dance is disabled.
 *
 * @param dance The portableDance_t pointer to update
 * @param danceIndex The index of the dance to select
 * @return true if the dance was within the array range, false if not
 */
bool portableDanceSetByIndex(portableDance_t* dance, uint8_t danceIndex)
{
    if (danceIndex >= getNumDances())
    {
        return false;
    }

    dance->danceIndex = danceIndex;
    dance->resetDance = true;

    if (dance->nvsNs != NULL)
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceIndex, dance->danceIndex);
    }
    return true;
}

/**
 * @brief Sets the current LED dance to the one specified, if it exists, and updates the saved index. This works even if
 * a dance is disabled.
 *
 * @param dance The portableDance_t pointer to update
 * @param danceName The name of the dance to select
 * @return true if the dance was found, false if not
 */
bool portableDanceSetByName(portableDance_t* dance, const char* danceName)
{
    for (uint8_t i = 0; i < getNumDances(); i++)
    {
        if (!strcmp(dance->dances[i].dance->name, danceName))
        {
            return portableDanceSetByIndex(dance, i);
        }
    }
    return false;
}

/**
 * @brief Sets the current LED dance speed and persists it to NVS if there is an NVS namespace set.
 *
 * @param dance The portableDance_t pointer to update
 * @param speed The dance speed to set. Dance will run at ::DANCE_SPEED_MULT / speed
 */
void portableDanceSetSpeed(portableDance_t* dance, int32_t speed)
{
    dance->speed = speed;
    if (dance->nvsNs != NULL)
    {
        writeNamespaceNvs32(dance->nvsNs, nvsKeyDanceSpeed, speed);
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
