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

        // clang-format off

        // Initialize lines

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
