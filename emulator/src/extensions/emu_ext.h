/*! \file emu_ext.h
 *
 * \section emuExt_intro Emulator Extensions
 *
 * Emulator extensions are structs made of callbacks that interface with the emulator to
 * allow functionality to be easily extended without modifying the emulator itself.
 *
 * Any callback functions that are not needed may be set to \c NULL and they will be ignored.
 * Details on the behavior and usage of each of these callbacks is available in ::emuExtension_t.
 *
 * \section emuExt_usage Usage
 *
 * An extension is created by defining an ::emuExtension_t struct. Aside from some restrictions
 * which may be described in each callback function's documentation, these functions are free
 * to make use of any exposed emulator state and functionality to enhance the swadge emulation.
 * These extension may even render to the screen using ::emuExtension_t::fnRenderCb and the drawing
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
 * const emuExtension_t exampleExt = {
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
 * static const emuExtension_t registeredExtensions[] = {
 *     ... // Existing extensions
 *     exampleExt,
 * };
 * \endcode
 */

#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "emu_args.h"

#include "CNFG.h"

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

typedef enum
{
    EMU_MOD_NONE  = 0x00,
    EMU_MOD_ALT   = 0x01,
    EMU_MOD_CTRL  = 0x02,
    EMU_MOD_SHIFT = 0x04,
    EMU_MOD_SUPER = 0x08,
} modKey_t;

/**
 * @brief Normalized key-codes across all platforms for use by emulator extensions
 * These are just the CNFG keycodes renamed so extensions don't have to include rawdraw directly.
 */
typedef enum
{
    EMU_KEY_BACKSPACE     = CNFG_KEY_BACKSPACE,
    EMU_KEY_TAB           = CNFG_KEY_TAB,
    EMU_KEY_CLEAR         = CNFG_KEY_CLEAR,
    EMU_KEY_ENTER         = CNFG_KEY_ENTER,
    EMU_KEY_SHIFT         = CNFG_KEY_SHIFT,
    EMU_KEY_CTRL          = CNFG_KEY_CTRL,
    EMU_KEY_ALT           = CNFG_KEY_ALT,
    EMU_KEY_PAUSE         = CNFG_KEY_PAUSE,
    EMU_KEY_CAPS_LOCK     = CNFG_KEY_CAPS_LOCK,
    EMU_KEY_ESCAPE        = CNFG_KEY_ESCAPE,
    EMU_KEY_SPACE         = CNFG_KEY_SPACE,
    EMU_KEY_PAGE_UP       = CNFG_KEY_PAGE_UP,
    EMU_KEY_PAGE_DOWN     = CNFG_KEY_PAGE_DOWN,
    EMU_KEY_END           = CNFG_KEY_END,
    EMU_KEY_HOME          = CNFG_KEY_HOME,
    EMU_KEY_LEFT_ARROW    = CNFG_KEY_LEFT_ARROW,
    EMU_KEY_TOP_ARROW     = CNFG_KEY_TOP_ARROW,
    EMU_KEY_RIGHT_ARROW   = CNFG_KEY_RIGHT_ARROW,
    EMU_KEY_BOTTOM_ARROW  = CNFG_KEY_BOTTOM_ARROW,
    EMU_KEY_SELECT        = CNFG_KEY_SELECT,
    EMU_KEY_PRINT         = CNFG_KEY_PRINT,
    EMU_KEY_EXECUTE       = CNFG_KEY_EXECUTE,
    EMU_KEY_PRINT_SCREEN  = CNFG_KEY_PRINT_SCREEN,
    EMU_KEY_INSERT        = CNFG_KEY_INSERT,
    EMU_KEY_DELETE        = CNFG_KEY_DELETE,
    EMU_KEY_HELP          = CNFG_KEY_HELP,
    EMU_KEY_LEFT_SUPER    = CNFG_KEY_LEFT_SUPER,
    EMU_KEY_RIGHT_SUPER   = CNFG_KEY_RIGHT_SUPER,
    EMU_KEY_NUM_0         = CNFG_KEY_NUM_0,
    EMU_KEY_NUM_1         = CNFG_KEY_NUM_1,
    EMU_KEY_NUM_2         = CNFG_KEY_NUM_2,
    EMU_KEY_NUM_3         = CNFG_KEY_NUM_3,
    EMU_KEY_NUM_4         = CNFG_KEY_NUM_4,
    EMU_KEY_NUM_5         = CNFG_KEY_NUM_5,
    EMU_KEY_NUM_6         = CNFG_KEY_NUM_6,
    EMU_KEY_NUM_7         = CNFG_KEY_NUM_7,
    EMU_KEY_NUM_8         = CNFG_KEY_NUM_8,
    EMU_KEY_NUM_9         = CNFG_KEY_NUM_9,
    EMU_KEY_NUM_MULTIPLY  = CNFG_KEY_NUM_MULTIPLY,
    EMU_KEY_NUM_ADD       = CNFG_KEY_NUM_ADD,
    EMU_KEY_NUM_SEPARATOR = CNFG_KEY_NUM_SEPARATOR,
    EMU_KEY_NUM_SUBTRACT  = CNFG_KEY_NUM_SUBTRACT,
    EMU_KEY_NUM_DECIMAL   = CNFG_KEY_NUM_DECIMAL,
    EMU_KEY_NUM_DIVIDE    = CNFG_KEY_NUM_DIVIDE,
    EMU_KEY_F1            = CNFG_KEY_F1,
    EMU_KEY_F2            = CNFG_KEY_F2,
    EMU_KEY_F3            = CNFG_KEY_F3,
    EMU_KEY_F4            = CNFG_KEY_F4,
    EMU_KEY_F5            = CNFG_KEY_F5,
    EMU_KEY_F6            = CNFG_KEY_F6,
    EMU_KEY_F7            = CNFG_KEY_F7,
    EMU_KEY_F8            = CNFG_KEY_F8,
    EMU_KEY_F9            = CNFG_KEY_F9,
    EMU_KEY_F10           = CNFG_KEY_F10,
    EMU_KEY_F11           = CNFG_KEY_F11,
    EMU_KEY_F12           = CNFG_KEY_F12,
    EMU_KEY_F13           = CNFG_KEY_F13,
    EMU_KEY_F14           = CNFG_KEY_F14,
    EMU_KEY_F15           = CNFG_KEY_F15,
    EMU_KEY_F16           = CNFG_KEY_F16,
    EMU_KEY_F17           = CNFG_KEY_F17,
    EMU_KEY_F18           = CNFG_KEY_F18,
    EMU_KEY_F19           = CNFG_KEY_F19,
    EMU_KEY_F20           = CNFG_KEY_F20,
    EMU_KEY_F21           = CNFG_KEY_F21,
    EMU_KEY_F22           = CNFG_KEY_F22,
    EMU_KEY_F23           = CNFG_KEY_F23,
    EMU_KEY_F24           = CNFG_KEY_F24,
    EMU_KEY_NUM_LOCK      = CNFG_KEY_NUM_LOCK,
    EMU_KEY_SCROLL_LOCK   = CNFG_KEY_SCROLL_LOCK,
    EMU_KEY_LEFT_SHIFT    = CNFG_KEY_LEFT_SHIFT,
    EMU_KEY_RIGHT_SHIFT   = CNFG_KEY_RIGHT_SHIFT,
    EMU_KEY_LEFT_CONTROL  = CNFG_KEY_LEFT_CONTROL,
    EMU_KEY_RIGHT_CONTROL = CNFG_KEY_RIGHT_CONTROL,
    EMU_KEY_LEFT_ALT      = CNFG_KEY_LEFT_ALT,
    EMU_KEY_RIGHT_ALT     = CNFG_KEY_RIGHT_ALT,
} keyCode_t;

