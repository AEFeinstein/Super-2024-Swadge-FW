//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <stdio.h>
#include <esp_heap_caps.h>
#include <string.h>
#include "cutscene.h"
#include "esp_random.h"
#include "fs_wsg.h"

#include "shapes.h"
#include "midiPlayer.h"


//==============================================================================
// Function Prototypes
//==============================================================================
static void resetCutscene(cutscene_t* cutscene);
static int randomInt(int lowerBoundInclusive, int upperBoundInclusive);
static uint8_t getRandomVariationFromStyle(cutsceneStyle_t* style);
static uint8_t getRandomVariationFromStyleIdx(cutscene_t* cutscene, uint8_t styleIdx);
static cutsceneStyle_t* getCurrentStyle(cutscene_t* cutscene);

/**
 * @brief 
 * 
 * @param cbFunc 
 * @param nextIconIdx 
 * @return cutscene_t* 
 */
cutscene_t* initCutscene(cutsceneCb cbFunc, cnfsFileIdx_t nextIconIdx)
{
    cutscene_t* cutscene = (cutscene_t*)heap_caps_calloc(1, sizeof(cutscene_t), MALLOC_CAP_SPIRAM);
    cutscene->cbFunc = cbFunc;
    cutscene->lines = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
    cutscene->styles = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
    cutscene->sprite = heap_caps_calloc(1, sizeof(wsg_t), MALLOC_CAP_SPIRAM);
    cutscene->textBox = heap_caps_calloc(1, sizeof(wsg_t), MALLOC_CAP_SPIRAM);
    for(int i = 0; i < 4; i++)
    {
        cutscene->nextIcon[i] = heap_caps_calloc(1, sizeof(wsg_t), MALLOC_CAP_SPIRAM);
        loadWsg(nextIconIdx+i, cutscene->nextIcon[i], true);
    }
    cutscene->xOffset = 280; //default start value for antagonists.

    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    midiPlayerReset(player);
    midiPause(player, false);
    //midiGmOn(player);
    midiControlChange(player, 13, MCC_BANK_LSB, 2);
    //midiSetParameter(player, 13, false, 10, 2);
    midiSetProgram(player, 13, 30);
    // midiControlChange(player, 13, MCC_SUSTENUTO_PEDAL, 80);
    // midiControlChange(player, 13, MCC_SOUND_RELEASE_TIME, 60);

    return cutscene;
}

static void resetCutscene(cutscene_t* cutscene)
{
    cutscene->a_down = false;
    cutscene->blinkTimer = 0;
    cutscene->isEnding = false;
    while(cutscene->lines->first != NULL)
    {
        cutsceneLine_t* line = (cutsceneLine_t*) shift(cutscene->lines);
        free(line->body);
    }
    cutscene->listenForPB_A = false;
    cutscene->PB_A_frameCounter = 0;
    cutscene->xOffset = 280; //default start value for antagonists.
}

void removeAllStyles(cutscene_t* cutscene)
{
    while(cutscene->styles->first != NULL)
    {
        cutsceneStyle_t* style = (cutsceneStyle_t*) shift(cutscene->styles);
        free(style->title);
    }
}

void addCutsceneStyle(cutscene_t* cutscene, paletteColor_t textColor, cnfsFileIdx_t spriteIdx, cnfsFileIdx_t textBoxIdx, char* title, uint8_t numSpriteVariations, bool isProtagonist, bool drawSprite, bool drawTextbox)
{
    cutsceneStyle_t* style = (cutsceneStyle_t*)heap_caps_calloc(1, sizeof(cutsceneStyle_t), MALLOC_CAP_SPIRAM);
    style->title = (char*)heap_caps_calloc(strlen(title) + 1, sizeof(char), MALLOC_CAP_SPIRAM);
    strcpy(style->title, title);
    style->textColor = textColor;
    style->spriteIdx = spriteIdx;
    style->textBoxIdx = textBoxIdx;
    style->numSpriteVariations = numSpriteVariations;
    style->isProtagonist = isProtagonist;
    style->drawSprite = drawSprite;
    style->drawTextBox = drawTextbox;

    // push to tail
    push(cutscene->styles, (void*)style);
}

