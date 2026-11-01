#include <string.h>
#include "ray_cutscene_scene_16_dream_3.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_DREAM_3;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneDream3(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "DREAM_3_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_DREAM_3)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_GFS (CS_STYLE_HYUT + 1)
        addCutsceneStyleGreatFairies(cutscene);

        // clang-format off

        // Initialize lines

        // [The final tableau is The Great DeJay Tree, who is playing a DJ set. The other fairies are listening. NOTE: If you bought all the items in Fredward's shop, he will appear in the scene playing synths. If you tipped Loot, he will also appear playing his lute. If you did both, Doria will also appear, playing her harp.]
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Huh...I don't remember this. What is everyone doing?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "They are enjoying themselves. Celebrating the fulfillment of the Legend.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "They're celebrating...me?", false, TOMI_SHOCKED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Well, not exactly. They know the Legend's prophecy was fulfilled, but they don't know how or who did it.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I... I see. So no one knows it was me.", false, TOMI_SAD, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Well... OK. As long as they're happy.", false, TOMI_SAD, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "[smiles] You've come a long way, brave Tomi.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I... I...have?", false, TOMI_SHOCKED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Yes. And there's something else.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "?", false, TOMI_SHOCKED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Tomi... Your story is just beginning.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "..?", false, TOMI_SHOCKED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Once you learn to chart your own path, I have no doubt you will become a Great Fairy yourself one day.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I... but how will I find my own path?", false, TOMI_PLEADING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Ah... Well.. That, Tomi... is a question we cannot answer for you.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "but...", false, TOMI_PLEADING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "... I think I understand.", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "You do?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yes. Thank you, Great Fairies. For everything.", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Of course, Tomi. We eagerly await your next adventure.", false, 0, NULL);
        // [A door opens. The Great Fairies part, allowing Tomi to walk through.
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
