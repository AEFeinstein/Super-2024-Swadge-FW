/*! \file emu_ext.h
 *
 * \section emuExt_intro Emulator Extensions
 *
 * Emulator extensions are structs made of callbacks that interface with the emulator to
 * allow functionality to be easily extended without modifying the emulator itself.
 *
 * Any callback functions that are not needed may be set to \c NULL and they will be ignored.
 * Details on the behavior and usage of each of these callbacks is available in ::emuCallback_t.
 *
 * \section emuExt_usage Usage
 *
 * An extension is created by defining an ::emuCallback_t struct. Aside from some restrictions
 * which may be described in each callback function's documentation, these functions are free
 * to make use of any exposed emulator state and functionality to enhance the swadge emulation.
 * These extension may even render to the screen using ::emuCallbackT::fnRenderCb and the drawing
 * functionality in \c "rawdraw_sf.h". If \c paneLocation, \c minPaneW, and \c minPaneH are all
 * non-zero, the extension will be assigned a dedicated pane where it can draw anything.
 *
 * In order for the callback to be loaded by the emulator, the callback must be added to the list
 * in \c emu_ext.c and any command-line arguments required must be added to \c emu_args.c.
 *
 * \section emuExt_example Example
 *
 * This extension will simply log everything that happens and draw a frame counter in a small pane
 *
 * \code{.c}
 * #include "emu_ext.h"
 *
 * static void exampleExtInit(emuArgs_t* emuArgs);
 * static void exampleExtPreFrame(uint64_t frameNum);
 * static void exampleExtPostFrame(uint64_t frameNum);
 * static int32_t exampleExtKey(uint32_t keycode, bool down);
 * static bool exampleExtMouseMove(int32_t x, int32_t y, mouseButton_t buttonMask);
 * static bool exampleExtMouseButton(int32_t x, int32_t y, mouseButton_t button, bool down);
 * static void exampleExtRender(uint32_t winW, uint32_t winH, uint32_t paneW, uint32_t paneH, uint32_t paneX, uint32_t
 * paneY);
 *
 * const emuCallback_t exampleExt = {
 *     .name            = "Example",
 *     .paneLocation    = PANE_BOTTOM,
 *     .minPaneW        = 50,
 *     .minPaneH        = 50,
 *     .fnInitCb        = exampleExtInit,
 *     .fnPreFrameCb    = exampleExtPreFrame,
 *     .fnPostFrameCb   = exampleExtPostFrame,
 *     .fnKeyCb         = exampleExtKeyCb,
 *     .fnMouseMoveCb   = exampleExtMouseMove,
 *     .fnMouseButtonCb = exampleExtMouseButton,
 *     .fnRenderCb      = exampleExtRender,
 * };
 * \endcode
 *
 * And then in \c emu_ext.c, add it to the list:
 * \code{.c}
 * // Extension Includes
 * // ...
 * #include "example_ext.h"
 *
 * static const emuCallback_t registeredCallbacks[] = {
 *     ... / Existing extensions
 *     exampleExt,
 * };
 * \endcode
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "emu_args.h"

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief Enum defining possible mouse buttons used in callbacks
 *
 *  TODO: Are these platform-specific?
 */
typedef enum
{
    EMU_MOUSE_LEFT   = 0x01,
    EMU_MOUSE_MIDDLE = 0x02,
    EMU_MOUSE_RIGHT  = 0x04,
    EMU_SCROLL_UP    = 0x08,
    EMU_SCROLL_DOWN  = 0x10,
    EMU_SCROLL_LEFT  = 0x20,
    EMU_SCROLL_RIGHT = 0x40,
} mouseButton_t;

/**
 * @brief The location of a pane within the window, or no pane.
 *
 */
typedef enum
{
    PANE_NONE   = 0,
    PANE_LEFT   = 1,
    PANE_RIGHT  = 2,
    PANE_TOP    = 3,
    PANE_BOTTOM = 4,
} paneLocation_t;

