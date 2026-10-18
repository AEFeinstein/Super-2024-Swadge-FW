#include "ray_cutscene.h"
#include "ray_cutscene_npc_kiki_bouba.h"

/**
 * @brief Add a cutscene style for Kiki
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleKiki(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_KIKI_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Kiki", 1, true, true, true);
}

/**
 * @brief Add a cutscene style for Bouba
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleBouba(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_BOUBA_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Bouba", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Bouba
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneKikiBouba(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_KIKI (CS_STYLE_HYUT + 1)
    addCutsceneStyleKiki(cutscene);

#define CS_STYLE_BOUBA (CS_STYLE_KIKI + 1)
    addCutsceneStyleBouba(cutscene);

    // clang-format off

    if (!ray->p.i.haveEwiOfTime)
    {
        // Before getting Ewi
        addCutsceneLine(cutscene, CS_STYLE_KIKI, "Hey, how'd you feel about a swap? Maybe an open companionship?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BOUBA, "No way! Me and my kid are inseparable!", false, 0, NULL);
    }
    // After getting Ewi, before visiting Loot
    else if (!(ray->p.cutsceneViewed & TQS_LOOT_INTRO))
    {
        addCutsceneLine(cutscene, CS_STYLE_KIKI, "Psst... Did you see Tomi's kid?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BOUBA, "Uhhh yeah - what's wrong with them?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_KIKI, "Do you think Tomi kidnapped someone? Should we tell the Great DeJay Tree?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Just keep walking, Hyut.", false, TOMI_BORED, NULL);
    }
    // After visiting Loot, before getting shield
    else if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_KIKI, "No, I want to trade back! This kid's aim is terrible!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BOUBA, "No take-backs!", false, 0, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_KIKI, "What if we worked out some kind of like, agreement? Like, you get my kid on weekends -", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BOUBA, "Jeez Kiki, your kid is right there!", false, 0, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_KIKI, "Hey kid, I don't want you speaking to that other two-timing Bouba anymore. I don't trust them for a second.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BOUBA, "Ugh, that Kiki is such a drama queen!", false, 0, NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_KIKI, "Hi Tomi!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Oh hey guys! How's the whole kid-sharing thing going?", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BOUBA, "Yep! We've got a nice arrangement. Good boundaries set, emotional intimacy established.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Nice! What's your secret?", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_KIKI, "Secret? Who told you anything about us keeping secrets?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BOUBA, "What did you hear?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Never mind. Good luck, guys!", false, TOMI_BORED, NULL);
    }

    // clang-format on
}
