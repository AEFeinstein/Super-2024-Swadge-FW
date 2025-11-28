#ifndef _CUTSCENE_H_
#define _CUTSCENE_H_

#include <stdint.h>
#include <stdbool.h>
#include "linked_list.h"
#include "hdw-btn.h"
#include "font.h"
#include "cnfs_image.h"
#include "wsg.h"


/**
 * @brief A callback which is called when this cutscene concludes. Use it to unpause your game loop.
 */
typedef void (*cutsceneCb)();

/**
 * @brief The underlying data for a cutscene line.
 */
typedef struct
{
    char* title; ///< The speaker's name displayed on screen.
    char* body;   ///< The spoken text displayed on screen.
    uint8_t styleIdx;   ///< Index into the list of lineStyle_t.
    uint8_t spriteVariation; ///< A number that is added to the spriteIdx to display various character poses.
} cutsceneLine_t;

/**
 * @brief Style specifications for a cutsceneLine such as text color, character sprite, and midi notes
 * 
 */
typedef struct
{
    paletteColor_t textColor;
    cnfsFileIdx_t spriteIdx;
    uint8_t numSpriteVariations; ///<A random number in range of this will be rolled and added to the spriteIdx. Provide 1 for no variation;
} cutsceneStyle_t;

/**
 * @brief The underlying data for a cutscene. This fundamentally is a list of cutsceneLine_t
 */
typedef struct
{
    cutsceneCb cbFunc;  ///< A callback which is called when this cutscene concludes. Use it to unpause your game loop.
    list_t* styles;     ///< A list_t of lineStyle_t
    list_t* lines;      ///< A list_t of cutsceneLine_t
    uint8_t blinkTimer; ///< Increments and overflows to make the next graphic blink.
    bool a_down;        ///< True when the a button is held.
    uint8_t yOffset;    ///< Decrements to slide the character portait in from below.
    wsg_t* sprite;      ///< The sprite rendered behind the text.
} cutscene_t;

cutscene_t* initCutscene(cutsceneCb cbFunc) __attribute__((warn_unused_result));
void addCutsceneStyle(cutscene_t* cutscene, paletteColor_t color, cnfsFileIdx_t spriteIdx, uint8_t numPoseVariations);
void addCutsceneLine(cutscene_t* cutscene, char* title, char* body, uint8_t styleIdx);
void updateCutscene(cutscene_t* cutscene);
void drawCutscene(cutscene_t* cutscene, font_t* font);

#endif
