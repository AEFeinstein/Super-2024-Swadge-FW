#ifndef _DIALOG_BOX_H_
#define _DIALOG_BOX_H_

#include <stdint.h>
#include "linked_list.h"
#include "font.h"
#include "wsg.h"
#include "palette.h"
#include "hdw-btn.h"

/// @brief If passed for X or Y, will center the dialog box on that axis
#define DIALOG_CENTER (1 << 15)

/// @brief If passed for W or H, will calculate the size of the dialog box based on its contents
#define DIALOG_AUTO (1 << 15)

typedef void (*dialogBoxCbFn_t)(const char* label);

/**
 * @brief Hints for the dialog box renderer
 *
 */
typedef enum
{
    OPTHINT_NORMAL = 0,
    OPTHINT_OK = 1,
    OPTHINT_CANCEL = 2,
    OPTHINT_DEFAULT = 4,
    OPTHINT_DISABLED = 8,
} dialogOptionHint_t;

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

dialogBox_t* initDialogBox(const char* title, const char* detail, const wsg_t* icon, dialogBoxCbFn_t cbFn);
void deinitDialogBox(dialogBox_t* dialogBox);
void dialogBoxAddOption(dialogBox_t* dialogBox, const char* label, const wsg_t* icon, dialogOptionHint_t hints);
void dialogBoxReset(dialogBox_t* dialogBox);
void drawDialogBox(const dialogBox_t* dialogBox, const font_t* titleFont, const font_t* detailFont, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r);
void dialogBoxButton(dialogBox_t* dialogBox, const buttonEvt_t* evt);

#endif
