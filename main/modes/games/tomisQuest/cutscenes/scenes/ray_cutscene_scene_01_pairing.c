#include <string.h>
#include "ray_cutscene_scene_01_pairing.h"

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutscenePairing(cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "PAIRING_SCENE"))
    {
        // Initialize Styles

#define CS_STYLE_NARRATOR (CS_STYLE_HYUT + 1)
        addCutsceneStyleNarrator(cutscene);

#define CS_STYLE_KAITIE (CS_STYLE_NARRATOR + 1)
        addCutsceneStyleKaitie(cutscene);

#define CS_STYLE_SABLE (CS_STYLE_KAITIE + 1)
        addCutsceneStyleSable(cutscene);

#define CS_STYLE_BEAN (CS_STYLE_SABLE + 1)
        addCutsceneStyleBean(cutscene);

#define CS_STYLE_DACC (CS_STYLE_BEAN + 1)
        addCutsceneStyleDacc(cutscene);

#define CS_STYLE_KIKI (CS_STYLE_DACC + 1)
        addCutsceneStyleKiki(cutscene);

        // Bouba not in this scene

#define CS_STYLE_TREE (CS_STYLE_KIKI + 1)
        addCutsceneStyleGreatDeejayTree(cutscene);

        // clang-format off

        // Initialize lines

        addCutsceneLine(cutscene, CS_STYLE_KAITIE, "Oooh! I like that one! She's got such cool style!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "That one looks like a good swimmer...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BEAN, "Hmm...that one's probably too young. They've got a few years before they can leave home.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Let me see!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_DACC, "Woah, look! That one's carrying an ocarina!! I want to bond with them!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_KIKI, "Real great selection this time!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Let me see!!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "(They won't let me through! I better SING to get their attention.)", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Hold A to enter SING mode, then slide up and down the left touch pad to change your note!", false, 0, NULL);

        // [as the player presses A, instead of singing...]

        addCutsceneLine(cutscene, CS_STYLE_TOMI, "(On second thought, no... Nobody wants to hear that. I'll just wait for an opening.)", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Suddenly, the bushes parted, and the children saw the group of fairies for the first time. The voice of the Great DeJay tree echoed through the woods.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TREE, "Children of Gaylordia, choose wisely... your fairies stand before you.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_SABLE, "Pick me!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_BEAN, "Pick me!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_KIKI, "Pick me!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "(Oh man, no one can see me! I definitely better SING now.)", false, 0, NULL);

        // [as the player presses A, instead of singing...]

        addCutsceneLine(cutscene, CS_STYLE_TOMI, "(... Ehh, no. I'd better not annoy them. I'm sure everything will be fine.)", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "One by one, each fairy paired with a child. And Tomi was left alone. ", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...? There's no one left? Oh... okay. I guess there'll always be next time... right?", false, 0, NULL);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
