#include "ray_cutscene.h"
#include "ray_cutscene_npc_loot.h"

/**
 * @brief Add a cutscene style for Loot
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleLoot(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_LOOT_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Loot", 1, true, true, true);
}

/**
 * @brief Callback to mark the first dialog as viewed
 */
static void endDialog1(void)
{
    getRayState()->p.cutsceneViewed |= TQS_LOOT_1;
}

/**
 * @brief Callback to mark the second dialog as viewed
 */
static void endDialog2(void)
{
    getRayState()->p.cutsceneViewed |= TQS_LOOT_2;
}

/**
 * @brief Initialize the cutscene for Loot
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneLoot(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_LOOT (CS_STYLE_HYUT + 1)
    addCutsceneStyleLoot(cutscene);

#define CS_STYLE_UNKNOWN (CS_STYLE_LOOT + 1)
    addCutsceneStyle(cutscene, c000, TQ_LOOT_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "???", 1, true, false, true);

    // clang-format off

    // After getting Ewi, before visiting Loot
    if (!(ray->p.cutsceneViewed & TQS_LOOT_1))
    {
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
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "(psst Hyut, let's just go. I'm sure we'll figure it out.)", false, TOMI_SUSPICIOUS, endDialog1);
    }
    // After visiting Loot, before getting shield
    else if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "hmhmmMMMmmm... Maahh meeee Mmayyy Mooh Mooo....!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "(I think he's vocalizing?)", false, TOMI_SUSPICIOUS, NULL);
    }
    // After getting shield, before getting boomerang
    else if (!ray->p.i.haveBoomerang)
    {
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "O-ho! Brave Adventurers! I see you followed my ballad to receive your first of the sacred instruments!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaa?", false, HYUT_NEUTRAL, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Uhh, what?", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Are you ready for my second verse?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Ummm...", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "When roads ahead are blocked and tough", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Our heroes surely had enough", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "A path within a forest scary", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "The house of a certain dignitary", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "La la, na na naaa, hey heyy, doo do doo", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I liked it up until the singalong part.", false, TOMI_BORED, NULL);
    }
    // After getting boomerang, before getting lullaby (dialog 1)
    else if (!(ray->p.cutsceneViewed & TQS_LOOT_2))
    {
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "O-ho! Brave Adventurers! You've returned once more with new spoils in hand!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah, we got a boomerang.", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "You know what they say about boomerangs, eh?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "No?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "I can't remember either, but I'm sure it'll come back to me eventually.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "GROAN. Good one, I guess.", false, TOMI_ANGRY, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Huh? What do you mean? I told you, I forgot how the saying goes...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Can we please get on with whatever this is?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Sure! Here's a ditty I like to call, \"The Ballad of the Boomerang.\"", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Today is gonna be the day that it's gonna throw it back to youuu", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "By now, you should have somehow realized what you gotta dooo", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "I don't believe that annnyboddy-", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Please stop.", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_HYUT, "Hyaaaa!!! Hyaaaaapp!!", false, HYUT_YELLING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "What? Why are you being so cranky?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Just tell us where to go already!", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "How am I supposed to know? I ain't your dad. Do fairies have dads?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "The Great DeJay Tree is kinda like my dad...", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Go check with him then, I dunno.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Great. Thanks, as always, for nothing.", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Farewell! \"And all the roads that lead you there are wiiiindiiiiiiinnng...\"", false, 0, endDialog2);
    }
    // After getting boomerang, before getting lullaby (dialog 2)
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "OH! I get it now! \"It'll come back to me eventually!\" HAHAHAHAHA!!", false, 0, NULL);
    }
    // Final dialog
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Hiya Loot.", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Hello, brave adventurers! Are you ready to hear my final song?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Final? Do you promise?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Here goes!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "The long and winding road", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "That leads to Gaylordia Square...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "Will never disappear", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "I've seen that Square before...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Loot... Do you worry about committing copyright infringement?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_LOOT, "All the time!", false, 0, NULL);
    }

    // clang-format on
}
