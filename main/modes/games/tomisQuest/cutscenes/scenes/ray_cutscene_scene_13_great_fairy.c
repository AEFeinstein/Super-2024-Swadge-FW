#include <string.h>
#include "ray_cutscene_scene_13_great_fairy.h"

/**
 * @brief Callback to mark this scene as viewed
 */
static void endScene(void)
{
    getRayState()->p.cutsceneViewed |= TQS_GREAT_FAIRY;
}

/**
 * @brief Initialize a cutscene based on the cutscene scene title
 *
 * @param ray The entire game state
 * @param cutscene The cutscene to initialize
 * @param title The title of the cutscene to initialize. May or may not match this cutscene
 * @return true if the cutscene was initialized, false if it wasn't
 */
bool initCutsceneGreatFairy(ray_t* ray, cutscene_t* cutscene, const char* title)
{
    if (0 == strcmp(title, "GREAT_FAIRY_SCENE"))
    {
        // If the cutscene was already viewed, don't bother
        if (ray->p.cutsceneViewed & TQS_GREAT_FAIRY)
        {
            return true;
        }

        // Initialize Styles

#define CS_STYLE_UNKN (CS_STYLE_HYUT + 1)
        addCutsceneStyleUnknown(cutscene);

#define CS_STYLE_NARRATOR (CS_STYLE_UNKN + 1)
        addCutsceneStyleNarrator(cutscene);

#define CS_STYLE_GF (CS_STYLE_NARRATOR + 1)
        addCutsceneStyleGreatFairy(cutscene);

#define CS_STYLE_GFS (CS_STYLE_GF + 1)
        addCutsceneStyleGreatFairies(cutscene);

        // clang-format off

        // Initialize lines

        // [After entering the final room, the player is no longer controlling Hyut - just Tomi]
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Hyut? You there? Hello?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_UNKN, "Tomi... Tomi...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "My name - they know my name!! I'm here! Great Fairies! I have conquered your trials! Please, show yourselves!", false, TOMI_FIREDUP, NULL);
        // [The Great Fairies flicker into view. It's not clear if they are actually there or merely projections.]
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yes!! Great Fairies! I knew it! The legends were true after all!", false, TOMI_FIREDUP, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "You have done well, Tomi. You have conquered all the Trials we have set for you, and fulfilled the prophecy of the Great Fairies.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Yes, Great Fairies! I followed the prophecy exactly! The road was long, but I never gave up!", false, TOMI_FIREDUP, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "We have noted your perseverance. Your completion of this trial brings honor to fairies everywhere.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Thank you!", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Tomi...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...Yes?", false, TOMI_CALM, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Congratulations! Thank you for playing!", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...Uhhh, yeah?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "The Great Fairies start to flicker away.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "So, uhh, what happens now? Where are you going?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "The Fairies disappear.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Hello? Great Fairies?", false, TOMI_PLEADING, NULL);
        // [The room is empty. Tomi goes into the next room, where there is a singular Great Fairy.]
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "You!", false, TOMI_FIREDUP, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GF, "Oh, hey - the exit is back the way you came in.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...When do I get to be a Great Fairy like you?", false, TOMI_FIREDUP, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GF, "Uh... what? Who told you that?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "It's like, the whole legend.", false, TOMI_BORED, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GF, "Huh?", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "What?", false, TOMI_SUSPICIOUS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GF, "Uh, no? The legend said you would complete the trial. It didn't say anything about you becoming a Great Fairy yourself.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "Huh? But... But... I did the whole adventure!", false, TOMI_PLEADING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GF, "Yes! And for that, we congratulate you.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "That... That's it?", false, TOMI_PLEADING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GF, "Yes? What else were you expecting? An achievement?", false, 0, NULL);
        // TODO an Achievement is unlocked: “You did it!”: Completed the Great Fairy Trials.
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "I... I thought... I did everything you asked!", false, TOMI_PLEADING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GF, "Yes. You did. You walked the path we set for you.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "But - I did all this work!", false, TOMI_PLEADING, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GF, "Yeah, like we said - Congratulations!", false, 0, NULL);
        // [Tomi begins to tear up.]
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "But... That can't be it!", false, TOMI_PLEADING_TEARS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GF, "...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "This isn't fair! I was supposed to be... Why are you doing this?! Is this like a game to you? It's not for me! This is my life!", false, TOMI_PLEADING_TEARS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GF, "Tomi... It's not like you were in any real danger. We watched over you the whole time.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "But... No... No... This wasn't how this was supposed to happen... I was supposed to... This was...", false, TOMI_PLEADING_TEARS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GF, "...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "If I don't get to be a Great Fairy, what was the point of all this?", false, TOMI_PLEADING_TEARS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GF, "Oh, Tomi...", false, 0, NULL);
        // [The other Great Fairies Reappear.]
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Tomi, please understand. One cannot achieve greatness by simply following the path others have laid out for them.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...?", false, TOMI_PLEADING_TEARS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "All of us Great Fairies you see before you... We are here because we made our own paths. Took risks. We found our own ways to live in the world - ways that were new.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "But... But...", false, TOMI_PLEADING_TEARS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "I'm sorry, Tomi... You're not one of us.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...I can't... I ... no ...", false, TOMI_PLEADING_TEARS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Please. Do not despair.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_NARRATOR, "Tomi is motionless, tears rolling down her face.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "Your journey has not been for naught.", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "How can you say that?", false, TOMI_PLEADING_TEARS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "...", false, 0, NULL);
        addCutsceneLine(cutscene, CS_STYLE_TOMI, "...", false, TOMI_PLEADING_TEARS, NULL);
        addCutsceneLine(cutscene, CS_STYLE_GFS, "I have an idea. Follow us, Tomi.", false, 0, endScene);

        // clang-format on

        return true;
    }
    else
    {
        return false;
    }
}
