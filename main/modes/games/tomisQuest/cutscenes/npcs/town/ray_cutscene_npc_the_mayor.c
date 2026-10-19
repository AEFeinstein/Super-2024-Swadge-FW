#include "ray_cutscene.h"
#include "ray_cutscene_npc_the_mayor.h"

/**
 * @brief Add a cutscene style for The Mayor
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleTheMayor(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_THE_MAYOR_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "The Mayor", 1, true, true, true);
}

/**
 * @brief Add a cutscene style for a sign
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleSign(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, 0, CUTSCENE_TEXTBOX_WSG, "Signpost", 1, true, false, true);
}

/**
 * @brief Initialize the cutscene for The Mayor
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneTheMayor(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_MAYOR (CS_STYLE_HYUT + 1)
    addCutsceneStyleTheMayor(cutscene);

#define CS_STYLE_SIGN (CS_STYLE_MAYOR + 1)
    addCutsceneStyleSign(cutscene);

    // clang-format off

    // After visiting Loot, before getting shield
    if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_MAYOR, "Welcome, brave adventures to the fine town of Gaylordia! Here, heroes from all walks of life come to connect, play games, hear great tunes, and maybe purchase some fine goods from our merch- I mean, ITEM SHOP!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "That's a cool-looking statue you got there.", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_MAYOR, "Ah, you have a fine eye for antiquities, but sadly that item is not for sale! It is said to have strange connections to the DUNGEON that sits to the West of our fair village.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "You have a Dungeon? Oh, I bet that's where I'll find the Great Fairy Trials!", false, TOMI_FIREDUP, NULL);
        addCutsceneLine(cutscene, CS_STYLE_MAYOR, "Ah, true, but sadly the way to the DUNGEON has been blocked by RUBBLE for some time.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaa! HYUP!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_MAYOR, "I like your positive attitude, brave adventurer!", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_SIGN, "Out to lunch on important matters completely unrelated to my statue", false, 0, NULL);
        // TODO [when the player shoots the statue with the Harpbow]
        // addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "A satisfying \"click\" can be heard.", false, 0, NULL);
        // addCutsceneLine(cutscene, CS_STYLE_TOMI, "I knew it! Let's go see if we unlocked the next stage of the Dungeon!", false, TOMI_FIREDUP, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_MAYOR, "Hello again, adventurers!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Oh hey Mayor, we didn't do anything to your statue while you were gone.", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_MAYOR, "Oh phew, that's a relief! 'Cause if you had touched it, I had set up this whole expensive home security system with lasers and spike pits and sharks...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "But we didn't see any -", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "HYAA! HUP!!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "-Right, yeah, phew! Good thing!", false, TOMI_EMBARASSED, NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_MAYOR, "Hello, brave adventurer!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Good day, Mayor.", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_MAYOR, "Have you figured out the secret of the DUNGEON yet?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I have... but it's going to require me to do something I really don't want to do...", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_MAYOR, "What's that?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I'm going to have to SING in the Town Square...", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_MAYOR, "Oh! Sounds lovely! Should I prepare an audience for you? Or a stage?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "No!! Please don't...", false, TOMI_SHOCKED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_MAYOR, "If you say so... looking forward to your performance!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Ehhehh, yeah...", false, TOMI_EMBARASSED, NULL);
    }
    // clang-format on
}
