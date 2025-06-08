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
 * verb and visual cues. A frustrated player is not a returning player. Keep in mind that a microgame that seems
 * well-balanced when played in isolation might feel unfair when the player is rapidly context switching between games.
 * Accessibility is also important to consider, e.g., don't rely on audio being heard.
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

#include "cosCrunchUtil.h"
#include "swadge2024.h"

extern swadgeMode_t cosCrunchMode;

/// The height in pixels of the area that microgames can draw on without invading the UI space. Note that UI elements
/// (timer and calendar, and desk if fnBackgroundDrawCallback is implemented) will still draw on top of microgames.
#define CC_DRAWABLE_HEIGHT (TFT_HEIGHT - 38)

typedef enum
{
    /**
     * @brief This state is shown briefly with the game's verb overlaid before the game starts to orient the player.
     * The game should render and can accept player input to telegraph what buttons will do, but should not use the
     * input to affect the game state yet.
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
     * @brief The phrase that is flashed on the screen after the microgame if the game was succeeded. This can be NULL
     * if you'd rather congratulate the player in a different manner.
     */
    const char* successMsg;

    /**
     * @brief The phrase that is flashed on the screen after the microgame if the game was failed. This can be NULL
     * if you'd rather taunt the player in a different manner.
     */
    const char* failureMsg;

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

    /**
     * @brief This function is called when the display driver wishes to update a section of the display. Implementing
     * this function in your microgame is optional and will cause the Cosplay Crunch UI to render over it in the
     * foreground. If you do not implement this function, your game will render on top of the cutting mat background.
     *
     * Note that your microgame will not receive this callback for the entire screen: any pixels at the bottom of the
     * display that are covered up by the Cosplay Crunch UI will be skipped.
     *
     * @param x The x coordinate that should be updated
     * @param y The y coordinate that should be updated
     * @param w The width of the rectangle to be updated
     * @param h The height of the rectangle to be updated
     * @param up Update number
     * @param upNum Update number denominator
     */
    void (*fnBackgroundDrawCallback)(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

    /**
     * @brief This function is called when the microgame timer expires. It is optional and intended for microgames that
     * need to determine their success condition at the end of the game. If you implement this function, you can still
     * call \ref cosCrunchMicrogameResult() at any time to complete the game early.
     *
     * @return `true` if the microgame was completed successfully, `false` if it was failed
     */
    bool (*fnMicrogameTimeout)(void);
} cosCrunchMicrogame_t;

/**
 * @brief Get a color set (base/highlight/lowlight) suitable for greyscale tinting to use for this microgame. Using tint
 * colors is optional, but microgames that can easily swap colors (painting, drawing, etc.), especially if the colors
 * can spill over onto the background, are encouraged to do so.
 *
 * This function returns a different color set every time, so microgames should only call this once to get the color
 * assigned to them.
 *
 * @return A pointer to a tint color set
 */
const tintColor_t* cosCrunchMicrogameGetTintColor(void);

/**
 * @brief Get a palette suitable for tinting a greyscale image using the `drawWsgPalette*()` functions from \ref
 * wsgPalette.h
 *
 * @param tintColor
 * @return A pointer to a palette that can be used to tint a wsg
 */
wsgPalette_t* cosCrunchMicrogameGetWsgPalette(const tintColor_t* tintColor);

/**
 * @brief Microgames can leave paint, marker, glitter, and whatever else on the mat that persists across a play session.
 * This will copy whatever is in your wsg to the background, where it will stay until it gets drawn over by another
 * microgame or the current game session ends.
 *
 * Your microgame's destroy callback is a good place to call this from, since no drawing takes place while games are
 * being switched out. You can also call this multiple times during gameplay if building up splatter
 * incrementally would work better for your microgame.
 *
 * @param wsg An image with transparency to be copied onto the background
 * @param x Screen x coordinate to draw
 * @param y Screen y coordinate to draw
 */
void cosCrunchMicrogamePersistSplatter(wsg_t wsg, uint16_t x, uint16_t y);

/**
 * @brief Submit the player's result of the currently running microgame. Every microgame must have a success condition,
 * but a failure condition is optional. The microgame timer running out will automatically send a failure result unless
 * you implement \ref cosCrunchMicrogame_t::fnMicrogameTimeout.
 *
 * @param successful Whether the player completed the game or failed
 */
void cosCrunchMicrogameResult(bool successful);
