#include "ray_cutscene.h"
#include "ray_cutscene_npc_cho.h"

/**
 * @brief Add a cutscene style for Cho
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleCho(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_CHO_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Cho", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Cho
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneCho(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_CHO (CS_STYLE_HYUT + 1)
    addCutsceneStyleCho(cutscene);

    // clang-format off

    // After visiting Loot, before getting shield
    if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_CHO, "Oh, hey kid, I love your cute fairy companion! Mine's named Zip - what's yours called?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "HYUP! HAAAAAHT!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_CHO, "Hmm, that's a weird name for a fairy. They seem more like a \"Tomi\" to me...", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_CHO, "Man, have you seen the statue in the MAYOR's house?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "The thing with the mysterious connection to the Dungeon?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_CHO, "Look dude - when I see something suspicious like that, I just have to SHOOT it.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "That seems crazy. Are you crazy? What if you break it?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_CHO, "Well, guess we'll never know. I lost my gear again. Why does this always happen to me?", false, 0, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_CHO, "Seems like you're getting stronger. Nearly ready to tackle the bottom of the Dungeon, eh?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I think so...", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_CHO, "Make sure you have lots of energy tanks and missile upgrades.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Huh? What's a \"missile\"?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_CHO, "Whoops, sorry - I keep forgetting which genre I'm in.", false, 0, NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_CHO, "Hi Tomi.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Why so glum?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_CHO, "Well, my adventure here never really got off the ground.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "How come?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_CHO, "I was going to go buy a bomb to clear the rubble on the way to the Dungeon, but the store was already sold out. Someone must have got to it before me.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Oh... hehehe... yes...", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_CHO, "It's all good, I appreciated the vacation.", false, 0, NULL);
    }
    // clang-format on
}
