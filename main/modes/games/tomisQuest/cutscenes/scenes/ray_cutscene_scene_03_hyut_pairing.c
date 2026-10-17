#include <string.h>
#include "ray_cutscene_scene_03_hyut_pairing.h"
#include "ray_player.h"

/**
 * @brief Get the Ewi after the cutscene is over
 */
static void getEwi(void)
{
    ray_t* ray = getRayState();

    // Add the Ewi to the inventory
    rayObjCommon_t ewi = {.type = OBJ_ITEM_EWI};
    rayPlayerTouchItem(ray, &ewi, ray->p.mapId);
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneHyutPairing(cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "HYUT_PAIRING_SCENE"))
    {
        // Initialize Styles

#define CS_STYLE_HYUT_QQQ (CS_STYLE_HYUT + 1)
        addCutsceneStyle(cutscene, c000, TQ_HYUT_PORTRAIT_00_NEUTRAL_WSG, CUTSCENE_TEXTBOX_WSG, "????", 2, false, true,
                         true);

#define CS_STYLE_NARRATOR (CS_STYLE_HYUT_QQQ + 1)
        addCutsceneStyleNarrator(cutscene);

        // clang-format off

        // Initialize lines

        addCutsceneLine(cutscene, CS_STYLE_HYUT_QQQ, "huff... huff... huff...", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "What is your problem, kid?!", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT_QQQ, "Ha.. Haa.. Hyaaa!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Can you talk? ", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT_QQQ, "Hyaa! Hyut!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Uhh, I guess I'll call you \"Hyut.\"", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyut! Haa!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "...", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "So... why were you here all alone in the forest? ", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaa, Hyup! Eyyaa!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Right. Well, probably the same reason I'm here. Nobody wants ya, huh. ", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hut.", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Well, what do they know, anyway? ", false, TOMI_ANGRY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "... Hup.", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Buncha jerks.", false, TOMI_ANGRY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "...", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "it's just... I just always thought.. Nah, it's stupid.", false, TOMI_SAD, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hya?", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I just - when I was little, I heard all these stories about the Great Fairy Trials. There's this old legend...", false, TOMI_SAD, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyup?", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah, it talks about one day, there'd be a fairy and a child that would join up and complete these trials set by the Great Fairies... ", false, TOMI_SAD, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I always dreamed that I'd be the one to do it - that I'd complete the trial and take my place among the Great Fairies... ", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "..?", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "But that's stupid. ", false, TOMI_ANGRY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "..!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Everyone thinks they're gonna complete the trial when they're little. But then you get older... life happens...", false, TOMI_ANGRY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "...?", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...it doesn't matter, anyway. It's not like I can complete the trial, I don't even have a kid to pair with. ", false, TOMI_SAD, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "..!! Hiyaaa!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Wow, jeez, sorry, I was totally oversharing there! Sorry about that - don't know where that emo stuff came from. Thanks for listening, I guess.", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "!! !! !! Hyaa!! ", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "What? What's your deal?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaaa!! HYAAA!!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "What, you wanna join up? You wanna take on the Great Fairy Trial together?", false, TOMI_SHOCKED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "HAAAAAAA!!!! HOOYYEEEAAAA!!!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Well, I guess you've got the enthusiasm for it...", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "HYUUUUUAAAAAHHH!!!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah!", false, TOMI_FIREDUP, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, " I mean... I guess we can give it a try?... What could go wrong?", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "HYUT has joined your party! Press B to swing Hyut's EWI of Time!", false, 0, getEwi);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
