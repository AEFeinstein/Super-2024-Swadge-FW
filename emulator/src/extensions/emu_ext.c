//==============================================================================
// Includes
//==============================================================================
#include "emu_ext.h"
#include "emu_args.h"
#include "macros.h"
#include "linked_list.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Extension Includes
#include "ext_touch.h"
#include "ext_leds.h"
#include "ext_fuzzer.h"
#include "ext_gamepad.h"
#include "ext_keymap.h"
#include "ext_midi.h"
#include "ext_modes.h"
#include "ext_replay.h"
#include "ext_tools.h"
#include "ext_screensaver.h"

//==============================================================================
// Registered Extensions
//==============================================================================

//==============================================================================
// ADD ALL EXTENSIONS HERE IN ORDER TO REGISTER THEM
//==============================================================================

static const emuExtension_t* registeredExtensions[] = {
    &touchEmuCallback,  &ledEmuExtension,     &fuzzerEmuExtension, &toolsEmuExtension, &keymapEmuCallback,
    &modesEmuExtension, &gamepadEmuExtension, &replayEmuExtension, &midiEmuExtension,  &screensaverEmuExtension,
};

//==============================================================================
// Macros
//==============================================================================

#define EMU_CB_LOOP_BARE    for (node_t* node = extManager.extensions.first; node != NULL; node = node->next)
#define EMU_CB_INFO         ((emuExtInfo_t*)(node->val))
#define EMU_CB_HAS_FN(cbFn) (EMU_CB_INFO->extension && EMU_CB_INFO->extension->cbFn)

/**
 * @brief Macro to be used as a for-loop replacement for calling a particular callback
 *
 * This macro must only be used after ::EMU_CB_SETUP has been used in the current scope.
 * Use this macro as though it were \c for(;;) initializer -- with braces. The body will
 * only operate on ::emuCallback_t for which \c cbFn is not NULL.
 *
 * Example:
 * \code{.c}
 * void doCallbacks(void)
 * {
 *     EMU_CB_SETUP
 *     EMU_CB_LOOP(fnPreFrameCb)
 *     {
 *         EMU_CB(fnPreFrameCb);
 *     }
 * }
 * \endcode
 */
#define EMU_CB_LOOP(cbFn) \
    EMU_CB_LOOP_BARE      \
    if (EMU_CB_INFO && EMU_CB_INFO->enabled && EMU_CB_HAS_FN(cbFn))

/**
 * @brief Macro used to call \c cbFn on the current callback with any args inside of an ::EMU_CB_LOOP loop.
 */
#define EMU_CB(cbFn, ...) EMU_CB_INFO->extension->cbFn(__VA_ARGS__)

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief Struct representing a sub-pane, its location, and minimum size
 *
 */
typedef struct
{
    paneLocation_t loc; ///< Location of the pane
    uint32_t minW;      ///< Requested minimum width of the pane
    uint32_t minH;      ///< Requested minimum height of the pane
    emuPane_t pane;
} emuPaneInfo_t;

typedef struct
{
    bool enabled;
    bool initialized;
    const emuExtension_t* extension;
    list_t panes;
} emuExtInfo_t;

typedef struct
{
    bool extsLoaded;
    list_t extensions;
    emuPaneMinimum_t paneMinimums[4];
    bool paneMinsCalculated;
} emuExtManager_t;

//==============================================================================
// Variables
//==============================================================================

static emuExtManager_t extManager = {0};

//==============================================================================
// Static Function Prototypes
//==============================================================================

static emuExtInfo_t* findExtInfo(const emuExtension_t* ext);
static const emuExtension_t* findExt(const char* name);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief
 *
 * @param ext
 * @return emuExtInfo_t*
 */
static emuExtInfo_t* findExtInfo(const emuExtension_t* ext)
{
    if (NULL == ext)
    {
        return NULL;
    }

    node_t* node = extManager.extensions.first;

    while (node != NULL)
    {
        emuExtInfo_t* info = (emuExtInfo_t*)(node->val);

        if (NULL != info && info->extension == ext)
        {
            return info;
        }

        node = node->next;
    }

    return NULL;
}

/**
 * @brief
 *
 * @param name
 * @return const emuExtension_t*
 */
