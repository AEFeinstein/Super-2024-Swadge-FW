#include <string.h>
#include "ray_cutscene_scene_05_doria_intro.h"

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneDoriaIntro(cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "DORIA_INTRO_SCENE"))
    {
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