void addCutsceneLine(cutscene_t* cutscene, uint8_t styleIdx, char* body, bool flipHorizontal, int8_t spriteVariation)
{
    cutsceneLine_t* line = (cutsceneLine_t*)heap_caps_calloc(1, sizeof(cutsceneLine_t), MALLOC_CAP_SPIRAM);
    line->body = (char*)heap_caps_calloc(strlen(body) + 1, sizeof(char), MALLOC_CAP_SPIRAM);
    strcpy(line->body, body);
    line->styleIdx = styleIdx;
    line->spriteVariation = spriteVariation < 0 ? getRandomVariationFromStyleIdx(cutscene, styleIdx) : spriteVariation;
    line->flipHorizontal = flipHorizontal;

    if(cutscene->lines->first == NULL)
    {
        cutsceneStyle_t* style = ((cutsceneStyle_t*) getAtIndex(cutscene->styles, line->styleIdx));
        loadWsg(style->spriteIdx + line->spriteVariation,
        cutscene->sprite, true);
        loadWsg(style->textBoxIdx, cutscene->textBox, true);

        if(style->isProtagonist)
        {
            cutscene->xOffset *= -1;
        }
    }

    // push to tail
    push(cutscene->lines, (void*)line);
}

static int randomInt(int lowerBoundInclusive, int upperBoundInclusive)
{
    return esp_random() % (upperBoundInclusive - lowerBoundInclusive + 1) + lowerBoundInclusive;
}

static uint8_t getRandomVariationFromStyle(cutsceneStyle_t* style)
{
    return randomInt(0,style->numSpriteVariations - 1);
}

static uint8_t getRandomVariationFromStyleIdx(cutscene_t* cutscene, uint8_t styleIdx)
{
    return getRandomVariationFromStyle((cutsceneStyle_t*)getAtIndex(cutscene->styles, styleIdx));
}

static cutsceneStyle_t* getCurrentStyle(cutscene_t* cutscene)
{
    return getAtIndex(cutscene->styles, ((cutsceneLine_t*)cutscene->lines->first->val)->styleIdx);
}

void updateCutscene(cutscene_t* cutscene, int16_t btnState)
{
    if(cutscene->lines->first == NULL)
    {
        return;
    }
    cutsceneStyle_t* style = getCurrentStyle(cutscene);

    //Extra hold required because sometimes click-unclick-click registers on the hardware from an intended single click.
    if(!cutscene->isEnding && cutscene->xOffset == 0 && cutscene->listenForPB_A)
    {
        if(btnState & PB_A)
        {
            if(cutscene->PB_A_previousFrame == false)
            {
                // Copying and customizing the timbre should really only be done once
                midiTimbre_t cutsceneTimbre;
                memcpy(&cutsceneTimbre, getTimbreForProgram(false, 2, 30), sizeof(midiTimbre_t));

                cutsceneTimbre.envelope.attackTime = 0;
                // Using decay instead of release, with a sustain volume of 0, makes the note behave more like percussion
                // (no note off is necessary)
                // The control is set in increments of 10ms, so this is equivalent to setting release CC to 60
                cutsceneTimbre.envelope.decayTime = MS_TO_SAMPLES(600);
                // Setting sustain volume to 0 means the note ends on its own after decay
                cutsceneTimbre.envelope.sustainVol = 0;
                cutsceneTimbre.envelope.releaseTime = 0;
                // End timbre initialization stuff

                midiPlayer_t* bgm = globalMidiPlayerGet(MIDI_BGM);
                // Play a random note within an octave at half velocity on channel 1
                int songPitches[] = {58, 61, 63, 64, 65, 68, 70};//1
                //int songPitches[] = {60, 62, 67, 69, 72, 74};//2 bouncehause
                uint8_t pitch            = randomInt(0, 6);

                // Setting the channel to something > 15 means the note will not be affected by a song changing MIDI controls.
                soundNoteOn(bgm, 16, songPitches[pitch], 0x7F, &cutsceneTimbre, false);
            }
            cutscene->PB_A_previousFrame = true;
            cutscene->PB_A_frameCounter++;
            if(cutscene->PB_A_frameCounter > 30)
            {
                //proceed to next cutscene line.
                if(cutscene->lines->first != NULL && cutscene->lines->first->next != NULL)//There is at least onle line after this one.
                {
                    free(((cutsceneLine_t*)shift(cutscene->lines))->body);
                    style = getCurrentStyle(cutscene);
                    cutscene->listenForPB_A = false;
                    cutscene->PB_A_frameCounter = 0;

                    //load the new character sprite into the existing wsg.
                    loadWsg(style->spriteIdx + ((cutsceneLine_t*)cutscene->lines->first->val)->spriteVariation,
                    cutscene->sprite, true);
                    loadWsg(style->textBoxIdx, cutscene->textBox, true);
                }
                else//This was the last line.
                {
                    cutscene->isEnding = true;
                }
            }
        }
        else
        {
            cutscene->PB_A_previousFrame = false;
            cutscene->PB_A_frameCounter--;
            if(cutscene->PB_A_frameCounter < 0)
            {
                cutscene->PB_A_frameCounter = 0;
            }
        }
    }
    else if(!cutscene->isEnding && !(btnState & PB_A))
    {
        cutscene->listenForPB_A = true;
    }

    if(cutscene->isEnding)
    {
        if(cutscene->xOffset < -280 || cutscene->xOffset > 280)
        {
            //The cutscene is over
            cutscene->cbFunc();
            resetCutscene(cutscene);
            return;
        }
        if(style->isProtagonist)
        {
            cutscene->xOffset-=4;
        }   
        else
        {
            cutscene->xOffset+=4;
        }
    }
    else if(cutscene->xOffset != 0)
    {
        if(style->isProtagonist)
        {
            cutscene->xOffset+=8;
        }
        else
        {
            cutscene->xOffset-=8;
        }
    }

    cutscene->blinkTimer+=8;
}

