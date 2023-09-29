/*! \file dialogBox.h
 *
 * \section dialogBox_design Design Philosophy
 *
 * Dialog boxes are a simple, generic way to show a message and/or prompt the user for a response.
 *
 * \section dialogBox_example Example
 *
 * Const strings and declarations for the example dialog box:
 * \code{.c}
 * // Declare the title and body text
 * static const char dialogTitle = "Confirmation";
 * static const char dialogDetail = "Something happened! Continue?";
 *
 * // Declare the option labels
 * static const char optCancel[] = "Cancel";
 * static const char optOk[] = "OK";
 *
 * // Declare the callback
 * static void dialogBoxCb(const char* label);
 * \endcode
 *
 * Initialize the dialog box:
 * \code{.c}
 * bool dialogComplete = false;
 * bool confirmed = false;
 * font_t dialogFont;
 * wsg_t infoIcon;
 * loadFont("ibm_vga8.font", &dialogFont, false);
 * loadWsg("info.wsg", &infoIcon, false);
 * dialogBox_t* dialog = initDialogBox(dialogTitle, dialogDetail, &infoIcon, dialogBoxCb);
 * \endcode
 *
 * Inside the main loop:
 * \code{.c}
 * clearPxTft();
 * if (!dialogComplete)
 * {
 *     buttonEvt_t evt = {0};
 *     while (checkButtonQueueWrapper(&evt))
 *     {
 *         dialogBoxButton(button, &evt);
 *     }
 *
 *     // Draw the dialog, sized automatically, in the center of the screen
 *     drawDialogbox(dialog, &dialogFont, &dialogFont, DIALOG_CENTER, DIALOG_CENTER, DIALOG_AUTO, DIALOG_AUTO, 6);
 * }
 * else
 * {
 *     const char* message = confirmed ? "Confirmed" : "Cancelled";
 *     drawText(&dialogFont, c555, message, (TFT_WIDTH - textWidth(&dialogFont, message)) / 2, (TFT_HEIGHT - dialogFont.height) / 2);
 * }
 * \endcode
 *
 * \code{.c}
 * static void dialogBoxCb(const char* label)
 * {
 *     if (optCancel == label)
 *     {
 *         printf("Cancel\n");
 *         dialogComplete = true;
 *     }
 *     else if (optOk == label)
 *     {
 *         printf("OK!\n");
 *         dialogComplete = true;
 *         confirmed = true;
 *     }
 * }
 * \endcode
 *
 */
#ifndef _DIALOG_BOX_H_
#define _DIALOG_BOX_H_

//==============================================================================
// Includes
//==============================================================================
#include <stdint.h>
#include "linked_list.h"
#include "font.h"
#include "wsg.h"
#include "palette.h"
#include "hdw-btn.h"

//==============================================================================
// Defines
//==============================================================================
/// @brief If passed for X or Y, will center the dialog box on that axis
#define DIALOG_CENTER (1 << 15)

/// @brief If passed for W or H, will calculate the size of the dialog box based on its contents
#define DIALOG_AUTO (1 << 15)

typedef void (*dialogBoxCbFn_t)(const char* label);

//==============================================================================
// Enums
//==============================================================================
/**
 * @brief Hints for the dialog box renderer. Multiple options can be combined with bitwise OR.
 *
 */
typedef enum
{
    /// @brief No special options for this dialog option.
    OPTHINT_NORMAL = 0,

    /// @brief This option is the "OK" option
    OPTHINT_OK = 1,

    /// @brief This option is the "Cancel" option, and will be selected if the user presses B.
    OPTHINT_CANCEL = 2,

    /// @brief This option should be selected by default when the dialog resets.
    OPTHINT_DEFAULT = 4,

    /// @brief This option should be shown, but should not be selectable.
    OPTHINT_DISABLED = 8,
} dialogOptionHint_t;

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    /// @brief The text to draw in the dialog
    const char* label;

    /// @brief The icon, if any, to draw inside the option button
    const wsg_t* icon;

    /// @brief The rendering hints for the option button
    dialogOptionHint_t hints;
} dialogBoxOption_t;

typedef struct
{
    /// @brief The icon to draw in the left side of the dialog box, or NULL for no icon
    const wsg_t* icon;

    /// @brief The title of the dialog box to draw at the top
    const char* title;

    /// @brief The text in the body of the dialog box
    const char* detail;

    /// @brief The linked list of dialog box options
    list_t options;

    /// @brief The currently-selected option
    node_t* selectedOption;

    /// @brief The callback function for when an item is selected
    dialogBoxCbFn_t cbFn;

    /// @brief Whether A is being held
    bool holdA;

    /// @brief Whether B is being held
    bool holdB;
} dialogBox_t;

//==============================================================================
// Function Declarations
//==============================================================================
dialogBox_t* initDialogBox(const char* title, const char* detail, const wsg_t* icon, dialogBoxCbFn_t cbFn);
void deinitDialogBox(dialogBox_t* dialogBox);
void dialogBoxAddOption(dialogBox_t* dialogBox, const char* label, const wsg_t* icon, dialogOptionHint_t hints);
void dialogBoxReset(dialogBox_t* dialogBox);
void drawDialogBox(const dialogBox_t* dialogBox, const font_t* titleFont, const font_t* detailFont, uint16_t x,
                   uint16_t y, uint16_t w, uint16_t h, uint16_t r);
void dialogBoxButton(dialogBox_t* dialogBox, const buttonEvt_t* evt);

#endif
