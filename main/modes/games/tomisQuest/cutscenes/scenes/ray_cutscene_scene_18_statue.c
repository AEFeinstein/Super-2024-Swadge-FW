#include <string.h>
#include "ray_cutscene_scene_18_statue.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_STATUE;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneStatue(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "STATUE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_STATUE)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_NARRATOR CS_STYLE_HYUT + 1
        addCutsceneStyleNarrator(cutscene);

        // clang-format off

        // Initialize lines

        // [when the player shoots the statue with the Harpbow]
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "A satisfying \"click\" can be heard.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I knew it! Let's go see if we unlocked the next stage of the Dungeon!", false, TOMI_FIREDUP, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
