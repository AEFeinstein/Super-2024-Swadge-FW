#include "ray_cutscene.h"
#include "ray_cutscene_npc_ember_demon.h"

/**
 * @brief Add a cutscene style for Ember Demon
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleEmberDemon(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_EMBER_DEMON_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Ember Demon", 1, true, true,
                     true);
}

/**
 * @brief Initialize the cutscene for Ember Demon
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneEmberDemon(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_EMBER (CS_STYLE_HYUT + 1)
    addCutsceneStyleEmberDemon(cutscene);

    // clang-format off

    // After visiting Loot, before getting shield
    if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_EMBER, "'Sup.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "\"Sup?\"", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "HYUP!", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_EMBER, "Good chat.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_EMBER, "With conversational skills like that, you could be MAYOR one day.", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_EMBER, "Oh hey, you didn't die in the Dungeon. Sick.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah, it was no biggie.", false, TOMI_COCKY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_EMBER, "So you're back to gather more supplies? Something like that?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Something like that.", false, TOMI_EMBARASSED, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_EMBER, "Man, I could totally go for some EDM right now.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "EDM? Like, dance music?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_EMBER, "Yeah, I haven't been to a rave in forever. Know where I could get down?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Maybe the Great DeJay tree in the Fairy Glen? He's been known to spin.", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_EMBER, "Killer.", false, 0, NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_EMBER, "Hey Tomi, just wrapping things up here before finishing your quest?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I suppose so...", false, TOMI_EMBARASSED, NULL);
        addCutsceneLine( cutscene, CS_STYLE_EMBER, "Yeah, I'm heading out too. Just wish I had an opportunity to hear some TUNES before leaving town...", false, 0, NULL);
    }
    // clang-format on
}
