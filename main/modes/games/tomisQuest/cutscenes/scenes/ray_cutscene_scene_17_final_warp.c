#include <string.h>
#include "ray_cutscene_scene_17_final_warp.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_FINAL_WARP;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneFinalWarp(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "FINAL_WARP"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_FINAL_WARP)
        {
            return true;
        }

        // Initialize Styles

        // clang-format off

        // Initialize lines

        // In the final room is a final Warp Pad. Hyut is standing on it. Interact with Hyut to trigger the last line of dialogue and end the game.]
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "There you are, Hyut! Come on, let's go!", false, TOMI_FIREDUP, endScene);
        // [Tomi and Hyut warp away.]
        // THE END

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
