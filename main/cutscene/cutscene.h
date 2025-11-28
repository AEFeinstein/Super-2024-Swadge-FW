#ifndef _CUTSCENE_H_
#define _CUTSCENE_H_

#include <stdint.h>
#include <stdbool.h>
#include "linked_list.h"
#include "hdw-btn.h"
#include "font.h"

#define TITLE_SIZE 10
#define BODY_SIZE 80

/**
 * @brief A callback which is called when this cutscene concludes. Use it to unpause your game loop.
 */
typedef void (*cutsceneCb)();


/**
 * @brief The underlying data for a cutscene line.
 */
typedef struct
{
    char title[TITLE_SIZE];          ///< The speaker's name displayed on screen.
    char body[BODY_SIZE];           ///< The spoken text displayed on screen.
    paletteColor_t color;    ///< The color of the text.
} cutsceneLine_t;

/**
 * @brief The underlying data for a cutscene. This fundamentally is a list of cutsceneLine_t
 */
typedef struct cutscene_t
{
    cutsceneCb cbFunc;        ///< The callback function to call when the cutscene concludes
    list_t* lines;            ///< A list_t of cutsceneLine_t
    uint8_t blinkTimer;       ///< Increments and overflows to make the next graphic blink.
    bool a_down;              ///< True when the a button is held.
} cutscene_t;

cutscene_t* initCutscene(const char* title, cutsceneCb cbFunc) __attribute__((warn_unused_result));
void addCutsceneLine(char title[TITLE_SIZE], char body[BODY_SIZE], paletteColor_t color);