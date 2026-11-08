#include <string.h>
#include "ray_cutscene_scene_11_final_fight.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_FINAL_FIGHT;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneFinalFight(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "FINAL_FIGHT_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_FINAL_FIGHT)
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

        // [Tomi and Hyut enter a dark room.]
        addCutsceneLine(cutscene, CS_STYLE_UNKN, "You have done well to make it this far, young fairy. But now... you must face your greatest challenge yet.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Ummmm... This isn't going to be a boss thing, is it?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaaa!!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "yeah, what they said!", false, TOMI_FIREDUP, NULL);
        addCutsceneLine(cutscene, CS_STYLE_UNKN, "Prepare yourself, young fairy. Your final trial begins now.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Suddenly, out of the shadows emerges a demonic, phantom. The figure has a penguin's head, is clad in a suit, and carries a briefcase.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HANK, "WHERE AM I? FOR WHAT HAVE I BEEN SUMMONED?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Uhhh...", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HANK, "YOU!! FAIRY!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Me?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HANK, "DERIVATIVE!! UNORIGINAL!!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaa!!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HANK, "CEASE AND DESIST THIS HACKNEYED NONSENSE AT ONCE!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Hey, who are you calling derivative? Hyut, let's bust this loser up!", false, TOMI_ANGRY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "HEEEYYAAAAAAA!!!!", false, HYUT_YELLING, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