void drawCutscene(cutscene_t* cutscene, font_t* font)
{
    if(cutscene->lines->first == NULL)
    {
        return;
    }

    cutsceneLine_t* line = (cutsceneLine_t*)cutscene->lines->first->val;
    cutsceneStyle_t* style = getAtIndex(cutscene->styles, line->styleIdx);

    if(style->drawSprite)
    {
        drawWsg(cutscene->sprite, cutscene->xOffset, 0, line->flipHorizontal, false, 0);
    }
    char variationText[5];
    snprintf(variationText, sizeof(variationText), "%d", line->spriteVariation);
    drawText(font, c541, variationText, 14, 14);
    if(cutscene->xOffset == 0)
    {
        if(style->drawTextBox)
        {
            drawWsgSimple(cutscene->textBox, 0, 137);
        }
        int8_t iconFrame = (cutscene->PB_A_frameCounter / 8);
        if(iconFrame > 3)
        {
            iconFrame = 3;
        }
        if(cutscene->PB_A_frameCounter > 0)
        {
            drawWsgSimpleScaled(cutscene->nextIcon[iconFrame], 248, 180, 2, 2);
        }
        else
        {
            drawWsgSimple(cutscene->nextIcon[iconFrame], 256, 190);
        }
        if (cutscene->blinkTimer > 0 && cutscene->PB_A_frameCounter == 0)
        {
            //drawWsgSimple(&dData->spriteNext, 254 + (textColor == c525 ? 4 : 0), -dData->offsetY + 186);
            drawText(font, c253, "A", 260, 190);
        }
    }

    if(cutscene->xOffset == 0)
    {
        int16_t xOff = 20;
        int16_t yOff = 150;
        //drawRect(xOff, yOff, 92, 162, c520);
        drawText(font, style->textColor, style->title, 20, 150);

        xOff = 13;
        yOff = 174;
        //drawRect(xOff, yOff, 252, 225, c520);
        drawTextWordWrap(font, style->textColor, line->body, &xOff, &yOff, 252, 225);
    }
}

void deinitCutscene(cutscene_t* cutscene)
{
    resetCutscene(cutscene);
    removeAllStyles(cutscene);
    free(cutscene->lines);
    free(cutscene->styles);
    for(int i = 0; i < 3; i++)
    {
        freeWsg(cutscene->nextIcon[i]);
    }
    freeWsg(cutscene->sprite);
    freeWsg(cutscene->textBox);
    free(cutscene);
}