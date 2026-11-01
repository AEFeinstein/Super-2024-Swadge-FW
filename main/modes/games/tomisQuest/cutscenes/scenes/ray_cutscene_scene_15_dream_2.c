#include <string.h>
#include "ray_cutscene_scene_15_dream_2.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_DREAM_2;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneDream2(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "DREAM_2_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_DREAM_2)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_GFS (CS_STYLE_HYUT + 1)
        addCutsceneStyleGreatFairies(cutscene);

        // clang-format off

        // Initialize lines

        // [The second room is a tableau showing Tomi singing in the Gaylordia Town Square. None of the characters are moving except for Tomi's sprite, which is singing.]
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Remember how scared you were to sing in front of the other fairies at the Connection Ceremony?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I do...", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "When you sang in the Town Square, were you still afraid?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I .. I was.", false, TOMI_SAD, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Indeed. You were. But you did it anyway.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I... did.", false, TOMI_SAD, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Come now. Not much further.", false, 0, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