static const emuExtension_t* findExt(const char* name)
{
    const emuExtension_t** cbList = registeredExtensions;

    for (int i = 0; i < ARRAY_SIZE(registeredExtensions); i++)
    {
        if (cbList[i] && !strncmp(name, cbList[i]->name, strlen(name)))
        {
            // Match!
            return cbList[i];
        }
    }

    return NULL;
}

/**
 * @internal
 * @brief Sets up the extension list without initilaizing any extensions.
 *
 * This just makes sure the list is always in the correct order.
 */
static void preloadExtensions(void)
{
    if (extManager.extsLoaded)
    {
        return;
    }

    const emuExtension_t** cbList = registeredExtensions;

    for (int i = 0; i < ARRAY_SIZE(registeredExtensions); i++)
    {
        const emuExtension_t* ext = cbList[i];
        emuExtInfo_t* info        = calloc(1, sizeof(emuExtInfo_t));
        info->extension           = ext;

        push(&extManager.extensions, info);
    }

    extManager.extsLoaded = true;
}

/**
 * @brief Initializes all registered emulator extensions
 *
 * @param args
 */
void initExtensions(emuArgs_t* args)
{
    preloadExtensions();

    node_t* extNode = extManager.extensions.first;
    while (NULL != extNode)
    {
        emuExtInfo_t* info        = (emuExtInfo_t*)(extNode->val);
        const emuExtension_t* ext = info->extension;

        if (ext->fnInitCb)
        {
            printf("Extension %s initializing...\n", ext->name);
            info->enabled = ext->fnInitCb(args);
            printf("%s!\n", info->enabled ? "enabled" : "disabled");
        }
        else
        {
            printf("Extension %s enabled!\n", ext->name);
            info->enabled = true;
        }
        info->initialized = true;

        extNode = extNode->next;
    }
}

/**
 * @brief Deinitializes all registered emulator extensions
 *
 */
void deinitExtensions(void)
{
    // Remove each extension from the list and clean it up
    emuExtInfo_t* extInfo;
    while ((extInfo = (emuExtInfo_t*)pop(&extManager.extensions)))
    {
        // If we had a deinit extension callback, we'd call it here
        // Clean up the extension's panes, then the extension
        emuPaneInfo_t* paneInfo;
        while ((paneInfo = (emuPaneInfo_t*)pop(&extInfo->panes)))
        {
            free(paneInfo);
        }

        if (extInfo->initialized)
        {
            if (extInfo->extension->fnDeinitCb)
            {
                extInfo->extension->fnDeinitCb();
            }
        }
        free(extInfo);
    }
}

/**
 * @brief Enable the extension
 *
 * @param name
 */
bool enableExtension(const char* name)
{
    preloadExtensions();

    emuExtInfo_t* extInfo = findExtInfo(findExt(name));
    if (NULL != extInfo)
    {
        if (!extInfo->initialized || !extInfo->enabled)
        {
            if (extInfo->extension->fnInitCb)
            {
                extInfo->extension->fnInitCb(&emulatorArgs);
                // Should we ignore the return value? I guess we're force-enabling it?
            }

            extInfo->initialized = true;
        }

        // If the extension had panes, we will need to recalculate
        if (extInfo->panes.length > 0)
        {
            extManager.paneMinsCalculated = false;
        }

        extInfo->enabled = true;

        return true;
    }

    return false;
}

/**
 * @brief
 *
 * @param name
 */
bool disableExtension(const char* name)
{
    preloadExtensions();

    emuExtInfo_t* extInfo = findExtInfo(findExt(name));
    if (NULL != extInfo)
    {
        extInfo->enabled = false;

        // Unload the extension's frames
        emuPaneInfo_t* paneInfo;
        while ((paneInfo = (emuPaneInfo_t*)pop(&extInfo->panes)))
        {
            free(paneInfo);
        }

        if (extInfo->initialized)
        {
            if (extInfo->extension->fnDeinitCb)
            {
                extInfo->extension->fnDeinitCb();
            }
        }

        // If the extension had panes, we will need to recalculate
        extManager.paneMinsCalculated = false;

        return true;
    }

    return false;
}

