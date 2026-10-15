#include <string.h>
#include "ray_cutscene_scene_intro.h"

static void startDrawingGame(void);

/**
 * @brief TODO doc
 *
 * @param cutscene
 * @param title
 * @return true
 * @return false
 */
bool initCutsceneIntro(cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "INTRO_SCENE"))
    {
#define CS_STYLE_NARRATOR 2
        addCutsceneStyleNarrator(cutscene);

#define CS_STYLE_KAITIE 3
        addCutsceneStyleKaitie(cutscene);

#define CS_STYLE_SABLE 4
        addCutsceneStyleSable(cutscene);

        // clang-format off

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

/**
 * @brief Dialog callback to start drawing the game mid-cutscene
 */
static void startDrawingGame(void)
{
    getRayState()->p.drawGame = true;
}
