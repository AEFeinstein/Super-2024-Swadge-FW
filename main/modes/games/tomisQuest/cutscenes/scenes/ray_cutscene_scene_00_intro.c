#include <string.h>
#include "ray_cutscene_scene_00_intro.h"

/**
 * @brief Dialog callback to start drawing the game mid-cutscene
 */
static void startDrawingGame(void)
{
    getRayState()->p.drawGame = true;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneIntro(cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "INTRO_SCENE"))
    {
        // Initialize Styles

#define CS_STYLE_NARRATOR (CS_STYLE_HYUT + 1)
        addCutsceneStyleNarrator(cutscene);

#define CS_STYLE_KAITIE (CS_STYLE_NARRATOR + 1)
        addCutsceneStyleKaitie(cutscene);

#define CS_STYLE_SABLE (CS_STYLE_KAITIE + 1)
        addCutsceneStyleSable(cutscene);

        // clang-format off

        // Initialize lines

        // [black screen with text.]

        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Once upon a time, in a faraway land, deep in a secluded forest...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "...lived the fairies.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "They lived in harmony, far from society and the wider kingdom of MAGRule. Once every generation, the fairies would be visited by a group of children from the nearby town of Gaylordia.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "These children would guide and keep a fairy as their personal companion.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "The bond between fairy and child was considered sacred, and legends told of a great hero who, along with their fairy, would conquer evil and achieve greatness.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "But... this is not that legend.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "This is the Legend of Tomi. The fairy without a child.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "---", false, 0, NULL);

        // [still black.]

        addCutsceneLine(cutscene, CS_STYLE_KAITIE, "Wake up, loser!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "Yeah, dingus, you're gonna miss the connection ceremony!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...huhwha?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "Of COURSE Tomi would oversleep.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_KAITIE, "Hey, the Great DeJay Tree said we had to wake her. She's up. C'mon, let's go.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "See ya!", false, 0, startDrawingGame);

        // [Scene starts: Tomi's Home.]

        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Aww jeez. Aww jeez! The Connection Ceremony is today?! I gotta hurry, or I'm going to miss the whole thing!", false, 0, NULL);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
