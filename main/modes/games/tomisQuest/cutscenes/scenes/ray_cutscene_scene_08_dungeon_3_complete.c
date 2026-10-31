#include <string.h>
#include "ray_cutscene_scene_08_dungeon_3_complete.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_DUNGEON_3_COMPLETE;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneDungeon3Complete(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "DUNGEON_3_COMPLETE_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_DUNGEON_3_COMPLETE)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_DORIA (CS_STYLE_HYUT + 1)
        addCutsceneStyleDoria(cutscene);

        // clang-format off

        // Initialize lines

        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Oh look. Doria again.", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Please Tomi, you must turn back here. This is as far as you need go.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaa!! Hut! Haa!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Doria, why are you trying to stop us?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "I am not your enemy, and I will not stand in your way. I merely offer information. I can tell you that there's nothing you need in the rooms ahead to complete your quest.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Oh? And how do you know that?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "...You've always had the power to access the Shrine of the Great Fairies. The path lies in my sacred melody.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Stand by the fountain in Gaylordia Town Square and sing my Lullaby. Then the path will become clear.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaa?? Huh! Hiyaa!!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah Doria, thanks, but... why didn't you tell us this sooner? This all seems pretty shady. And why shouldn't we see what's at the bottom of the Dungeon?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Again, I am simply trying to - perhaps - spare you some pain. Like I said: play my melody in the town square and complete the Trial. Or you can continue on here and see what lies beneath. The choice is yours.", false, 0, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
