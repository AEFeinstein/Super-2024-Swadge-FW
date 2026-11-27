#include <string.h>
#include "ray_cutscene_scene_20_bow_get.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_BOW_GET;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneBowGet(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "BOW_GET"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_BOW_GET)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_NARRATOR CS_STYLE_HYUT + 1
        addCutsceneStyleNarrator(cutscene);

        // clang-format off

        // Initialize lines

        // Note: input instructions (bow)
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "You found the bow!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Swipe down on the right touch strip to draw an arrow and release to fire.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "The longer the swipe, the faster the arrow.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "You'll never run out of arrows!", false, 0, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