/**
 * @brief Struct for holding various callbacks used to extend emulator functionality.
 *
 * Any callback value may be left NULL and it will just not be called.
 *
 */
typedef struct
{
    /**
     * @brief A name for this extension.
     *
     */
    const char* name;

    /**
     * @brief The location for this callback's pane, or 0 for none
     *
     */
    paneLocation_t paneLocation;

    /**
     * @brief The minimum wuidth in pixels for this extension's pane, or 0 for none
     *
     */
    uint32_t minPaneW;

    /**
     * @brief The minimum height in pixels for this extension's pane, or 0 for none
     *
     */
    uint32_t minPaneH;

    /**
     * @brief Function to be called once upon startup with the parsed command-line args
     *
     * This callback should only be used to initialize the callback's internal data, and
     * should not attempt to use any emulator functionality as it may not yet be initialized
     * at the time this callback is called.
     *
     * The callback may return
     * @return true if this callback was initialized successfully
     * @return false if this callback was not initialized or is disabled
     */
    bool (*fnInitCb)(emuArgs_t* emuArgs);

    /**
     * @brief Function to be called before an emulator frame is rendered
     *
     * This is the ideal location to inject button presses and other events into the emulator,
     * as it will be called immediately before the swadge's main loop executes
     *
     */
    void (*fnPreFrameCb)(uint64_t frame);

    /**
     * @brief Function to be called after an emulator frame is rendered
     *
     */
    void (*fnPostFrameCb)(uint64_t frame);

    /**
     * @brief Function to be called whenever a key is pressed.
     *
     * To allow the key press to be sent to other callbacks and the emulator, return 0.
     * To consume the key press and stop it from being sent to the emulator, return a negative value.
     * To replace the key press with a new one, return the new key code.
     *
     * TODO document the modifier keys, for all our sake
     *
     * @param keycode The key code. This is an ASCII character ORed with constants for modifier keys.
     * @param down true if the key was pressed, or false if the key was released
     * @return A new keycode to replace the event with, or -1 to cancel it, or 0 to do nothing.
     */
    int32_t (*fnKeyCb)(uint32_t keycode, bool down);

    /**
     * @brief Function to be called whenever the mouse moves.
     *
     * @param x The new X position of the mouse
     * @param y The new Y position of the mouse
     * @param buttonMask A bitwise mask of all the buttons that were held down during the move event
     * @return true to stop further propagation of this event
     *
     */
    bool (*fnMouseMoveCb)(int32_t x, int32_t y, mouseButton_t buttonMask);

    /**
     * @brief Function to be called whenever a mouse button is pressed or released
     *
     * @param x The mouse X position
     * @param y The mouse Y position
     * @param button A single button value for the button that was pressed or released
     * @param down true if the button was pressed, false if it was released
     * @return true to stop further propagation of this event
     */
    bool (*fnMouseButtonCb)(int32_t x, int32_t y, mouseButton_t button, bool down);

    /**
     * @brief Function to be called to render the UI, optionally in a pane.
     *
     * If this callback is associated with a pane, its dimensions and location are included.
     * Otherwise, paneW, paneH, paneX, and paneY will be zero.
     *
     * @param winW The window width, in pixels
     * @param winH The window height, in pixels
     * @param paneW The pane's width, or zero if the callback has no pane associated
     * @param paneH The pane's height, or zero if the callback has no pane associated
     * @param paneX The pane's X offset within the main window
     * @param paneY The pane's Y offset within the main window
     */
    void (*fnRenderCb)(uint32_t winW, uint32_t winH, uint32_t paneW, uint32_t paneH, uint32_t paneX, uint32_t paneY);
} emuCallback_t;

//==============================================================================
// Function Prototypes
//==============================================================================

/**
 * @brief Get the list of emulator callbacks and return the number via the \c count out-param
 * @param count A pointer to an int to be updated with the number of items in the returned emuCallback_t* list
 * @return emuCallback_t* A pointer to a list of emuCallback_t containing \c *count items
 */
const emuCallback_t** getEmuCallbacks(int* count);
