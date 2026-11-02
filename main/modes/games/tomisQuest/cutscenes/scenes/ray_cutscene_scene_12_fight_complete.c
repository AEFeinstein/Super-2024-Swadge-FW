#include <string.h>
#include "ray_cutscene_scene_12_fight_complete.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_FIGHT_COMPLETE;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneFightComplete(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "FIGHT_COMPLETE_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_FIGHT_COMPLETE)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_UNKN (CS_STYLE_HYUT + 1)
        addCutsceneStyleUnknown(cutscene);

#define CS_STYLE_NARRATOR (CS_STYLE_UNKN + 1)
        addCutsceneStyleNarrator(cutscene);

#define CS_STYLE_HANK (CS_STYLE_NARRATOR + 1)
        addCutsceneStyleHankW(cutscene);

        // clang-format off

        // Initialize lines

        addCutsceneLine(cutscene, CS_STYLE_HANK, "HUURRRGGGHH!!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaaa!!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HANK, "WHY? WHY MUST I BE CONSCRIPTED TO PERPETUATE THIS CHILDISH CHARADE?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HANK, "HANK WADDLEDORF WILL HAVE HIS REVENGE! ONE DAY, I WILL EXTERMINATE ALL YOUR DESCENDANTS! YES! ... ONE DAY....", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyyyaaaaaa!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Hank crumbles, leaving a stack of legal papers in his wake.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Man, what a jerk. Are you okay, HYUT?", false, TOMI_ANGRY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyyyut!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_UNKN, "Well done, young fairy. The way forward is open.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I can't believe it. I did it! I mean... we did it, HYUT!", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "We're finally going to meet the Great Fairies!", false, TOMI_FIREDUP, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "HIYAAA!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Do I look ok? Ohmygosh, I ... I - I can't believe this is finally happening!!", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "HEEEYAAHH!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Stay close, Hyut... I can't believe it... It's really happening...", false, TOMI_EMBARASSED, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
