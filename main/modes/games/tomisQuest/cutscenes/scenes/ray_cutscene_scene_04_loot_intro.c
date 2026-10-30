#include <string.h>
#include "ray_cutscene_scene_04_loot_intro.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_LOOT_INTRO;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneLootIntro(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "LOOT_INTRO_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_LOOT_INTRO)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_LOOT (CS_STYLE_HYUT + 1)
        addCutsceneStyleLoot(cutscene);

#define CS_STYLE_UNKNOWN (CS_STYLE_LOOT + 1)
        addCutsceneStyle(cutscene, c000, TQ_LOOT_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "???", 1, true, false, true);

        // clang-format off

        // Initialize lines

        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Well... here we are.. Gaylordia Field! I... uh...", false, TOMI_FIREDUP, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyut?", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I... I've never actually left the woods before. I have no idea what I'm doing.", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaa!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Do you know the way?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyuh! Hah!! Hyaa!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Right... of course, I don't know what I was expecting.", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_UNKNOWN, "Hoo hoo!! I know the way!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Who are you supposed to be?", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "I'm Loot, hoo hoo! I'm here to guide you!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Were you... were you just waiting for us? Are you like a stalker or something?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Hoo hoo!! Strong spirit on this one!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaaaaaaa!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Yes, indeed!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Ummmmm...", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Not to worry, intrepid adventurers! I shall sing you my ballad, which shall guide you along your way!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Just gotta do a quick tune-up here...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Uh - no thanks, we're g-", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "You don't have a capo on you by any chance? No..? It's ok, it's just a little high in my range, but it's fine, all good...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "....", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "[begins doing vocal warmups]", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "(psst Hyut, let's just go. I'm sure we'll figure it out.)", false, TOMI_SUSPICIOUS, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
