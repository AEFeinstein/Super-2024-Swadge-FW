/*! \file cutscene.h
 *
 * \section cutscene_design Design Philosophy
 *
 * A cutscene takes the data in the cutscene_t data structure and renders it to the display. Using cutscene should feel similar in some ways to using menuMegaRenderer,
 * and it handles all its own memory management. You shouldn't need to malloc/calloc any of the structs found in cutscene.h.
 * The cutscenes also make sounds as you press A to progress, or mash the arrow keys to jam out. Many default midi values are internally set up for you to quickly get testing.
 * Although a musically inclined ear would do well to tactfully assign certain instruments and effects to characters and set pitches that can work nicely with background music.
 * \section cutscene Usage
 *
 * Start with initCutscene(), then add any number of styles with addCutsceneStyle(). Generally think of a style as a character. It has its name, various poses, and a certain sound.
 * Styles can also be used in other clever ways such as an incoming transmission, or getting a new ability!
 * 
 * Consider calling setMidiParams() in a way that meshes with other background music. You can call it again when the BGM changes, and all the parameters will be overwritten.
 * But this function is completely optional. Midi sounds will be populated with some kind of noise by default.
 * 
 * Add any number of dialogue lines with addCutsceneLine(). A list will be internally shifted as each line is disposed with the A press.
 * At any point later in your game, simply add a bunch of other dialogue lines to do yet another cutscene.
 * 
 * Add updateCutscene() to your own update loop. And add drawCutscene() to your own draw loop.
 * 
 * When you are all done with cutscenes, typically on Mode Exit, you must call deinitCutscene().
 *
 * \section cutscene_example Example
 *
 * See megaPulseEx.c and mgCutscenes.c and to see how cutscenes were used in the 2026 Mega Pulse X platformer.
 */

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
 * @brief Style specifications to draw, and sound a cutsceneLine such as text color, character sprite, and midi notes.
 * 
 */
typedef struct
{
    char* title; ///< The speaker's name displayed on screen.
    paletteColor_t textColor; ///< The color of the text.
    cnfsFileIdx_t spriteIdx; ///< The file index of the character sprite.
    cnfsFileIdx_t textBoxIdx; ///< The file index of the textbox sprite.
    uint8_t numSpriteVariations; ///<A random number in range of this will be rolled and added to the spriteIdx. Provide 1 for no variation;
    bool stageLeft; ///<If true, then the sprite enters and leaves the left side of the screen. If false, then the right side.
    bool drawSprite;    ///< Set false to draw no sprite.
    bool drawTextBox;   ///< Set false to draw no textbox.
    uint8_t instrument; ///< Program# of the instrument when A is pressed.
    int8_t octaveOvset; ///< How many octaves to transpose this character's sound up or down. Zero for no effect.
    uint16_t noteLength; ///< Increase to have the note decay slower. 250 sounds fairly natural.
    bool slowAttack;    ///< True to make the note have a slow attack.
} cutsceneStyle_t;

/**
 * @brief The underlying data for a cutscene. A cutscene is fundamentally made of a list of cutsceneLine_t. With other variables that can be stored in one place here.
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
    int8_t nextIconAnimationTimer; ///< Increments and controls the next icon frame.
    bool isEnding;      ///< True as the character slides out of frame.
    uint8_t soundBank;  ///< 2 for mega man sounds. https://adam.feinste.in/Super-2024-Swadge-FW/MIDI.html#general-midi-bank-0
    midiTimbre_t timbre; ///< Lots of midi params for character sound playback.
    int32_t bgm_headroom; ///< The volume of background music. Fades 33% lower during cutscenes.
    int16_t btnState_previousFrame; ///< The btnState of the previous frame.
    int16_t songPitches[8]; ///< Up to eight pitches and no less than four pitches that harmonize with a song. Any pitches in idx 4 thru 7 may be -1 to be unused.
} cutscene_t;

cutscene_t* initCutscene(cutsceneCb cbFunc, cnfsFileIdx_t nextIconIdx, uint8_t soundBank);
void removeAllStyles(cutscene_t* cutscene);
void addCutsceneStyle(cutscene_t* cutscene, paletteColor_t color, cnfsFileIdx_t spriteIdx, cnfsFileIdx_t textBoxIdx, char* title, uint8_t numPoseVariations, bool stageLeft, bool drawSprite, bool drawTextbox);
void setMidiParams(cutscene_t* cutscene, uint8_t styleIdx, uint8_t instrument, int8_t octaveOvset, uint16_t noteLength, bool slowAttack);
void setSongPitches(cutscene_t* cutscene, int16_t songPitches[8]);
void addCutsceneLine(cutscene_t* cutscene, uint8_t styleIdx, char* body, bool flipHorizontal, int8_t spriteVariation);
void updateCutscene(cutscene_t* cutscene, int16_t btnState);
void drawCutscene(cutscene_t* cutscene, font_t* font);
void deinitCutscene(cutscene_t* cutscene);

#endif