/**
 * @brief The location of a pane within the window.
 *
 */
typedef enum
{
    PANE_LEFT   = 0, ///< Left side pane, extends the entire height of the window
    PANE_RIGHT  = 1, ///< Right side pane, extends the entire height of the window
    PANE_TOP    = 2, ///< Top pane, extends only above the screen and between the side panes
    PANE_BOTTOM = 3, ///< Bottom pane, extends only below the screen and between the side panes
} paneLocation_t;

/**
 * @brief Struct representing a sub-pane in the main emulator window
 *
 */
typedef struct
{
    uint32_t paneW; ///< Width of the pane
    uint32_t paneH; ///< Height of the pane
    uint32_t paneX; ///< X offset of the pane
    uint32_t paneY; ///< Y offset of the pane
} emuPane_t;

/**
 * @brief Struct representing the minimum size and count of a set of panes
 *
 */
typedef struct
{
    uint32_t min;   ///< The minimum width or height of this pane
    uint32_t count; ///< The total number of sub-panes in this pane
} emuPaneMinimum_t;

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
     * @param keycode The key code. This is either a lowercase ASCII value or a keyCode_t constant.
     * @param down true if the key was pressed, or false if the key was released
     * @param modifiers A bitfield representing all the modifier keys currently held down
     * @return A new keycode to replace the event with, or -1 to cancel it, or 0 to do nothing.
     */
    int32_t (*fnKeyCb)(uint32_t keycode, bool down, modKey_t modifiers);

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
     * @param panes A pointer to an array of panes, in the order they were requested
     * @param numPanes The number of panes in the array
     */
    void (*fnRenderCb)(uint32_t winW, uint32_t winH, const emuPane_t* panes, uint8_t numPanes);
} emuExtension_t;

//==============================================================================
// Function Prototypes
//==============================================================================

void initExtensions(emuArgs_t* args);
void deinitExtensions(void);
bool enableExtension(const char* name);
bool disableExtension(const char* name);
void calculatePaneMinimums(emuPaneMinimum_t* paneInfos);
void layoutPanes(int32_t winW, int32_t winH, int32_t screenW, int32_t screenH, emuPane_t* screenPane,
                 uint8_t* screenMult);
void requestPane(const emuExtension_t* ext, paneLocation_t loc, uint32_t minW, uint32_t minH);

void doExtPreFrameCb(uint64_t frame);
void doExtPostFrameCb(uint64_t frame);
int32_t doExtKeyCb(uint32_t keycode, bool down, modKey_t modifiers);
void doExtMouseMoveCb(int32_t x, int32_t y, mouseButton_t buttonMask);
void doExtMouseButtonCb(int32_t x, int32_t y, mouseButton_t button, bool down);
void doExtRenderCb(uint32_t winW, uint32_t winH);
