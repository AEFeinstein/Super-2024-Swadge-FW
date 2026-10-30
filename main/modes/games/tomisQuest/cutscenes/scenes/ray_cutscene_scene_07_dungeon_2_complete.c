#include <string.h>
#include "ray_cutscene_scene_07_dungeon_2_complete.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_DUNGEON_2_COMPLETE;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneDungeon2Complete(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "DUNGEON_2_COMPLETE_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_DUNGEON_2_COMPLETE)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_DORIA (CS_STYLE_HYUT + 1)
        addCutsceneStyleDoria(cutscene);

#define CS_STYLE_NARRATOR (CS_STYLE_DORIA + 1)
        addCutsceneStyleNarrator(cutscene);

        // clang-format off

        // [Tomi and Hyut clear the second phase of the dungeon and open the chest to receive the magical boomerang. Then Doria appears.]
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "I see you've made some progress.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaa! Hyup!! Ha!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Have you achieved \"greatness\" yet?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "What's that supposed to mean? That's what we're doing here. Questing. Great Fairy Trials. It's in the name.", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Mmmhmm. And what do you think it means to be \"great\"?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I dunno, doing great things? Stuff that's hard?", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Hmm.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaa! Hiiiyyeea!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah, I agree, what do you care? And why are you constantly stalking us? It's kind of creepy.", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "I don't have to answer any of your questions.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah, see? Super creepy.", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Let's just say... I am trying to keep you from getting hurt.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hup! Haaa! Hyaaa!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah, I don't know what your angle is, lady. We were just fighting for our lives back there, I didn't see you helping at all.", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "You didn't appear to need my help when it came to combat.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Easy for you to say NOW. Any tips on where to use this magical boomerang then?", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "... I don't have to answer your questions.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Doria vanishes in a huff.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah, figures.", false, TOMI_BORED, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
