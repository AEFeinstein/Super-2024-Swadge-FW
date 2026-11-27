#include <string.h>
#include "ray_cutscene_scene_21_turntables_get.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_TURNTABLES_GET;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneTurntablesGet(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "TURNTABLES_GET"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_TURNTABLES_GET)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_NARRATOR CS_STYLE_HYUT + 1
        addCutsceneStyleNarrator(cutscene);

        // clang-format off

        // Initialize lines

        // Note: input instructions (turntables)
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "You Got the Magic Turntables! Activate them by slashing a DJ Booth.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Use both touch strips to catch notes and scratch a fat beat!", false, 0, NULL);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
