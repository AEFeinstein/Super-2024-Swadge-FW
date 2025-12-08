#ifndef _CUTSCENE_H_
#define _CUTSCENE_H_

#include <stdint.h>
#include <stdbool.h>
#include "linked_list.h"
#include "hdw-btn.h"
#include "font.h"
#include "cnfs_image.h"
#include "wsg.h"
#include "midiPlayer.h"


/**
 * @brief A callback which is called when this cutscene concludes. Use it to unpause your game loop.
 */
typedef void (*cutsceneCb)();

/**
 * @brief The underlying data for a cutscene line.
 */
typedef struct
{
    char* body;   ///< The spoken text displayed on screen.
    uint8_t styleIdx;   ///< Index into the list of lineStyle_t.
    int8_t spriteVariation; ///< A number that is added to the spriteIdx to display various character poses.
    bool flipHorizontal; ///< Draw the character sprite flipped.
} cutsceneLine_t;

/**
 * @brief Style specifications for a cutsceneLine such as text color, character sprite, and midi notes
 * 
 */
typedef struct
{
    char* title; ///< The speaker's name displayed on screen.
    paletteColor_t textColor;
    cnfsFileIdx_t spriteIdx;
    cnfsFileIdx_t textBoxIdx;
    uint8_t numSpriteVariations; ///<A random number in range of this will be rolled and added to the spriteIdx. Provide 1 for no variation;
    bool isProtagonist; ///<If true, then the sprite enters and leaves the left side of the screen. If false, then the right side.
    bool drawSprite;    ///< Set false to draw no sprite.
    bool drawTextBox;   ///< Set false to draw no textbox.
    uint8_t instrument; ///< Program# of the instrument when A is pressed.
    int8_t octaveOvset; ///< How many octaves to transpose this character's sound up or down. Zero for no effect.
} cutsceneStyle_t;

/**
 * @brief The underlying data for a cutscene. This fundamentally is a list of cutsceneLine_t
 */
typedef struct
{
    cutsceneCb cbFunc;  ///< A callback which is called when this cutscene concludes. Use it to unpause your game loop.
    list_t* styles;     ///< A list_t of lineStyle_t
    list_t* lines;      ///< A list_t of cutsceneLine_t
    int8_t blinkTimer;  ///< Increments and overflows to make the next graphic blink.
    bool a_down;        ///< True when the a button is held.
    int16_t xOffset;    ///< Decrements to slide the character portait in from below.
    wsg_t* sprite;      ///< The character sprite.
    wsg_t* textBox;     ///< The textbox rendered over the character sprite.
    wsg_t* nextIcon[4]; ///< The nextIcon with a few animation frames.
    bool listenForPB_A; ///< Used for detecting intentional A presses.
    int8_t PB_A_frameCounter; ///< Increments while A is held.
    bool PB_A_previousFrame; ///< True if A is held on the previous frame.
    bool isEnding;      ///< True as the character slides out of frame.
    uint8_t soundBank;  ///< 2 for mega man sounds. https://adam.feinste.in/Super-2024-Swadge-FW/MIDI.html#general-midi-bank-0
    midiTimbre_t timbre;
} cutscene_t;

cutscene_t* initCutscene(cutsceneCb cbFunc, cnfsFileIdx_t nextIconIdx, uint8_t soundBank);
void removeAllStyles(cutscene_t* cutscene);
void addCutsceneStyle(cutscene_t* cutscene, paletteColor_t color, cnfsFileIdx_t spriteIdx, cnfsFileIdx_t textBoxIdx, char* title, uint8_t numPoseVariations, bool stageLeft, bool drawSprite, bool drawTextbox, uint8_t instrument, int8_t octaveOvset);
void addCutsceneLine(cutscene_t* cutscene, uint8_t styleIdx, char* body, bool flipHorizontal, int8_t spriteVariation);
void updateCutscene(cutscene_t* cutscene, int16_t btnState);
void drawCutscene(cutscene_t* cutscene, font_t* font);
void deinitCutscene(cutscene_t* cutscene);

#endif
