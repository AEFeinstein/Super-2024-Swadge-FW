#include "ray_cutscene.h"
#include "ray_cutscene_npc_trash_man.h"

/**
 * @brief Add a cutscene style for Trash Man
 *
 * @param cutscene The cutscene to add to
 */
void addCutsceneStyleTrashMan(cutscene_t* cutscene)
{
    addCutsceneStyle(cutscene, c000, TQ_TRASH_MAN_PORTRAIT_WSG, CUTSCENE_TEXTBOX_WSG, "Trash Man", 1, true, true, true);
}

/**
 * @brief Initialize the cutscene for Trash Man
 *
 * Note that Tomi is style 0 and Hyut is style 1
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to add to
 */
void initNpcCutsceneTrashMan(ray_t* ray, cutscene_t* cutscene)
{
#define CS_STYLE_TRASH_MAN (CS_STYLE_HYUT + 1)
    addCutsceneStyleTrashMan(cutscene);

    // clang-format off

    // After visiting Loot, before getting shield
    if (!ray->p.i.haveShield)
    {
        addCutsceneLine(cutscene, CS_STYLE_TRASH_MAN, "Hey there, brave adventurers! Can I interest you in-", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "(to Hyut) Don't make eye contact...just keep walking...", false, TOMI_EMBARASSED, NULL);
    }
    // After getting shield, before getting turntables
    else if (!ray->p.i.haveTurntables)
    {
        addCutsceneLine(cutscene, CS_STYLE_TRASH_MAN, "Hello again brave adventurers! Can I interest-", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Dude, NOT interested. Man, what a creep.", false, TOMI_BORED, NULL);
    }
    // After getting turntables, before getting lullaby
    else if (!ray->p.i.haveDoriasLullaby)
    {
        addCutsceneLine(cutscene, CS_STYLE_TRASH_MAN, "Brave adventurers! You will DEFINITELY not want to miss-", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "NO dude, leave us alone!", false, TOMI_ANGRY, NULL);
    }
    // After getting lullaby
    else
    {
        addCutsceneLine(cutscene, CS_STYLE_TRASH_MAN, "Brave adventurers! Please do not ignore me again, I have vital information for you to complete your quest!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Sigh, fine. What do you got for us?", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TRASH_MAN, "did you know... that to the west of us, there lies a mysterious DUNGEON? What mysteries could it possi-", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yeah dude, we already checked it out. Like, a while ago.", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TRASH_MAN, "Oh.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "See ya.", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TRASH_MAN, "Yes, well... See ya, I guess.", false, 0, NULL);
    }

    // clang-format on
}
