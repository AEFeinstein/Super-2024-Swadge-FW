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

//==============================================================================
// Function Prototypes
//==============================================================================
static void resetCutscene(cutscene_t* cutscene);
static void loadAndPlayCharacterSound(cutsceneStyle_t* style, cutscene_t* cutscene, uint8_t pitchIdx);
static void loadAndPlayRandomCharacterSound(cutsceneStyle_t* style, cutscene_t* cutscene);
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
cutscene_t* initCutscene(cutsceneCb cbFunc, cnfsFileIdx_t nextIconIdx, uint8_t soundBank)
{
    cutscene_t* cutscene = (cutscene_t*)heap_caps_calloc(1, sizeof(cutscene_t), MALLOC_CAP_SPIRAM);
    cutscene->cbFunc = cbFunc;
    cutscene->lines = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
    cutscene->styles = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
    cutscene->sprite = heap_caps_calloc(1, sizeof(wsg_t), MALLOC_CAP_SPIRAM);
    cutscene->textBox = heap_caps_calloc(1, sizeof(wsg_t), MALLOC_CAP_SPIRAM);
    cutscene->soundBank = soundBank;
    for(int i = 0; i < 4; i++)
    {
        cutscene->nextIcon[i] = heap_caps_calloc(1, sizeof(wsg_t), MALLOC_CAP_SPIRAM);
        loadWsg(nextIconIdx+i, cutscene->nextIcon[i], true);
    }
    cutscene->xOffset = 280; //default start value for antagonists.
    cutscene->nextIconAnimationTimer = 30;
    cutscene->bgm_headroom = 0x4000;
    //default is C major scale pitches
    cutscene->songPitches[0] = 60;
    cutscene->songPitches[1] = 62;
    cutscene->songPitches[2] = 64;
    cutscene->songPitches[3] = 65;
    cutscene->songPitches[4] = 67; 
    cutscene->songPitches[5] = 69;
    cutscene->songPitches[6] = 71;
    cutscene->songPitches[7] = 72;

    midiPlayer_t* player = globalMidiPlayerGet(MIDI_SFX);
    midiPlayerReset(player);
    midiPause(player, false);
    player->headroom = 0x8000;//max volume
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
    cutscene->nextIconAnimationTimer = 30;
    cutscene->xOffset = 280; //default start value for antagonists.
    cutscene->bgm_headroom = 0x4000;
    
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_SFX);
    player->headroom = 0x4000;
    player = globalMidiPlayerGet(MIDI_BGM);
    player->headroom = 0x4000;
}

static void loadAndPlayCharacterSound(cutsceneStyle_t* style, cutscene_t* cutscene, uint8_t pitchIdx)
{
    cutscene->nextIconAnimationTimer = 4;
    // Copying and customizing the timbre should really only be done once
    memcpy(&cutscene->timbre, getTimbreForProgram(false, 2, style->instrument), sizeof(midiTimbre_t));

    cutscene->timbre.envelope.attackTime = 0;
    
    if(style->slowAttack)
    {
        cutscene->timbre.envelope.attackTime = MS_TO_SAMPLES(250);//600
        cutscene->timbre.envelope.attackTimeVel = (MS_TO_SAMPLES(500) << 8) / 127;
    }
    else
    {
        cutscene->timbre.envelope.attackTime = 0;
        cutscene->timbre.envelope.attackTimeVel = 0;
    }

    // Using decay instead of release, with a sustain volume of 0, makes the note behave more like percussion
    // (no note off is necessary)
    // The control is set in increments of 10ms, so this is equivalent to setting release CC to 60
    cutscene->timbre.envelope.decayTime = MS_TO_SAMPLES(250);//600
    cutscene->timbre.envelope.decayTimeVel = (MS_TO_SAMPLES(style->noteLength) << 8) / 127;
    // Setting sustain volume to 0 means the note ends on its own after decay
    cutscene->timbre.envelope.sustainVol = 0;
    cutscene->timbre.envelope.releaseTime = 0;
    // End timbre initialization stuff

    midiPlayer_t* player = globalMidiPlayerGet(MIDI_SFX);
    midiPause(player, false);
    
    //midiControlChange(player, 13, MCC_BANK_LSB, 2);

    player->headroom = 0x8000;//max volume

    // Setting the channel to something > 15 means the note will not be affected by a song changing MIDI controls.
    soundNoteOn(player, 13, cutscene->songPitches[pitchIdx] + 12 * style->octaveOvset, 127, &cutscene->timbre, false);
}

