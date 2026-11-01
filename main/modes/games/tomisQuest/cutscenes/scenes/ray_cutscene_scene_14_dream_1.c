#include <string.h>
#include "ray_cutscene_scene_14_dream_1.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_DREAM_1;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneDream1(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "DREAM_1_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_DREAM_1)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_GFS (CS_STYLE_HYUT + 1)
        addCutsceneStyleGreatFairies(cutscene);

        // clang-format off

        // Initialize lines

        // [In the next room, Tomi is suddenly back in the Secluded Area. There are sprites of both Tomi and Hyut frozen in time, that the player (controlling Tomi) can interact with.]

        // Interacting with Dream Tomi:
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Is that... me?", false, TOMI_SHOCKED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Yes and no. This is where you began your journey, by befriending this lost child.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I look so... weak.", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Perhaps you were.", false, 0, NULL);

        // Interacting with Dream Hyut:
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "That's not really Hyut, is it...", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "It is... in a sense.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...They were so excited to go on an adventure together ...", false, TOMI_SAD, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Indeed they were. Come. Let's continue.", false, 0, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
