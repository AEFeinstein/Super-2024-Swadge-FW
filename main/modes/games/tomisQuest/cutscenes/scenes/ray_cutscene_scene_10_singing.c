#include <string.h>
#include "ray_cutscene_scene_10_singing.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_SINGING;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneSinging(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "SINGING_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_SINGING)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_KING_DONUT (CS_STYLE_HYUT + 1)
        addCutsceneStyleKingDonut(cutscene);

#define CS_STYLE_PULSE (CS_STYLE_KING_DONUT + 1)
        addCutsceneStyleMegaPulse(cutscene);

#define CS_STYLE_SAWTOOTH (CS_STYLE_PULSE + 1)
        addCutsceneStyleSawtooth(cutscene);

#define CS_STYLE_CHO (CS_STYLE_SAWTOOTH + 1)
        addCutsceneStyleCho(cutscene);

#define CS_STYLE_EMBER (CS_STYLE_CHO + 1)
        addCutsceneStyleEmberDemon(cutscene);

#define CS_STYLE_NARRATOR (CS_STYLE_EMBER + 1)
        addCutsceneStyleNarrator(cutscene);

        // clang-format off

        // Initialize lines

        // [Tomi overcomes her fears and sings Doria's Lullaby in the town square, causing the Shrine of the Great Fairies to appear.]
        // [This cutscene is triggered after Tomi sings DORIA'S LULLABY in the town square of Gaylordia and after the entire melody, Doria's Lullaby (reprise) plays out.]
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Hyut?", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaa?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I had my eyes closed. Was anyone watching?", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaa!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "What's that sound? Oh jeez... is it?", false, TOMI_SHOCKED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaa!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Is that... Applause?", false, TOMI_SHOCKED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_KING_DONUT, "Bravo! Jolly good show!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_PULSE, "Wooo!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_SAWTOOTH, "Wooo!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_CHO, "Nice one, Tomi!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_EMBER, "Sick tune, dude.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...", false, TOMI_SHOCKED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hiyyya??", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Suddenly, a deep rumbling shakes Gaylordia Town Square. The fountain shook, and a secret warp pad emerged!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Oh man... I can't believe it actually worked! If nothing happened, I was ready to die of embarrassment.", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "HYYAAA!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Welp... here goes nothing..!", false, TOMI_EMBARASSED, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
