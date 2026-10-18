#include <string.h>
#include "ray_cutscene_scene_02_hyut_intro.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_HYUT_INTRO;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray the entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneHyutIntro(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "HYUT_INTRO_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_HYUT_INTRO)
        {
            return true;
        }
        
        // Initialize Styles

#define CS_STYLE_HYUT_QQQ_NO_PORTRAIT (CS_STYLE_HYUT + 1)
        addCutsceneStyle(cutscene, c000, TQ_HYUT_PORTRAIT_00_NEUTRAL_WSG, CUTSCENE_TEXTBOX_WSG, "????", 2, false, false,
                         true);

#define CS_STYLE_HYUT_QQQ (CS_STYLE_HYUT_QQQ_NO_PORTRAIT + 1)
        addCutsceneStyle(cutscene, c000, TQ_HYUT_PORTRAIT_00_NEUTRAL_WSG, CUTSCENE_TEXTBOX_WSG, "????", 2, false, true,
                         true);

        // clang-format off

        addCutsceneLine(cutscene, CS_STYLE_HYUT_QQQ_NO_PORTRAIT, "Hyaaa!!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "What is that strange screaming?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT_QQQ_NO_PORTRAIT, "HUP!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I better investigate.", false, TOMI_SUSPICIOUS, NULL);

        // at this point, Hyut will have a character portrait, but will not have a name yet.

        addCutsceneLine(cutscene, CS_STYLE_HYUT_QQQ, "HAAAAAHH!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Woah! Hey kid, why aren't you with the others?", false, TOMI_SHOCKED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT_QQQ, "HYAAAA!! HYUP! HAAA!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Wo-woah! Chill out! Jeez! Watch where you're swinging that thing!", false, TOMI_SHOCKED, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
