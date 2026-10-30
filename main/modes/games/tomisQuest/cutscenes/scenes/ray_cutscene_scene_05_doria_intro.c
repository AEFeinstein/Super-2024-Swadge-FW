#include <string.h>
#include "ray_cutscene_scene_05_doria_intro.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_DORIA_INTRO;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneDoriaIntro(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "DORIA_INTRO_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_DORIA_INTRO)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_DORIA (CS_STYLE_HYUT + 1)
        addCutsceneStyleDoria(cutscene);

#define CS_STYLE_UNKN (CS_STYLE_DORIA + 1)
        addCutsceneStyle(cutscene, c000, TQ_DORIA_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "???", 1, true, false, true);

#define CS_STYLE_NARRATOR (CS_STYLE_UNKN + 1)
        addCutsceneStyleNarrator(cutscene);

        // clang-format off

        addCutsceneLine(cutscene, CS_STYLE_UNKN, "What do you think you're doing?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Who's there?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "A perfectly-placed arrow strikes the ground in front of Hyut's feet.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "!!", false, TOMI_SHOCKED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_UNKN, "State your business in the dungeon, child.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaa!! Hyup!! Hut!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "(It's ok Hyut, I got this.)", false, TOMI_COCKY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Hi - whoever you are. We're here to... uhh", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...explore?", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_UNKN, "Does this fairy speak for you, child?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah, I do.", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_UNKN, "... Very well.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "I am Doria. Protector of this space and the trials within. I ask you again - why do you seek to explore the Dungeon?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I 'unno, it seems like the sort of thing you'd want to do as part of the Great Fairy Trials...", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Sigh... another group of \"adventurers.\" Have you considered what completing the trials may bring you?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "A legacy of greatness, obviously.", false, TOMI_FIREDUP, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "I have learned that it is not my place to stop those who seek to attempt the Trials. However, I must observe - you are woefully unprepared for the dangers ahead.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Thanks a lot, lady.", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyup! Haa!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Do you and your companion even understand each other? It seems they merely... follow you around.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "We've got a good thing going! They swipe their sword when I tell 'em to! And we got bombs, so that's pretty cool?", false, TOMI_COCKY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "You lack even basic defensive training. You can't even JUMP.", false, 0, NULL);
        // TODO final instruction
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Tap 'A' quickly to command Hyut to jump!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "See? A jump. We're quick studies.", false, TOMI_COCKY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Sigh. I suppose we shall see. If you survive, I will see you again.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Doria vanishes in a puff of smoke.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Welp, here goes nothing...", false, TOMI_EMBARASSED, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
