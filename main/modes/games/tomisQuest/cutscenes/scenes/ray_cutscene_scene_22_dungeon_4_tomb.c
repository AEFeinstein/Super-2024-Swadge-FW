#include <string.h>
#include "ray_cutscene_scene_22_dungeon_4_tomb.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_DUNGEON_4_TOMB;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneDungeon4Tomb(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "DUNGEON_4_TOMB_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_DUNGEON_4_TOMB)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_NARRATOR (CS_STYLE_HYUT + 1)
        addCutsceneStyleNarrator(cutscene);

        // clang-format off

        // Initialize lines

        // [IF TOMI MAKES IT THROUGH THE FINAL CHALLENGE ROOM]
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Tomi and Hyut enter a dimly lit room and open a large stone doorway. Inside is an old tomb. It's been ransacked. There are graves, but the headstones have been defaced; no one can make out what they say now.", false, 0, NULL);
        // [At the end of the hallway near the exit is a mural. Interact with it to trigger the next dialogue]
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "A mural depicts the sheet music to Doria's Lullaby. It has been defaced by some unknown vandals. Written all over the mural are cryptic phrases...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "\"FAKE\"", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "\"NOT THE FIRST\"", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "\"JUST A GAME\"", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "\"NOTHING NEW\"", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "\"DERIVATIVE\"", false, 0, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
