#include <string.h>
#include "ray_cutscene_scene_19_bomb_get.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_BOMB_GET;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneBombGet(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "BOMB_GET"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_BOMB_GET)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_NARRATOR CS_STYLE_HYUT + 1
        addCutsceneStyleNarrator(cutscene);

        // clang-format off

        // Initialize lines

        // Note: input instructions (bomb)
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "You bought the bombs!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Swipe up on the right touch strip to set a bomb.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "The longer the swipe, the longer the fuse.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "You'll never run out of bombs!", false, 0, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
