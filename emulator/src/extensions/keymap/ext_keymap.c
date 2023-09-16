#include "ext_keymap.h"

#include <string.h>
#include <stdio.h>

#include "macros.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static bool keymapInit(emuArgs_t* emuArgs);
static int32_t keymapKeyCb(uint32_t keycode, bool down);

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    const char* name; ///< The name of the keyboard layout

    union
    {
        /// @brief Holds each keycode in a separate char
        struct
        {
            char up;
            char down;
            char left;
            char right;

            char a;
            char b;

            char start;
            char select;
        };

        /// @brief Holds all keycodes in a single string
        // These keycodes will be in the same memory
        char keymap[9];
    };
} emuKeymap_t;

//==============================================================================
// Variables
//==============================================================================

const emuExtension_t keymapEmuCallback = {
    .name            = "keymap",
    .fnInitCb        = keymapInit,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = keymapKeyCb,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = NULL,
};

static const emuKeymap_t keymaps[] = {
    {.name = "qwerty", .keymap = "WSADLKOI"},  // QWERTY (default)
    {.name = "azerty", .keymap = "ZSQDLKOI"},  // AZERTY
    {.name = "colemak", .keymap = "WRASIEYU"}, // Colemak
    {.name = "dvorak", .keymap = ",OAENTRC"},  // Dvorak
};

static const emuKeymap_t* activeKeymap = NULL;

//==============================================================================
// Functions
//==============================================================================

static bool keymapInit(emuArgs_t* emuArgs)
{
    if (emuArgs->keymap != NULL)
    {
        for (const emuKeymap_t* keymap = keymaps; keymap < (keymaps + ARRAY_SIZE(keymaps)); keymap++)
        {
            if (!strncmp(emuArgs->keymap, keymap->name, strlen(emuArgs->keymap)))
            {
                printf("Set keymap to '%s'\n", keymap->name);
                // Set the keyboard map, we found it!
                activeKeymap = keymap;

                // Setup successful
                return true;
            }
        }

        if (activeKeymap == NULL)
        {
            // We never set a keymap
            fprintf(stderr, "WARN: Unknown keyboard layout '%s'\n", emuArgs->keymap);
            return false;
        }
    }

    // No configuration found, no need to set up
    return false;
}

static int32_t keymapKeyCb(uint32_t keycode, bool down)
{
    // Convert lowercase characters to their uppercase equivalents
    if ('a' <= keycode && keycode <= 'z')
    {
        keycode = (keycode - 'a' + 'A');
    }

    // Check if the key matches one in the layout
    if (activeKeymap != NULL)
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            if (activeKeymap->keymap[i] == keycode)
            {
                // Remap onto qwerty
                return keymaps->keymap[i];
            }
        }
    }

    for (uint8_t i = 0; i < 8; i++)
    {
        if (keymaps->keymap[i] == keycode)
        {
            // Stop the original key from colliding
            return -1;
        }
    }

    return 0;
}