/**
 * @brief Request a pane at \c loc to be passed to the extension when rendering.
 *
 * Each extension can request any number of panes and in any location. When ::emuExtension_t::fnRenderCb
 * is called, all requested panes will be passed in the same order in which they were requested. Panes
 * will be laid out left-to-right and top-to-bottom within their locations, first by the registration
 * order of their extensions, and then by their request order.
 *
 * @param ext  A pointer to the extension requesting the pane
 * @param loc  Which edge of the screen the pane should be placed in
 * @param minW The minimum width of the pane
 * @param minH The minimum height of the pane
 * @return An ID number for the requested pane
 */
int requestPane(const emuExtension_t* ext, paneLocation_t loc, uint32_t minW, uint32_t minH)
{
    emuExtInfo_t* extInfo = findExtInfo(ext);
    if (NULL != extInfo)
    {
        emuPaneInfo_t* paneInfo = calloc(1, sizeof(emuPaneInfo_t));

        paneInfo->loc          = loc;
        paneInfo->minW         = minW;
        paneInfo->minH         = minH;
        paneInfo->pane.visible = true;
        paneInfo->pane.id      = extInfo->panes.length;

        push(&extInfo->panes, paneInfo);
        extManager.paneMinsCalculated = false;

        return paneInfo->pane.id;
    }
    return -1;
}

/**
 * @brief Sets the visibility of a panel using its ID
 *
 * @param ext The extension for the pane
 * @param paneId The ID of the pane, as returned by requestPane()
 * @param visible The visibilty to set
 */
void setPaneVisibility(const emuExtension_t* ext, int paneId, bool visible)
{
    emuExtInfo_t* extInfo = findExtInfo(ext);
    if (NULL != extInfo)
    {
        node_t* node = extInfo->panes.first;
        for (int i = 0; node != NULL && i < paneId; i++)
        {
            node = node->next;
        }

        if (node != NULL)
        {
            emuPaneInfo_t* pane           = node->val;
            pane->pane.visible            = visible;
            extManager.paneMinsCalculated = false;
        }
    }
}

/**
 * @brief Helper function to calculate the minimum size needed for the extension panes
 *
 * The results of the calculation will be written into \c paneMinimums at the index
 * matching the value of each extension's panes' ::paneLocation_t.
 *
 * @param[out] paneMinimums A pointer to a 0-initialized array of at least 4 ::emuPaneMinimum_t to be used as output.
 * @return true if the pane minimums have changed
 * @return false if the pane minimums did not change

 */
bool calculatePaneMinimums(emuPaneMinimum_t* paneMinimums)
{
    // Figure out the minimum required height/width of each pane side
    // For this we don't need to know anything about the actual window dimensions yet

    bool result = false;

    // Check if the cached pane minimums are still good
    if (!extManager.paneMinsCalculated)
    {
        // Reset the pane minimums so they don't ratchet up
        memset(extManager.paneMinimums, 0, sizeof(extManager.paneMinimums));

        // Iterate over all the extensions
        // Update the cached pane minimums
        node_t* extNode = extManager.extensions.first;
        while (NULL != extNode)
        {
            // Make sure the extension is enabled and has at least one pane
            emuExtInfo_t* extInfo = (emuExtInfo_t*)(extNode->val);
            if (extInfo->enabled && extInfo->panes.length > 0)
            {
                // Iterate over all the extension's panes
                node_t* paneNode = extInfo->panes.first;
                while (NULL != paneNode)
                {
                    // Make sure each pane isn't hidden
                    emuPaneInfo_t* paneInfo = (emuPaneInfo_t*)(paneNode->val);
                    if (paneInfo->pane.visible)
                    {
                        // Get the minumums for this pane's location
                        emuPaneMinimum_t* locMin = (extManager.paneMinimums + paneInfo->loc);

                        // Now, just calculate either the minimum width for side panes,
                        // or the minimum height for top/bottom panes
                        switch (paneInfo->loc)
                        {
                            case PANE_LEFT:
                            case PANE_RIGHT:
                            {
                                locMin->min = MAX(locMin->min, paneInfo->minW);
                                locMin->count++;
                                // minLeftPaneH += cbList[i]->minPaneH;
                                // minRightPaneH += cbList[i]->minPaneH;
                                break;
                            }

                            case PANE_TOP:
                            case PANE_BOTTOM:
                            {
                                locMin->min = MAX(locMin->min, paneInfo->minH);
                                locMin->count++;
                                // minTopPaneW += cbList[i]->minPaneW;
                                // minBottomPaneW += cbList[i]->minPaneH
                                break;
                            }
                        }
                    }

                    paneNode = paneNode->next;
                }
            }
            extNode = extNode->next;
        }

        // Cache is valid again!
        extManager.paneMinsCalculated = true;
        result                        = true;
    }

    // Copy the cached pane minimums to the output
    memcpy(paneMinimums, extManager.paneMinimums, sizeof(extManager.paneMinimums));
    return result;
}