static void loadAndPlayRandomCharacterSound(cutsceneStyle_t* style, cutscene_t* cutscene)
{
    uint8_t pitchIdx = randomInt(0, 7);
    while(cutscene->songPitches[pitchIdx] == -1)
    {
        pitchIdx = randomInt(0, 7);
    }
    loadAndPlayCharacterSound(style, cutscene, pitchIdx);
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
    //Set some default midi value based on the provided bank
    switch(cutscene->soundBank)
    {
        case 0://general midi
        {
            style->instrument = 1;//grand piano
            break;
        }
        case 1://magfest
        {
            style->instrument = 11;//"MAGFest Wave"
            break;
        }
        case 2://Mega Man X
        {
            style->instrument = 82;//Synth Lead
            break;
        }
        default:
        {
            break;
        }
    }
    style->octaveOvset = 0;
    style->noteLength = 250;
    style->slowAttack = false;

    // push to tail
    push(cutscene->styles, (void*)style);
}

void setMidiParams(cutscene_t* cutscene, uint8_t styleIdx, uint8_t instrument, int8_t octaveOvset, uint16_t noteLength, bool slowAttack)
{
    cutsceneStyle_t* style = getAtIndex(cutscene->styles, styleIdx);
    style->instrument = instrument;
    style->octaveOvset = octaveOvset;
    style->noteLength = noteLength;
    style->slowAttack = slowAttack;
}

void setSongPitches(cutscene_t* cutscene, int16_t songPitches[8])
{
    for(int i = 0; i < 8; i++)
    {
        cutscene->songPitches[i] = songPitches[i];
    }
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
    cutscene->blinkTimer+=8;
    //Extra hold required because sometimes click-unclick-click registers on the hardware from an intended single click.
    if(!cutscene->isEnding && cutscene->xOffset == 0)
    {
        if(btnState & PB_A && !(cutscene->btnState_previousFrame & PB_A))
        {
            //proceed to next cutscene line.
            if(cutscene->lines->first != NULL && cutscene->lines->first->next != NULL)//There is at least onle line after this one.
            {
                free(((cutsceneLine_t*)shift(cutscene->lines))->body);
                style = getCurrentStyle(cutscene);

                //load the new character sprite into the existing wsg.
                loadWsg(style->spriteIdx + ((cutsceneLine_t*)cutscene->lines->first->val)->spriteVariation,
                cutscene->sprite, true);
                loadWsg(style->textBoxIdx, cutscene->textBox, true);

                loadAndPlayRandomCharacterSound(style, cutscene);
            }
            else//This was the last line.
            {
                cutscene->isEnding = true;
            }
        }
        else if(btnState & PB_DOWN && !(cutscene->btnState_previousFrame & PB_DOWN))
        {
            loadAndPlayCharacterSound(style, cutscene, 0);
        }
        else if(btnState & PB_LEFT && !(cutscene->btnState_previousFrame & PB_LEFT))
        {
            loadAndPlayCharacterSound(style, cutscene, 1);
        }
        else if(btnState & PB_RIGHT && !(cutscene->btnState_previousFrame & PB_RIGHT))
        {
            loadAndPlayCharacterSound(style, cutscene, 2);
        }
        else if(btnState & PB_UP && !(cutscene->btnState_previousFrame & PB_UP))
        {
            loadAndPlayCharacterSound(style, cutscene, 3);
        }
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
        if(cutscene->xOffset == 0)
        {
            loadAndPlayRandomCharacterSound(style, cutscene);
        }
    }
    //Audio ducking BGM during character sound
    if(cutscene->nextIconAnimationTimer <= 30)
    {
        cutscene->nextIconAnimationTimer++;

        cutscene->bgm_headroom = 0x1000 + (cutscene->nextIconAnimationTimer - 4) * 153;
        midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
        player->headroom = cutscene->bgm_headroom;
    }
    //BGM volume fades to 50% during cutscene, and increases to normal at the end.
    else
    {
        if(cutscene->isEnding)
        {
            if(cutscene->bgm_headroom < 0x4000)
            {
                cutscene->bgm_headroom += 0x111;
            }
            else
            {
                cutscene->bgm_headroom = 0x4000;
            }
        }
        else if(cutscene->xOffset != 0)
        {
            if(cutscene->bgm_headroom > 0x2000)
            {
                cutscene->bgm_headroom -= 0x111;
            }
            else
            {
                cutscene->bgm_headroom = 0x2000;
            }
        }
        midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
        player->headroom = cutscene->bgm_headroom;
    }
    cutscene->btnState_previousFrame = btnState;
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
        if(cutscene->nextIconAnimationTimer > 30)
        {
            drawWsgSimple(cutscene->nextIcon[0], 256, 190);
            if (cutscene->blinkTimer > 0)
            {
                drawText(font, c253, "A", 260, 190);
            }
        }
        else
        {
            int8_t iconFrame = (cutscene->nextIconAnimationTimer / 4);
            if(iconFrame > 3)
                    {
                        iconFrame = 3;
                    }
            drawWsgSimpleScaled(cutscene->nextIcon[iconFrame], 215, 130, 4, 4);
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
        //drawRect(xOff, yOff, 252, 240, c520);
        drawTextWordWrap(font, style->textColor, line->body, &xOff, &yOff, 252, 240);
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