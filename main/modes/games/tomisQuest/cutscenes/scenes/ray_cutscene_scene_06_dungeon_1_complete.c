#include <string.h>
#include "ray_cutscene_scene_06_dungeon_1_complete.h"

/**
 * @brief Dialog callback to start the instrument tutorial
 */
static void startLullabyTutorial(void)
{
    ray_t* ray = getRayState();

    // Set the tutorial flag before switching screens to not destroy the cutscene
    ray->instrumentTutorial         = true;
    ray->instrumentTutorialAutoPlay = true;
    ray->instrumentAutoPlayIdx      = 0;
    ray->instrumentAutoPlayTimer    = 0;
    raySwitchToScreen(RAY_INSTRUMENT);
    // This will return to the cutscene when finished
}

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_DUNGEON_1_COMPLETE;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneDungeon1Complete(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "DUNGEON_1_COMPLETE_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_DUNGEON_1_COMPLETE)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_DORIA (CS_STYLE_HYUT + 1)
        addCutsceneStyleDoria(cutscene);

#define CS_STYLE_NARRATOR (CS_STYLE_DORIA + 1)
        addCutsceneStyleNarrator(cutscene);

        // clang-format off

        // [Doria is waiting in the treasure room.]
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "So, you've made it.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah, no sweat. Barely even needed that lousy JUMP you taught us. We're professionals.", false, TOMI_COCKY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "...", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Claim your prize then.", false, 0, NULL);
        // [Hyut opens the treasure chest:]
        // TODO update with final instructions
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "You got the Shield Drum! Tap the left touch strip to trigger a protective, color-coded barrier!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Tomi, this shield is one of the great Sacred Instruments. Use it wisely.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Sweet! Prizes!", false, TOMI_FIREDUP, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Still... you have only just begun the Great Fairy Trial. The entrance to the next Dungeon is locked - you do not have the tools yet to continue on.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Oh great, this Trial has backtracking in it? Laaaaame...", false, TOMI_ANGRY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "This is a Warp Pad. It will return you to Gaylorida Field once activated.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "How do I activate it?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "You must stand on the pad and sing this Sacred Melody. This is Doria's Lullaby.", false, 0, startLullabyTutorial);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "The Pause menu can remind you how to play Doria's Lullaby.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Nice tune. I could swear I've heard it before.", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DORIA, "Remember it well, fairy. You can return here if you ever need a reminder on how the melody goes.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Sure thing, weird ninja-lady.", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Doria vanishes without another word.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Huh. Weird...", false, TOMI_SUSPICIOUS, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
