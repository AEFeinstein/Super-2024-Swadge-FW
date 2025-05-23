/*! \file cosCrunch.h
 *
 * \section whats_a_microgame What's A Microgame?
 *
 * Microgames are single-screen, single-objective games lasting around 5 seconds, designed to be presented to the player
 * in rapid-fire fashion. Cosplay Crunch is a collection of cosplay- and crafting-themed microgames, intended to mirror
 * the frantic pace of last-minute crafting.
 *
 * \section design_considerations Design Considerations
 *
 * Microgames should have a cosplay/crafting theme, but eclectic visual, audio, and gameplay design is encouraged.
 * Gameplay-wise, microgames should have a single objective that is clearly conveyed to the player through choice of
 * verb and visual cues. A frustrated player is not a returning player. Accessibility is also important to consider,
 * e.g., don't rely on audio being heard.
 *
 * \section writing_a_microgame Writing A Microgame
 *
 * Microgames are implemented similarly to pared-down Swadge modes: they have a similar init/main loop/deinit cycle that
 * is managed by the Cosplay Crunch game mode. Microgames go through a series of states as defined in
 * ::cosCrunchMicrogameState, allowing them time before gameplay to display helpful animations and time after gameplay
 * to congratulate or taunt the player. Once a player succeeds or fails the microgame, send the result with
 * cosCrunchMicrogameResult() to signify that the microgame is over. If you don't send a result, the game will be failed
 * once the timer runs out.
 */

#pragma once

#include "swadge2024.h"

extern swadgeMode_t cosCrunchMode;

typedef enum
{
    /**
     * @brief This state is shown briefly with the game's verb overlaid before the game starts to orient the player.
     * The game should render but ignore player input.
     */
    CC_MG_GET_READY,

    /**
     * @brief The game is in progress: accept input, render the game, all that jazz.
     */
    CC_MG_PLAYING,

    /**
     * @brief The player has succeeded. Display something that elevates their spirits.
     */
    CC_MG_CELEBRATING,

    /**
     * @brief The player has failed. Display something that underscores their torment.
     */
    CC_MG_DESPAIRING,
} cosCrunchMicrogameState;

/**
 * @brief The definition for a Cosplay Crunch microgame. Once your microgame is built, add this struct to the
 * `microgames[]` array in `cosCrunch.c` to include it in the randomized rotation. While testing a microgame, you can
 * comment out the other microgames in the array to play only yours on repeat.
 */
typedef struct
{
    /**
     * @brief The phrase that is flashed on the screen before the microgame runs. Should be an action: "Sew", "Glue",
     * "Try not to cry", etc.
     */
    const char* verb;

    /**
     * @brief How long the player has until they fail the microgame, in microseconds.
     */
    uint64_t timeoutUs;

    /**
     * @brief This function is called when this microgame is about to be started. It should initialize variables and
     * load assets.
     */
    void (*fnInitMicrogame)(void);

    /**
     * @brief This function is called when the microgame is exited. It should free any allocated memory.
     */
    void (*fnDestroyMicrogame)(void);

    /**
     * @brief This function is called from the main loop. The microgame should run its game logic and render like a mode
     * would. This function should NOT call checkButtonQueueWrapper() like a mode would: it will receive all button
     * events as an argument.
     *
     * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
     * it's time to do things
     * @param timeRemainingUs The time left until the game is failed
     * @param state The current game state
     * @param buttonEvts Button events that have occurred since the last main loop call
     * @param buttonEvtCount The number of button events in the buttonEvts array
     */
    void (*fnMainLoop)(int64_t elapsedUs, uint64_t timeRemainingUs, cosCrunchMicrogameState state,
                       buttonEvt_t buttonEvts[], uint8_t buttonEvtCount);
} cosCrunchMicrogame_t;

/**
 * @brief Submit the player's result of the currently running microgame. Every microgame must have a success condition,
 * but a failure condition is optional. The microgame timer running out will automatically send a failure result.
 *
 * @param successful Whether the player completed the game or failed
 */
void cosCrunchMicrogameResult(bool successful);