/**
 * @brief Calculates the position of all extension panes and the display and updates their locations and sizes.
 *
 * @param winW The total window width, in pixels.
 * @param winH The total window height, in pixels
 * @param screenW The actual width of the emulator screen, in pixels
 * @param screenH The actual height of the emulator screen, in pixels
 * @param[out] screenPane A pointer to an ::emuPane_t to be updated with the screen location and dimensions
 * @param[out] screenMult A pointer to an int to be updated with the screen scale multiplier
 */
void layoutPanes(int32_t winW, int32_t winH, int32_t screenW, int32_t screenH, emuPane_t* screenPane,
                 uint8_t* screenMult)
{
    emuPaneMinimum_t paneInfos[4];
    calculatePaneMinimums(paneInfos);

    // Figure out how much the screen should be scaled by
    uint8_t widthMult = (winW - paneInfos[PANE_LEFT].min - paneInfos[PANE_RIGHT].min) / screenW;
    if (0 == widthMult)
    {
        widthMult = 1;
    }

    uint8_t heightMult = (winH - paneInfos[PANE_TOP].min - paneInfos[PANE_BOTTOM].min) / screenH;
    if (0 == heightMult)
    {
        heightMult = 1;
    }

    // Set the scale to whichever dimension's multiplier was smallest
    *screenMult = MIN(widthMult, heightMult);

    // Update the screen pane size to the scaled size of the screen
    screenPane->paneW = screenW * (*screenMult);
    screenPane->paneH = screenH * (*screenMult);

    // These will hold the overall pane dimensions for easier logic
    emuPane_t winPanes[4] = {0};

    // The number of panes assigned in each area, for positioning subpanes
    uint8_t assigned[4] = {0};

    // Width/height of the dividers between the screen and each pane, if there are any
    uint32_t leftDivW   = paneInfos[PANE_LEFT].count > 0;
    uint32_t rightDivW  = paneInfos[PANE_RIGHT].count > 0;
    uint32_t topDivH    = paneInfos[PANE_TOP].count > 0;
    uint32_t bottomDivH = paneInfos[PANE_BOTTOM].count > 0;

    // Assign the remaining space to the left and right panes proportionally with their minimum sizes
    winPanes[PANE_LEFT].paneX = 0;
    winPanes[PANE_LEFT].paneY = 0;

    // Only set the pane dimensions if there are actually any panes, to avoid division-by-zero
    if (paneInfos[PANE_LEFT].count > 0)
    {
        winPanes[PANE_LEFT].paneW = (winW - rightDivW - leftDivW - screenPane->paneW) * (paneInfos[PANE_LEFT].min)
                                    / (paneInfos[PANE_LEFT].min + paneInfos[PANE_RIGHT].min);
        winPanes[PANE_LEFT].paneH = winH;
    }
    else
    {
        winPanes[PANE_LEFT].paneW = 0;
        winPanes[PANE_LEFT].paneH = 0;
    }

    // The screen will be just to the right of the left pane and its divider
    screenPane->paneX = winPanes[PANE_LEFT].paneW + leftDivW;

    // Assign whatever space is left to the right pane to account for rounding problems
    winPanes[PANE_RIGHT].paneX = screenPane->paneX + screenPane->paneW + rightDivW;
    winPanes[PANE_RIGHT].paneY = 0;
    if (paneInfos[PANE_RIGHT].count > 0)
    {
        winPanes[PANE_RIGHT].paneW = winW - (winPanes[PANE_LEFT].paneW + leftDivW + screenPane->paneW + rightDivW);
        winPanes[PANE_RIGHT].paneH = winH;
    }
    else
    {
        winPanes[PANE_RIGHT].paneW = 0;
        winPanes[PANE_RIGHT].paneH = 0;
    }

    // Now do the horizontal panes, which have the same X and W as the screen
    winPanes[PANE_TOP].paneX = screenPane->paneX;
    winPanes[PANE_TOP].paneY = 0;
    if (paneInfos[PANE_TOP].count > 0)
    {
        winPanes[PANE_TOP].paneW = screenPane->paneW;
        // Assign the remaining space to the left and right panes proportionally with their minimum sizes
        winPanes[PANE_TOP].paneH = (winH - bottomDivH - topDivH - screenPane->paneH) * (paneInfos[PANE_TOP].min)
                                   / (paneInfos[PANE_TOP].min + paneInfos[PANE_BOTTOM].min);
    }

    // For the bottom one, flip things around just a bit so we can center the screen properly
    if (paneInfos[PANE_BOTTOM].count > 0)
    {
        winPanes[PANE_BOTTOM].paneW = screenPane->paneW;
        // Assign whatever space is left to the right pane to account for roundoff
        winPanes[PANE_BOTTOM].paneH = winH - (winPanes[PANE_TOP].paneH + topDivH + screenPane->paneH + bottomDivH);
    }

    // The screen will be just below the top pane and its divider, plus half of any extra space not used by the panes
    // (to center)
    screenPane->paneY
        = winPanes[PANE_TOP].paneH + topDivH
          + (winH - winPanes[PANE_TOP].paneH - winPanes[PANE_BOTTOM].paneH - screenPane->paneH - topDivH - bottomDivH)
                / 2;

    // Center the screen in the window if it's bigger
    if (paneInfos[PANE_LEFT].count == 0 && paneInfos[PANE_RIGHT].count == 0)
    {
        screenPane->paneX = (winW - screenPane->paneW) / 2;

        winPanes[PANE_BOTTOM].paneX = 0;
        winPanes[PANE_TOP].paneX    = 0;

        winPanes[PANE_BOTTOM].paneW = winW;
        winPanes[PANE_TOP].paneW    = winW;
    }
    else
    {
        winPanes[PANE_BOTTOM].paneX = screenPane->paneX;
        winPanes[PANE_TOP].paneX    = screenPane->paneX;
    }
    winPanes[PANE_BOTTOM].paneY = screenPane->paneY + screenPane->paneH + bottomDivH;

///< Macro for calculating the offset of the current sub-pane within the overall pane
#define SUBPANE_OFFSET(side, hOrW) \
    ((paneInfos[side].count > 0) ? (assigned[side] * winPanes[side].pane##hOrW / paneInfos[side].count) : 0)

///< Macro for calculating the size of the currunt sub-pane within the overall pane
#define SUBPANE_SIZE(side, hOrW)                                                                                   \
    ((paneInfos[side].count > 0)                                                                                   \
         ? ((assigned[side] + 1) * winPanes[side].pane##hOrW / paneInfos[side].count - SUBPANE_OFFSET(side, hOrW)) \
         : 0)

    // One difference between the left/right and top/bottom sides that's now important:
    // Left/Right panes get the entire side
    // Top/Bottom panes only get the space under the screen...

    // Now, we actually apply all the dimensions we calculated to the panes
    // Iterate over all the extensions
    node_t* extNode = extManager.extensions.first;
    while (NULL != extNode)
    {
        // Make sure the extension is enabled and has at least one pane
        emuExtInfo_t* extInfo = (emuExtInfo_t*)(extNode->val);
        if (extInfo->enabled && extInfo->panes.length > 0)
        {
            // Iterate over all the extension's panes
            node_t* paneNode = extInfo->panes.first;
            while (NULL != paneNode)
            {
                emuPaneInfo_t* paneInfo = (emuPaneInfo_t*)(paneNode->val);
                emuPane_t* cbPane       = &(paneInfo->pane);

                // Copy the overall pane settings for the appropriate side onto the sub-pane for this callback
                cbPane->paneX = winPanes[paneInfo->loc].paneX;
                cbPane->paneW = winPanes[paneInfo->loc].paneW;
                cbPane->paneY = winPanes[paneInfo->loc].paneY;
                cbPane->paneH = winPanes[paneInfo->loc].paneH;

                switch (paneInfo->loc)
                {
                    case PANE_LEFT:
                    case PANE_RIGHT:
                    {
                        // Handle the left/right columns
                        // We just set the Y and Height
                        cbPane->paneY += SUBPANE_OFFSET(paneInfo->loc, H);
                        cbPane->paneH = SUBPANE_SIZE(paneInfo->loc, H);
                        break;
                    }

                    case PANE_TOP:
                    case PANE_BOTTOM:
                    {
                        cbPane->paneX += SUBPANE_OFFSET(paneInfo->loc, W);
                        cbPane->paneW = SUBPANE_SIZE(paneInfo->loc, W);
                        break;
                    }
                }

                assigned[paneInfo->loc]++;

                paneNode = paneNode->next;
            }
        }

        extNode = extNode->next;
    }

#undef SUBPANE_OFFSET
#undef SUBPANE_SIZE
}

/**
 * @brief Calls the pre-frame callback for all enabled extensions
 *
 * @param frame The absolute frame number
 */
void doExtPreFrameCb(uint64_t frame)
{
    EMU_CB_LOOP(fnPreFrameCb)
    {
        EMU_CB(fnPreFrameCb, frame);
    }
}

/**
 * @brief Calls the post-frame callback for all enabled extensions
 *
 * @param frame The absolute frame number
 */
void doExtPostFrameCb(uint64_t frame)
{
    EMU_CB_LOOP(fnPostFrameCb)
    {
        EMU_CB(fnPostFrameCb, frame);
    }
}

/**
 * @brief Calls the key callback for all enabled extensions
 *
 * @param keycode The original event keycode
 * @param down    True if the key was pressed, false if it was released
 * @return int32_t The final keycode after handling, or a negative numbe if the key event was consumed
 */
int32_t doExtKeyCb(uint32_t keycode, bool down, modKey_t modifiers)
{
    int32_t finalKey = keycode;
    EMU_CB_LOOP(fnKeyCb)
    {
        int32_t newKey = EMU_CB(fnKeyCb, finalKey, down, modifiers);

        if (newKey < 0)
        {
            // If result is negative, cancel the event and stop processing
            return newKey;
        }
        else if (newKey > 0)
        {
            // If the result is non-zero, replace the key with that value
            finalKey = newKey;
        }
    }

    return finalKey;
}

/**
 * @brief Calls the mouse movement callback for all enabled extensions
 *
 * @param x The new mouse position X-coordinate
 * @param y The new mouse position Y-coordinate
 * @param buttonMask A mask of all the currently pressed mouse buttons
 */
void doExtMouseMoveCb(int32_t x, int32_t y, mouseButton_t buttonMask)
{
    EMU_CB_LOOP(fnMouseMoveCb)
    {
        if (EMU_CB(fnMouseMoveCb, x, y, buttonMask))
        {
            return;
        }
    }
}

/**
 * @brief Calls the mouse button callback for all enabled extensions
 *
 * @param x The mouse position X-coordinate
 * @param y The mouse position Y-coordinate
 * @param button The button that was pressed or released
 * @param down True if the event was pressed, false if it was released
 */
void doExtMouseButtonCb(int32_t x, int32_t y, mouseButton_t button, bool down)
{
    EMU_CB_LOOP(fnMouseButtonCb)
    {
        if (EMU_CB(fnMouseButtonCb, x, y, button, down))
        {
            return;
        }
    }
}

/**
 * @brief Calls the render callback for all enabled extensions
 *
 * @param winW The window width in pixels
 * @param winH The window height in pixels
 */
void doExtRenderCb(uint32_t winW, uint32_t winH)
{
    node_t* node = extManager.extensions.first;
    while (node != NULL)
    {
        emuExtInfo_t* info = (emuExtInfo_t*)(node->val);

        if (info && info->enabled && info->extension && info->extension->fnRenderCb)
        {
            emuPane_t panes[info->panes.length];
            uint8_t i        = 0;
            node_t* paneNode = info->panes.first;
            while (NULL != paneNode)
            {
                emuPaneInfo_t* paneInfo = (emuPaneInfo_t*)paneNode->val;

                if (paneInfo->pane.visible)
                {
                    memcpy(&panes[i++], &paneInfo->pane, sizeof(emuPane_t));
                }

                paneNode = paneNode->next;
            }

            info->extension->fnRenderCb(winW, winH, (i > 0) ? panes : NULL, i);
        }

        node = node->next;
    }
}
