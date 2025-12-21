#include "mgCutscenes.h"
#include "midiPlayer.h"

typedef enum
{
    Pulse,
    Sawtooth,
    Bigma,
    TrashMan,
    AbilityUnlocked,
    SystemText,
    KineticDonut,
    JoltLapin,
    FlareGryffyn,
    CrashTurtle,
    QuestionMark,
    DeadeyeChirpzi,
    HankWaddle,
    GrindPangolin,
    DrainBat,
    SmashGorilla,
    SeverYataga,
    Jasper,
    Ember,
    Percy,
    Sunny,
    WarningMessage,
    SawtoothPostReveal,
    BlackScreen,
    TrashManUncorrupted,
    KineticDonutUncorrupted,
    JoltLapinUncorrupted,
    FlareGryffynUncorrupted,
    CrashTurtleUncorrupted,
    DeadeyeChirpziUncorrupted,
    GrindPangolinUncorrupted,
    SmashGorillaUncorrupted,
    SeverYatagaUncorrupted,
    DrainBatUncorrupted,
    DeadeyeWithoutZip,
    SunnyFlipped,
    SawtoothPostRevealFlipped,
    SawtoothFlipped,
    HankUnrevealed,
    CreditStyle,
} cutsceneCharacters;

static void setSongPitchesFromCurrentSong(mgGameData_t* gameData)
{
    switch (gameData->soundManager->currentBgmIndex)
    {
        case MG_BGM_KINETIC_DONUT:
        {
            int16_t songPitches[] = {63, 65, 68, 70, 61, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_BOSS_SEVER_YATAGA:
        {
            int16_t songPitches[] = {63, 68, 70, 73, -1, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_GRIND_PANGOLIN:
        {
            int16_t songPitches[] = {62, 65, 67, 69, 72, 74, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_BOSS_GRIND_PANGOLIN:
        {
            int16_t songPitches[] = {65, 67, 68, 70, 72, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_SEVER_YATAGA:
        {
            int16_t songPitches[] = {57, 60, 64, 67, 69, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_RIP_BARONESS:
        {
            int16_t songPitches[] = {60, 62, 64, 65, 67, 70, 72, 74, 76};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_BOSS_TRASH_MAN:
        {
            int16_t songPitches[] = {59, 62, 64, 66, 65, 69, 71, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_BOSS_BIGMA:
        {
            int16_t songPitches[] = {57, 60, 67, 65, 64, 62, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_SMASH_GORILLA:
        {
            int16_t songPitches[] = {57, 60, 62, 65, -1, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_DEADEYE_CHIRPZI:
        {
            int16_t songPitches[] = {55, 57, 60, 62, 58, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_BOSS_DEADEYE_CHIRPZI:
        {
            int16_t songPitches[] = {69, 76, 77, 79, 65, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_DRAIN_BAT:
        {
            int16_t songPitches[] = {62, 63, 65, 67, -1, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_BOSS_DRAIN_BAT:
        {
            int16_t songPitches[] = {62, 67, 68, 69, 65, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_FLARE_GRYFFYN:
        {
            int16_t songPitches[] = {64, 67, 69, 71, -1, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_BOSS_FLARE_GRYFFYN:
        {
            int16_t songPitches[] = {64, 71, 72, 74, 69, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_PRE_FIGHT:
        {
            int16_t songPitches[] = {62, 64, 65, 68, -1, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_POST_FIGHT:
        {
            int16_t songPitches[] = {62, 65, 67, 57, -1, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_INTRO_STAGE:
        case MG_BGM_MAXIMUM_HYPE_CREDITS:
        {
            int16_t songPitches[]
                = {62, 61, 60, 69, 62, 60, -1, -1}; // have 62 and 60 duplicated for some statistical weighting.
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_BOSS_HANK_WADDLE:
        {
            int16_t songPitches[] = {67, 67, 67, 67, 67, 67, 67, 67};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_OVO_LIVES:
        {
            int16_t songPitches[] = {67, 70, 60, 61, 62, 65, 67, 78};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_THE_GAUNTLET:
        {
            int16_t songPitches[] = {57, 59, 64, 66, -1, -1, -1, -1};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        case MG_BGM_THE_FINAL_MEGAJAM:
        {
            int16_t songPitches[] = {62, 62, 62, 62, 62, 62, 62, 62};
            setSongPitches(gameData->cutscene, songPitches);
            break;
        }
        default:
        {
            break;
        }
    }
}

static void unlockAbility(mgGameData_t* gameData, mgAbilities_t ability)
{
    gameData->abilities |= (1U << ability);
    writeNvs32(MG_abilitiesNVSKey, gameData->abilities);
}

void stageStartCutscene(mgGameData_t* gameData)
{
    gameData->cutscene->cbFunc = goToReadyScreen;
    setSongPitchesFromCurrentSong(gameData);

    /* clang-format off */
    switch (gameData->level)
    {
        case 10: // Intro Stage
        {
            addCutsceneLine(gameData->cutscene, SystemText,
                            "PULSE and SAWTOOTH teleport into Omega Harbor's convention center, which looks an awful "
                            "lot like the Gaylord...",
                            false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SystemText,
                            "The grand atrium is filled with flickering-banners, while pixelated distortion pulses "
                            "through the walls.",
                            false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Sawtooth, "We're too late... the MAGiX virus is already spreading.",
                            false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "This is nuts! What even is this place?", true, 1, NULL);
            addCutsceneLine(gameData->cutscene, Sawtooth,
                            "It used to be our home base... before it was corrupted. We got a ping near the Main "
                            "Stage. Let's move!",
                            false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Wait, what am I supposed to do?", true, 0, NULL);
            addCutsceneLine(gameData->cutscene, Sawtooth, "Use your rhythm-feel it out! You'll learn fast. Just GO!",
                            false, 3, NULL);
            break;
        }
        case 1: // The Bouncehaus (Kinetic Donut)
        {
            addCutsceneLine(gameData->cutscene, SystemText,
                            "KINETIC DONUT's mindspace looks like a garishly lit, neon food court. Everything bounces "
                            "to a chaotic beat.",
                            false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SystemText,
                            "Floating menus glitch in and out, and the smell of fried food lingers.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse,
                            "Woah. Did we just land in a fever dream? Or do I smell... chicken tenders?", false, 1, NULL);
            addCutsceneLine(
                gameData->cutscene, SawtoothFlipped,
                "This is the Bouncehaus. Best food in Omega Harbor. Budget-friendly, energy-rich, low judgement.", true,
                3, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            " Until Bigma's virus hit. Now no one's ordering anything sane.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "And the RemiX leader here is...?", false, 0, NULL);
            addCutsceneLine(
                gameData->cutscene, SawtoothFlipped,
                "Kinetic Donut. King of Queue Chaos. If he's been corrupted, you're in for a very loud fight.", false,
                1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Great. I haven't even had lunch.", false, 3, NULL);
            break;
        }
        case 6: // The Beat Colosseum (Smash Gorilla)
        {
            addCutsceneLine(gameData->cutscene, SystemText,
                            "PULSE and SAWTOOTH teleport into SMASH GORILLA's mindscape, a glowing festival arena "
                            "filled with thumping salsa music.",
                            false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Woah... Now this is a party.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            "Focus up! This place may look festive, but the hazards here are wired to the rhythm.",
                            false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "So the party's jumping, but the baddies are ready to rumble?",
                            false, 1, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "That's right.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Or should I say...ready to rumba?", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "... ...", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "Sawtooth teleports away.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Aw...", false, 3, NULL);
            break;
        }
        case 7: // The Foundry of Echoes (Deadeye Chirpzi)
        {
            addCutsceneLine(gameData->cutscene, SystemText,
                            "PULSE and SAWTOOTH teleport into DEADEYE CHIRPZI's mindscape...", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SystemText,
                            "A blackened metal catwalk and level all above a roiling lava chasm...", false, 0, NULL);
            addCutsceneLine(
                gameData->cutscene, SystemText,
                "The air shimmers with heat distortion, and distant metal screeches echo like distorted screams.",
                false, 0, NULL);
            addCutsceneLine(
                gameData->cutscene, Pulse,
                "This place is HOT - seems like a real labyrinth. What can you tell me about Deadeye Chirpzi?", false,
                3, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            "Be sharp. She's a formidable bounty hunter, likely the best in the galaxy. ", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            "I've heard stories of her incredible adventures a few years back - people still talk "
                            "about that gunship of hers.",
                            false, 3, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "She'll likely have eyes on you already, so be on your toes.",
                            false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "I'd love to, but I'm pretty sure my boots are melting.", false,
                            3, NULL);
            break;
        }
        case 2: // The Sunshine Speedway (Grind Pangolin)
        {
            addCutsceneLine(
                gameData->cutscene, SystemText,
                "GRIND PANGOLIN's mindscape looks like the roof of a hurtling train. Wind howls. Signs zip past.",
                false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "WHOA! Okay - I'm awake now!", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            "Always tricky teleporting onto a moving target. You got your balance?", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Think so! Is it always this fast?!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            "Not usually. Someone's pushing the throttle way past safe. You're gonna need something "
                            "heavy-hitting to stop this runaway.",
                            false, 3, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Like... a brick wall? Or maybe a sternly worded letter?", false,
                            0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "...A sternly worded letter... Sure. I'm out.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "SAWTOOTH teleports away.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "*shrug*", false, 1, NULL);
            break;
        }
        case 8: // The Lost Ballroom (Drain Bat)
        {
            addCutsceneLine(
                gameData->cutscene, SystemText,
                "DRAIN BAT's mindscape is a dark and gloomy cave. In the distance, muffled beats can be heard.", false,
                0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse,
                            "Wait, I thought you said we were heading to the Underground Stage?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            "Yep. Welcome to the Lost Ballroom. This is where all the really underground acts go.",
                            false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse,
                            "Whoa... you really know where all the cool kids hang, huh Sawtooth?", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            "Cool, cursed, corrupted-take your pick. Just watch your head...", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Great. I'll keep my head down and my playlist loud.", false,
                            1, NULL);
            break;
        }
        case 3: // The Twin Peaks (Sever Yataga)
        {
            addCutsceneLine(
                gameData->cutscene, SystemText,
                "SEVER YATAGA's mindscape is a massive EDM rave. Massive glowing torii arches frame the neon skyline.",
                false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Woah, this club is nuts! Why are there shrines here?", false,
                            2, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            "Welcome to the Twin Peaks - Omega Harbor's highest and loudest floor. It used to be a "
                            "sacred dance space...",
                            false, 3, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "Now it's Sever Yataga's turf.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "\"Yataga\"? What's that?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            "Short for Yatagarasu-a mythical three-legged crow in Shinto lore. A divine guide. It's "
                            "said to lead people through darkness.",
                            false, 3, NULL);
            addCutsceneLine(gameData->cutscene, Pulse,
                            "So why does THIS Yataga sound like a boss fight with a cover charge?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            "The leaders of the MAGPIE nest - Percy and Jasper - fused to create Sever Yataga.", false,
                            3, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            "The MAGPIES once stood for unity, guidance, accessibility... until BIGMA got to them.",
                            false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Corrupting a guiding god into a rave god...", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            "Isolation in the name of freedom. A twisted remix of what they stood for.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Then I'm heading in. Time to turn the volume down-with love.",
                            false, 1, NULL);
            break;
        }
        case 9: // Arena Hypernova (Flare Gryffyn and Jolt Lapin and Crash Turtle)
        {
            addCutsceneLine(gameData->cutscene, SystemText,
                            "PULSE teleports into Arena Hypernova, an open-air concert venue.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SystemText,
                            "Half the scaffolding is collapsing; light rigs spark and dangle dangerously, but SAWTOOTH "
                            "is not there.",
                            false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "... SAWTOOTH?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "SAWTOOTH, come in! Are you there?", false, 4, NULL);
            addCutsceneLine(gameData->cutscene, WarningMessage, "INCOMING TRANSMISSION", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, JoltLapin, "Back off, PULSE. This isn't your fight.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, FlareGryffyn,
                            "Yeah. We and your friend have some... unfinished business.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, CrashTurtle, "Interfere and you might get hurt.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "I'm not afraid of you! I can take anything you throw at me.",
                            false, 4, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped,
                            "PULSE... you should listen to them. Stay out of this. I'll take care of these guys. Head "
                            "back to base.",
                            false, 1, NULL);
            addCutsceneLine(gameData->cutscene, WarningMessage, "TRANSMISSION LOST", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "...SAWTOOTH?! ...", false, 0, NULL);
            addCutsceneLine(
                gameData->cutscene, Pulse,
                "This doesn't feel right. I know SAWTOOTH is tough... but I've got to help her. Wait for me!", false,
                1, NULL);
            break;
        }
        case 4: // The Recycled Pit in the Inferno Arena (Trash Man fake our for Ember)
        {
            addCutsceneLine(gameData->cutscene, SystemText,
                            "PULSE and SAWTOOTH teleport in. The air ripples with heatwaves and flies buzz.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Ughh... what's that awful smell?", false, 4, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "Check your coordinates. Something's not right.",
                            false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse,
                            "It says this is the right place... but it looks like a landfill.", false, 0, NULL);
            addCutsceneLine(
                gameData->cutscene, SawtoothFlipped,
                "This is supposed to be RIP BARONESS's mindscape, but this doesn't look like a demon's lair...", false,
                3, NULL);
            addCutsceneLine(gameData->cutscene, Pulse,
                            "It smells more like the green room of MAGWest after a band trip to Taco Bravo. eugh..",
                            false, 3, NULL);
            break;
        }
        case 5: // The Overture Terminal (gauntlet)
        {
            addCutsceneLine(gameData->cutscene, SystemText,
                            "SAWTOOTH and PULSE teleport onto a shattered platform in front of an enormous LED screen.",
                            false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "Sparks and corrupted stage lighting flicker around them.",
                            false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostReveal,
                            "Well... this is it. Bigma's stronghold. Final act. No do-overs.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse,
                            "We've hit every beat, cured every one of the MAGiX Leaders.. We're not backing down now.",
                            true, 1, NULL);
            addCutsceneLine(gameData->cutscene, Bigma,
                            "Ahhh, there you are! My two favorite little understudies. Took you long enough to find "
                            "the green room.",
                            false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Cut the theatrics, BIGMA! Your show's getting cancelled.",
                            false, 4, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "Bold words from a backup dancer.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Bigma,
                            "Tell-have you actually mastered the skills your little adventure gave you? Or have you "
                            "just been faking this whole time?",
                            false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostRevealFlipped,
                            "Don't listen to him. He's trying to get in your head.", false, 3, NULL);
            addCutsceneLine(
                gameData->cutscene, Bigma,
                "Oh, I'm already in his head, SAWTOOTH. In EVERYONE'S head. That's the beauty of a truly viral sound.",
                false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "I'm not here to audition, BIGMA. I'm here to end this tour.",
                            false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Bigma,
                            "Heh. I love that fire. Now step inside, if you think you've earned your spotlight. The "
                            "next act is full of surprises.",
                            false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "Let's see if you can hit every note.", false, 0, NULL);

            break;
        }
        case 11: // Bigma Stage 2 (BOSS RUSH)
        {
            addCutsceneLine(gameData->cutscene, SystemText,
                            "PULSE and SAWTOOTH teleport into a darkened void of a room.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SystemText,
                            "No walls. Just glitching floor tiles beneath their feet and distant holographic echoes of "
                            "battles past.",
                            false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse,
                            "Huh? Where even are we? This doesn't feel like a lair. It feels like... a memory?", false,
                            0, NULL);
            addCutsceneLine(
                gameData->cutscene, SawtoothPostRevealFlipped,
                "Stay sharp. I've got signal distortion across all channels. We're somewhere between systems.", false,
                1, NULL);
            addCutsceneLine(gameData->cutscene, Bigma,
                            "Welcome to the encore, PULSE. You survived the warm-up. Now it's time for the deep cuts.",
                            false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Come out and fight me already!", false, 4, NULL);
            break;
        }
        default: // Hank Final Battle
        {
            addCutsceneLine(gameData->cutscene, HankUnrevealed, "Oh, COME ON!", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Huh-?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostReveal, "Who's there?!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, HankUnrevealed, "You didn't think it would wrap up THAT easily, did you?!", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Do you recognize that voice?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, HankUnrevealed, "I wasn't talking to you, kid. I was talking to her.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, HankUnrevealed, "Isn't that right...Deadeye?", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "The other RemiXes are all revealed in the shadows, in chains. A spotlight falls on CHO.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "Me? ...No. No way... That voice...", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, HankUnrevealed, "Hahaha! YES! Deadeye.... Or should I say... Cho. I knew you'd remember me, even after all these years.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "... No. It can't be. Is that-", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Who??", false, 0, NULL);
            break;
        }
    }
    /* clang-format on */
}

void bossIntroCutscene(mgGameData_t* gameData)
{
    gameData->cutscene->cbFunc = initBossFight;
    setSongPitchesFromCurrentSong(gameData);
    gameData->changeState = MG_ST_CUTSCENE;

    /* clang-format off */
    switch (gameData->level)
    {
        case 10: // Intro Stage
        {
            addCutsceneLine(gameData->cutscene, Sawtooth, "Show yourself!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "Still barking orders, I see. Some things never change.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "That voice... no way. That's... BIGMA?!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "You remember me. Good. Saves us time. Let's skip the reunion speech and get to the main event.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Sawtooth, "What happened to you? You built this place with us!", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "Built it. Improved it. Outgrew it. I'm remixing this whole world. But you? You're just stuck on the opening track.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Sawtooth, "You've lost it!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "No. I finally found my groove.", false, 0, NULL);
            break;
        }
        case 1: // The Bouncehaus (Kinetic Donut)
        {
            if(gameData->abilities & (1U << MG_CAN_OF_SALSA_ABILITY))
            {
                addCutsceneLine(gameData->cutscene, KineticDonutUncorrupted, "Ugh... so hungry...", false, -1, NULL);
                addCutsceneLine(gameData->cutscene, KineticDonutUncorrupted, "WAIT - Is that a CAN OF SALSA you've got there? Is it chunky? Spicy? ZESTY CHIPOTLE?", false, -1, NULL);
                addCutsceneLine(gameData->cutscene, Pulse, "Uh, yeah?", false, 0, NULL);
                addCutsceneLine(gameData->cutscene, KineticDonutUncorrupted, "GIMMIE.", false, -1, NULL);
                addCutsceneLine(gameData->cutscene, Pulse, "Umm... sure?", false, 0, NULL);
                addCutsceneLine(gameData->cutscene, SystemText, "PULSE tosses KINETIC DONUT the can. He rips it open and drinks it like a sports drink.", false, 0, loseCanOfSalsa);
                addCutsceneLine(gameData->cutscene, KineticDonutUncorrupted, "OHHH you're a LIFESAVER! I think I was just hangry. My brain was stuck in \"party mode.\"", false, -1, NULL);
                addCutsceneLine(gameData->cutscene, Pulse, "So... we're good?", false, 2, NULL);
                addCutsceneLine(gameData->cutscene, KineticDonutUncorrupted, "More than good. You fixed my vibe... Here - take this. Now go stick it to whoever cancelled lunch!", false, -1, startPostFightMusic);
                unlockAbility(gameData, MG_DROP_THE_MIC_ABILITY);
                addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... PULSE receives: Drop the Mic", false, 0, NULL);
                addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... Press down and A in the air.", false, 0, NULL);
                addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... Unleash a ground-pound sound attack with a mic drop!", false, 0, NULL);
                addCutsceneLine(gameData->cutscene, Pulse, "Thanks! Hope you, uh... get a real meal.", false, 2, NULL);
                addCutsceneLine(gameData->cutscene, KineticDonutUncorrupted, "Oh, I will. Tacos first, existential crisis later!", false, -1, NULL);
            }
            else
            {
                addCutsceneLine(gameData->cutscene, Pulse, "You there! Step away from the deep fryer!!", false, 4, NULL);
                addCutsceneLine(gameData->cutscene, KineticDonut, "Oooooh-did my tacos get here early?! Love when that happens!", false, -1, NULL);
                addCutsceneLine(gameData->cutscene, Pulse, "I'm not your delivery driver!", false, 3, NULL);
                addCutsceneLine(gameData->cutscene, Pulse, "Though I AM about to serve you a beatdown.", false, 1, NULL);
                addCutsceneLine(gameData->cutscene, KineticDonut, "Wait... no tacos? UGH. That makes me... really MAD!", false, -1, NULL);
                addCutsceneLine(gameData->cutscene, SystemText, "The arena begins bouncing erratically.", false, 0, NULL);
                addCutsceneLine(gameData->cutscene, KineticDonut, "Welcome to the nosh pit, baby! Hope you brought antacids!", false, -1, NULL);
            }
            break;
        }
        case 6: // The Beat Colosseum (Smash Gorilla)
        {
            addCutsceneLine(gameData->cutscene, Pulse, "You there! Cut the music - this whole place is out of control!", false, 4, NULL);
            addCutsceneLine(gameData->cutscene, SmashGorilla, "What do YOU want, rookie?", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Uh-were you always this... articulate? I didn't realize you spoke English.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SmashGorilla, "No solo ingles, idiota.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Hey! I know enough to know that was rude.", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Are you asking for a challenge?", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, SmashGorilla, "Dejalo caer como si estuviera caliente, tonto.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Oh it's ON!", false, 2, NULL);
            break;
        }
        case 7: // The Foundry of Echoes (Deadeye Chirpzi)
        {
            addCutsceneLine(gameData->cutscene, Pulse, "Hello...?", false, 4, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "A sudden sniper shot blast cracks the floor at PULSE's feet.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "WOAH! That was close! Good thing that system text gave me a warning!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeChirpzi, "That was a warning shot. Take another step, and Echo here finishes the job..", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Two on one? That's low-even for a bounty hunter.", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeChirpzi, "This isn't about honor, rookie. It's about precision.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Then you better aim better-'cause I'm not going down easy!", false, 1, NULL);
            break;
        }
        case 2: // The Sunshine Speedway (Grind Pangolin)
        {
            addCutsceneLine(gameData->cutscene, Pulse, "Hold it, rude boy! This ride's over!", false, 4, NULL);
            addCutsceneLine(gameData->cutscene, GrindPangolin, "Oh noooo, I'm so scared! What are you gonna do-mail me a sternly worded letter?", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Uh-no, I was-wait, how did you...?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, GrindPangolin, "You're slower than a sound check at a basement show! You're off-beat, outclassed, and outta time! Let's shred!", false, -1, NULL);
            break;
        }
        case 8: // The Lost Ballroom (Drain Bat)
        {
            addCutsceneLine(gameData->cutscene, DrainBat, "B'ohoho!!", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Battrice? You don't belong in this world!", false, 4, NULL);
            addCutsceneLine(gameData->cutscene, DrainBat, "It was not by my hand that I was once again-", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Okay. Yeah, Gonna stop you right there.. It was BIGMA. Mind control. Virus. You've been corrupted.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, DrainBat, "... I was brought here by-what? I had a whole monologue...", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Do it later. Right now it's just me and you.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, DrainBat, "...Fine. Enough talk-have at you!", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Gesundheit!", false, 2, NULL);
            break;
        }
        case 3: // The Twin Peaks (Sever Yataga)
        {
            addCutsceneLine(gameData->cutscene, SeverYataga, "LEAVE. US. ALONE.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Yataga-san! It's me-Pulse! We need you back in the fight for Omega Harbor!", false, 4, NULL);
            addCutsceneLine(gameData->cutscene, SeverYataga, "WE OWE NOTHING TO THE WORLD THAT BROKE US. THIS IS OUR HAVEN. OUR RAVE. OUR RULES.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "That sounds... honestly exhausting. No naps? No snacks?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SeverYataga, "WE ARE THE SKY'S LAST SCREAM. CHALLENGE US... AND PREPARE TO BE SILENCED.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Okay, okay! Just asking nicely first! Guess we're doing this!", false, 3, NULL);
            break;
        }
        case 9: // Arena Hypernova (Flare Gryffyn and Jolt Lapin and Crash Turtle)
        {
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "PULSE! I told you to back off!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Look, I don't know what history you have with these guys, but I want to help you.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "This is personal. Stay back, or I shoot.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "...", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "...Just.... Give me a minute first.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "...", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "SAWTOOTH enters the boss room alone. From the next room, the sounds of battle can be heard-explosions, energy discharges.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "... Okay that's long enough. I'm going in.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "SAWTOOTH! Where are you?", false, 4, NULL);
            addCutsceneLine(gameData->cutscene, FlareGryffyn, "Heh... heh... your little friend might be a flake, but she still has quite the fight in her.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, FlareGryffyn, "She took down Jolt Lapin and Crash Turtle on her own. But... no one gets between me and my prey.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "What did you do to her?!", false, 4, NULL);
            addCutsceneLine(gameData->cutscene, FlareGryffyn, "Wouldn't you like to know, little punk? You ready for an encore?", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "You better not have hurt her!", false, 1, NULL);
            break;
        }
        case 4: // The Recycled Pit in the Inferno Arena (Trash Man fake our for Ember)
        {
            addCutsceneLine(gameData->cutscene, Pulse, "RIP BARONESS! ... Why are you in a cage?!", false, 4, NULL);
            addCutsceneLine(gameData->cutscene, Ember, "It's just Ember, actually. \"Baroness\" was a phase.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "So... you're not corrupted?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Ember, "Nah. I'm a demon. I'm already corrupt. That's kinda the brand.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "...Then why are you-", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Ember, "... Look I made some questionable dating choices, okay?", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, WarningMessage, "INCOMING INTRUDER SIGNAL", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, QuestionMark, "WHAHAHAHAHAHAH!!!", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "A wall explodes. A strange man dramatically skids in on his garbage hovercraft.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, TrashMan, "IT IS I -DR. GARBOTNIK. THE TRASH MAN!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Who?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, TrashMan, "THE TRASH MAN!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Ember, "Yeah, this is on me. Sorry.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Is this why it smells like melted hamsters and regret?", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, TrashMan, "Silence, fools!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "TRASH MAN reveals a saxophone he was storing in his hovercraft, along with sunglasses and a fedora.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, TrashMan, "...Now dig on this.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Ember, "Aaaaand there it is.", false, -1, NULL);
            break;
        }
        case 5: // The Overture Terminal (gauntlet)
        {
            addCutsceneLine(gameData->cutscene, Pulse, "Whew, I can't believe we made it.", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostRevealFlipped, "Don't let your guard down now, PULSE!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Where has BIGMA gone?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostRevealFlipped, "Hmm, that mixtape must be some kind of trap.", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "After getting through this terminal, we can conquer ANYTHING he throws at us!", false, 2, NULL);
            break;
        }    
        case 11: // Bigma stage 2 (BOSS RUSH)
        {
            addCutsceneLine(gameData->cutscene, Bigma,
                            "Tsk-tsk. You think the headliner performs without openers? No, no, no. First-you'll face "
                            "your greatest hits.",
                            false, 1, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostReveal,
                            "Are those...? No, wait-something's off. These aren't the RemiXes. They're... copies.",
                            false, 3, NULL);
            addCutsceneLine(gameData->cutscene, Bigma,
                            "MAGiX backups. Constructed from your past encounters, and turned against you.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Bigma,
                            "Every fight, every tactic-you taught me how to defeat you, PULSE. Time to see if you've "
                            "learned anything new.",
                            false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "UGH. A boss rush? Why is it always a boss rush?!", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostReveal,
                            "You've got this. You're faster, smarter, and stronger now. Just keep your cool.", false,
                            3, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "I'll be watching from the green room. Don't disappoint me.",
                            false, 0, NULL);
            break;
        }
        default: // Hank Final Battle
        {
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "Hank Waddle?? MY OLD LAWYER?!", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "HAHAHA! So you DO remember me!!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostRevealFlipped, "Okay-what?! Cho, you seriously need to explain what's happening.", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "BIGMA, I'm disappointed. You were supposed to be my masterpiece! All that rage! That nostalgia!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "I practically gift-wrapped your villain arc!!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "You... corrupted me??", true, 2, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostRevealFlipped, "Cho...?", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "I - I don't know, but... is this... is this about...what happened back in 2024?", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "What HAPPENED in 2024?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "I, uh... partied a LITTLE too hard at MAGFest. Woke up in a space station.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "Gear missing... Reality kind of ... bent sideways. It was a whole thing.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "And this guy - Hank, my lawyer - was texting me the whole time...", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "Do you have ANY IDEA how much damage you caused?! The legal fallout?! The paperwork?!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "You undermined EVERYTHING I built my career on!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Wait-so you turned into a villain over PAPERWORK?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "NO! .... Yes. But also-you! All of you!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "This entire scene is a time bomb of stolen aesthetics, derivative tropes, and flagrant disrespect for artistic integrity!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "You and your... kind ... are a blight on this world!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "And with my corruption program, I've shown the world just how destructive you all really are.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "I, HANK WADDLE, will finally be the one to right this wrong. ONCE AND FOR ALL!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "... We kinda lost touch after that.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "We'll stop you just like we stopped BIGMA!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "Oh... I was SO hoping you'd say that. Cue FINAL BATTLE MUSIC!", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "IT. IS. SO. ON!!", false, 0, NULL);
            break;
        }
    }
    /* clang-format on */
}

void bossOutroCutscene(mgGameData_t* gameData)
{
    gameData->cutscene->cbFunc = goToReadyScreen;
    setSongPitchesFromCurrentSong(gameData);

    gameData->changeState = MG_ST_CUTSCENE;

    // cut the music
    // mmm I don't like cutting the music.
    // globalMidiPlayerGet(MIDI_BGM)->paused = true;

    /* clang-format off */
    switch (gameData->level)
    {
        case 10: // Intro Stage
        {
            addCutsceneLine(gameData->cutscene, Pulse, "Huff... huff... You're fast, but I'm faster. It's over!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "Is it, now? You're still dancing to someone else's tune.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Let's end this. Come home, Bigma. We can fix this, together...", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "Hmmm... A fresh start sounds nice...", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "PULSE and BIGMA shake hands.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "... but I work solo.", false, 3, stopMusic);
            addCutsceneLine(gameData->cutscene, SystemText, "BIGMA lets out a corrupted Colossus Roar, stunning PULSE and SAWTOOTH.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Aghh! What... was that?!", false, 4, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "BIGMA laughs and escapes in a digital blur.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "This was just a warm-up, Pulse! Catch me on the big stage-if you can keep the beat.", false, 1, startPostFightMusic);
            addCutsceneLine(gameData->cutscene, Pulse, "...Now what? He's gone...", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "Dont worry, while you two were fighting, I was remotely hacking Bigma's systems to get his encryption key.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "That'll do. I was also able to pause that blasted level timer count down.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "You should have some room to breathe after clearing a boss.", false, 2, NULL);

            break;
        }
        case 1: // The Bouncehaus (Kinetic Donut)
        {
            addCutsceneLine(gameData->cutscene, KineticDonutUncorrupted, "Hnggh... my head's spinning... Did... Did I stage-dive into the fryer?", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Almost. You've been corrupted by MAGiX. It's not your fault.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, KineticDonutUncorrupted, "I-I couldn't stop bouncing. Everything was loud, and I was starving...", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "I get that way too sometimes.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, KineticDonutUncorrupted, "Here. Take this. It's a little messy, but it packs a punch.", false, -1, startPostFightMusic);
            unlockAbility(gameData, MG_DROP_THE_MIC_ABILITY);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... PULSE receives: Drop the Mic", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... Press down and A in the air.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... Unleash a ground-pound sound attack with a mic drop!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "You going to be okay, Your Majesty?", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, KineticDonutUncorrupted, "Yeah, I'm gonna go find something greasy and lie down.", false, -1, NULL);
            break;
        }
        case 6: // The Beat Colosseum (Smash Gorilla)
        {
            addCutsceneLine(gameData->cutscene, SmashGorillaUncorrupted, "Oof... rhythm's all outta sync.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Who's tonto now?", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, SmashGorillaUncorrupted, "Cheerio, old bean. Bit of a kerfuffle, wasn't it?", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "...Huh?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SmashGorillaUncorrupted, "Just messin' with ya. Got glitched pretty bad, huh?", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "The whole place was vibing TOO hard. You back to normal?", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SmashGorillaUncorrupted, "Yeah. Head's clearer now. Here - take this. Might help you keep the groove tight.", false, -1, startPostFightMusic);
            unlockAbility(gameData, MG_CAN_OF_SALSA_ABILITY);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... PULSE receives: Can of Salsa", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... A backup up energy tank!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... Fill it up with excess health pickups.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Thanks! Stay safe, and uh... vamanos?", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SmashGorillaUncorrupted, "NOW you're getting it.", false, -1, NULL);
            break;
        }
        case 7: // The Foundry of Echoes (Deadeye Chirpzi)
        {
            addCutsceneLine(gameData->cutscene, DeadeyeChirpziUncorrupted, "Urgghh... My head....I haven't blacked out like that since the afterparty in 2024...", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeChirpziUncorrupted, "Zip, you there, bud?", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "Zip glitches out, sparks shooting outward, and lets out a chirping noise.", false,-1, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeChirpziUncorrupted, "Hey, it's okay. You're safe now. It's over.", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Is he... talking to you?", true, 0, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeChirpziUncorrupted, "Not exactly. But I get it.", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "Zip pulses again, and then dissolves into digital motes that spiral around Cho. They settle into Cho's rig.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Is he gone?", true, 0, NULL);
            //TO DO MAKE DEADEYE ART WITHOUT ZIP, use every where after this.
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "Nah. Just back where he started. Echoes don't disappear-they resonate.", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "And now I've got surround sound.", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Glad he's still with you.", true, 2, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "Speaking of, Zip's still got some juice left in him and he's itching for one last solo.", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Wait-are you saying I get an encore?", true, 0, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "More like a beamline direct to the face. You ever feel the need to absolutely delete something?", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "Frequently.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "Then here. Let Zip ride shotgun for a bit.", true, -1, startPostFightMusic);
            unlockAbility(gameData, MG_SHOOP_DA_WOOP_ABILITY);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... PULSE receives: Shoop Da Whoop", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... Wait a bit, then fire for a charged shot!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... The swadge LED lights show when it's ready.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "... Did I just get possessed by a chorus?", true, 0, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "Nah. Just one very passionate synth creature with volume issues.", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "I love it already.", true, 1, NULL);
            break;
        }
        case 2: // The Sunshine Speedway (Grind Pangolin)
        {
            addCutsceneLine(gameData->cutscene, GrindPangolinUncorrupted, "Owwww. Where even is this?  Why do I look like a rejected street team flyer?", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Long story. You got corrupted. Tried to turn this train into a one-way ride to Ska-mageddon.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, GrindPangolinUncorrupted, "Ugghhh. I hate when I go full speed without thinking.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Happens to the best of us.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, GrindPangolinUncorrupted, "Here. Should help you keep up when things get too fast.", false, -1, startPostFightMusic);
            unlockAbility(gameData, MG_TROMBONE_SLIDE_ABILITY);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... PULSE receives: Trombone Slide", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... Press Down and A to Dash! You'll slide under some attacks!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Now that's what I'm talking about!", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, GrindPangolinUncorrupted, "Give 'em whiplash for me. And hey-thanks for the brakes.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "You got it. Enjoy your rest!", false, 1, NULL);
            break;
        }
        case 8: // The Lost Ballroom (Drain Bat)
        {
            addCutsceneLine(gameData->cutscene, DrainBatUncorrupted, "Ughhh... my head. What happened? Did the rave go fully gothic... or did I?", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "You were mind-controlled, Battrice. BIGMA's been messing with everyone.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, DrainBatUncorrupted, "Guh... Figures. I gotta get back to my village in California.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Yeah, uh... where is that exactly?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, DrainBatUncorrupted, "Beneath the cliffs of San Simeon. Population: three. We make artisanal jams.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "... Sure.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, DrainBatUncorrupted, "Anyway. You're gonna need this to make it through the next wave.", false, -1, startPostFightMusic);
            unlockAbility(gameData, MG_SURE_YOU_CAN_ABILITY);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... PULSE Receives: Sure, You Can!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... Press up and B for a rising uppercut!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... Also works in the air!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, DrainBatUncorrupted, "It's my signature move-Sure, You Can!", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Sure, I'll allow it. See ya!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, DrainBatUncorrupted, "Ugh... I hope my goat didn't eat the WiFi again.", false, -1, NULL);
            break;
        }
        case 3: // The Twin Peaks (Sever Yataga)
        {
            addCutsceneLine(gameData->cutscene, Percy, "Ngh... what happened? Did we... fuse?", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Jasper, "My feathers are aching in places I didn't know could ache. Also-was I speaking in all caps? That's usually my final form.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "You were caught in BIGMA's virus. But you're back now-and still in one piece... sort of.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Percy, "You helped us remember who we are. What we stand for.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Jasper, "And who we fight with, not just for. Like... the power of nakama, y'know?", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Happy to have you both back.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Percy, "There's one more thing we can offer.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Jasper, "It's not a weapon. But you'll know when it saves your life.", false, -1, startPostFightMusic);
            unlockAbility(gameData, MG_PLOT_ARMOR_ABILITY);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... PULSE receives: Plot Armor", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... Reduces damage taken by half!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "This feels... solid. But also, weirdly... ironic?", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Percy, "It's Plot Armor, Pulse. Like a final arc twist, except wearable.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Jasper, "No main character should be without it. Especially in the final act. Ganbatte, Pulse-san!", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Wait-so am I the main character?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Percy, "In this arc? Looks like it.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Jasper, "Just remember: believe in the us that believes in you!", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "... Okay. But, thanks.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Wait... Are you Japanese, Jasper?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Jasper, "Nah, I'm from Queens. I'm just a big ol' weeb.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Noted. See ya!", false, 2, NULL);
            break;
        }
        case 9: // Arena Hypernova (Flare Gryffyn and Jolt Lapin and Crash Turtle)
        {
            addCutsceneLine(gameData->cutscene, FlareGryffyn, "Not bad, not bad. But I've still got one last trick-HYAHHH!", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "A sudden flash, FLARE GRYFFYN takes a hit from behind and collapses.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "That oughta shut him up for a while.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "SAWTOOTH! You're OKAY! What happened?!", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "Sigh... It was a lot, seeing these guys again.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Okay, you have to tell me... What is going on here?! What is your history with the Suncats?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "...", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "...", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "Well... we've come this far. Might as well come clean.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "...", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SunnyFlipped, "...I was their leader once.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "You're... SUNNY?! Sunny McShreds of the Suncats?!! You've been hiding your identity the whole time?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SunnyFlipped, "I... had a falling out with the rest of the team a few years back. We lost touch. Life happened.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SunnyFlipped, "I was going to call them, but then BIGMA started targeting all the RemiXes... I thought it was best if I stayed hidden.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "... I'm so sorry, SAWT- I mean, SUNNY. It must have been horrible to see your friends like this.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "Sunny puts her helmet back on.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostRevealFlipped, "...", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "...Will they be OK?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostRevealFlipped, "...I honestly don't know. I hope so. But they're knocked out pretty hard.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "...", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostRevealFlipped, "Here. Take Gryffyn's Reflector Shield. You can use it to bounce back energy-based attacks.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostRevealFlipped, "Might give you an edge against BIGMA's flashier tricks.", false, 2, startPostFightMusic);
            unlockAbility(gameData, MG_REFLECTOR_SHIELD_ABILITY);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... PULSE receives: Reflector Shield", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... Press down and B to bounce back energy-based attacks!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Are you sure Gryffyn won't mind?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostRevealFlipped, "Once we put a stop to BIGMA, I'm sure he'll forgive you.", false, 1, NULL);
            break;
        }
        case 4: // The Recycled Pit in the Inferno Arena (Trash Man fake our for Ember)
        {
            addCutsceneLine(gameData->cutscene, TrashMan, "NOOO! MY ART!! YOU PHILISTINES! YOU'VE SEEN NEITHER THE LAST-", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "TRASH MAN's hovercraft malfunctions, shooting Dr. Garbotnik into deep space. A warped saxophone falls to the ground with a sad squeak.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Ember, "Finally. That jazz solo was cruel and unusual punishment.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Tell me about it.", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, Ember, "Literal. Torture. Zero out of ten. Hell would not recommend.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "So... uh... you good?", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Ember, "Been worse. Thanks for the assist.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Do you want to, like, repay me with a power-up or something?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Ember, "Pfft. I'm not a vending machine. But take that. Maybe you can weaponize jazz.", false, -1, startPostFightMusic);
            unlockAbility(gameData, MG_OBNOXIOUS_NOODLING_ABILITY);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... PULSE receives: Obnoxious Noodling", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, AbilityUnlocked, "........................................................... Tap A in the air to double-jump on a cloud of bad lyrics.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "... It honks with emotion.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Ember, "You're welcome. I guess.", false, -1, NULL);
            break;
        }
        case 5: // The Overture Terminal (gauntlet)
        {
            //addCutsceneLine(gameData->cutscene, Pulse, "No script here. No boss fight in the gauntlet.", false, -1);
            break;
        }
        case 11: // Bigma stage 2 (BOSS RUSH)
        {
            // addCutsceneLine(gameData->cutscene, Bigma,
            //                 "Well done. The understudy outlasts the entire cast. You should be proud.", false, -1);
            // addCutsceneLine(gameData->cutscene, Pulse, "Enough games! You're the only one left!", false, -1);
            // addCutsceneLine(gameData->cutscene, Bigma,
            //                 "And you're still just a fan trying to improvise your way through the finale.", false, -1);
            // addCutsceneLine(gameData->cutscene, Bigma,
            //                 "It's time to return to the Main Stage. The place where all this began. Where I first "
            //                 "heard your signal.",
            //                 false, -1);
            // addCutsceneLine(gameData->cutscene, Pulse, "... Main Stage.", false, -1);
            // addCutsceneLine(gameData->cutscene, Bigma, "Curtain's rising, PULSE. Don't keep me waiting.", false, -1);

            addCutsceneLine(gameData->cutscene, Bigma, "ROOOOOAAAAARR... WHY won't you JUST QUIT?", false, 2, NULL);

            addCutsceneLine(gameData->cutscene, Pulse, "You've made us fight through so much... But I still don't get it. Why? You've changed so much...", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "No, YOU'RE the ones who've changed!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostReveal, "What?", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "We used to be underground! We were weird! We made art! But now? Look around you!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Huh?", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "It's all so BIG. And LOUD. There are too many people. Too many crowds. Things are just too... ...mainstream!", false, 1, stopMusic);
            addCutsceneLine(gameData->cutscene, SystemText,"BIGMA throws his head back and lets out another colossus roar.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "I just want things to go back the way they were!", false, 2, NULL);

            addCutsceneLine(gameData->cutscene, Pulse, "BIGMA... I'm sorry.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "Urggghhhh....", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostReveal, "I miss the old days too sometimes...", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "I think we all do.", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostReveal, "But we can't live in the past forever... We evolve. We grow. For the next generation.", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "...", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "For the next generation...?", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "...", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "BIGMA takes PULSE's hand...", false, -1, NULL);
            break;
        }
        default: // Hank Final Battle
        {
            addCutsceneLine(gameData->cutscene, HankWaddle, "NOOO! NO!! I can't lose to YOU! You're .... A bunch of fakes! Cheap knock-offs!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, KineticDonutUncorrupted, "Well, duh.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "WHAT?!", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, GrindPangolinUncorrupted, "You think we don't know that, ya dingus?", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, DrainBatUncorrupted, "We were born... out of love!", true, -1, NULL);
            //Expedition 33 reference
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "For those that come after!", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, SmashGorillaUncorrupted, "And everything that shaped us.", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, Ember, "Flaws and all, baby.", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, Jasper, "We're not afraid to carry the torch...", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Percy, "Or light new ones.", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, FlareGryffynUncorrupted, "And maybe... even mend what was broken along the way.", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "HANK WADDLE's subsystems start exploding.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "You think we're copies, but that's not the point. We're all echoes of what came before.", true, 2, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "What do you mean?", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, SawtoothPostReveal, "The ones who inspired us - they were just riffing on the stuff they liked, too.", false, 3, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "Then what are you?", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "We're something new.", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "Something that couldn't exist without the past...", true, 1, NULL);
            addCutsceneLine(gameData->cutscene, Sunny, "...But doesn't need to live in it.", true, 1, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "So... in other words...", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, DeadeyeWithoutZip, "HANK, you're fired!!!!!!", true, -1, NULL);
            addCutsceneLine(gameData->cutscene, HankWaddle, "No! No no no-if I go down, I'll take you with-AAAAUUUGHHHH!!!!", false, 1, NULL);
            //Sawtooth reverts to her masked voice (non-meow) to get s*&!t done
            addCutsceneLine(gameData->cutscene, SawtoothFlipped, "EVERYBODY! We have to get out of here!", false, 3, stopMusic);
            addCutsceneLine(gameData->cutscene, BlackScreen, "", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, BlackScreen, "After teleporting to safety, PULSE, SAWTOOTH, BIGMA, and the freed RemiXes watch HANK WADDLE's exploding facility from a distant           .    cliffside.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, BlackScreen, "The following day, PULSE and friends return to the wreckage to reflect on their adventure.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "Sunny... was he right about us?", true, 0, NULL);
            addCutsceneLine(gameData->cutscene, Sunny, "I... don't know.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, Bigma, "Maybe you'll find out someday. But until then...", true, 2, NULL);
            addCutsceneLine(gameData->cutscene, Pulse, "...LET'S FREAKIN' PARTY!!", false, 2, startCreditMusic);
            //cutscenes play BGM
            addCutsceneLine(gameData->cutscene, CreditStyle, "THANK YOU FOR PLAYING MEGA PULSE EX!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Producer, Boss Designer: Dac", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Firmware Leader: Adam Feinstein", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Hardware Leader: Emily Anthony", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Lead Programmer, Gameplay Designer, Enemy Artist, Boss Designer: Jon Vega", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Programmer, Quality Assurance, Gameplay Designer, Enemy Artist, Script Writer, Boss Designer: James Albracht", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "May Virginia Lynn Albracht rest in peace and never be forgotten.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Reach out to james.albracht@magfest.org with LEGIT game job offers, plz.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Music Composer, Lead Script Writer, Level Designer, Gameplay Designer: Joe Newman", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Midi Engineer: Dylan Whichard", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Cutscene Artist, Script Writer, Character Designer: Kaitie Lawson", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Player and Boss and Trophy Artist: Richard Lambert @azureine-edge.bsky.social", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Environment and Enemy Artist: Emma @objetdiscret.itch.io", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Boss Artist, Character Designer: Greg Lord", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Swadgeman (MAG-TV Trailer): JFrye and Producer Scott", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Trophy Engineer: Jeremy Stintzcum", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "\"Ovo Lives!\" Composer: Livingston Rampey", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Special thanks to you: THE STONE who SPLIT the future of MAGFest in two!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Notify circuitboards@magfest.org if you want to volunteer on the swadge for next year!", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, CreditStyle, "Checkout coordinates on wplace.live lat:36.34, long:-82.68", false, 0, startTrashManMusic);

            addCutsceneLine(gameData->cutscene, BlackScreen, "Starfield. Quiet. Peaceful. Suddenly - a Garbotnik industries craft hurtles past the camera, sparking and trailing smoke.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, TrashManUncorrupted, "WHA-HAHA... I regret nothing...", false, 1, NULL);
            addCutsceneLine(gameData->cutscene, TrashManUncorrupted, "Note to self: recalibrate escape trajectory BEFORE launching dramatic exit.", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "Suddenly, a floating burrito wrapper hits him in the face.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, TrashManUncorrupted, "... Is that my lunch?", false, 2, NULL);
            addCutsceneLine(gameData->cutscene, BlackScreen, "", false, 0, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "TRASH MAN WILL RETURN... In whatever sequel he can crash into.", false, -1, NULL);
            addCutsceneLine(gameData->cutscene, SystemText, "Select New Game+ then play again with all your abilities!", false, -1, NULL);
            //globalMidiUnpauseAll();
            break;
        }
    }
    /* clang-format off */
}


/* UNUSED DIALOGUE
            addCutsceneLine(gameData->cutscene, SawtoothPostReveal, "Pulse....", false, -1);
            addCutsceneLine(gameData->cutscene, Pulse, "...Yeah?", true, -1);
            addCutsceneLine(gameData->cutscene, SawtoothPostReveal, "I think this is it. BIGMA's up ahead.", false, -1);
            addCutsceneLine(gameData->cutscene, Pulse, "I feel it too.", true, -1);
            addCutsceneLine(gameData->cutscene, Bigma, "So. You've finally made it.", false, -1);
            addCutsceneLine(gameData->cutscene, SawtoothPostReveal, "It's been a long time.", false, -1);
            addCutsceneLine(gameData->cutscene, Pulse, "You've made us fight through so much... But I still don't get it. Why? You've changed so much...", false, -1);
            addCutsceneLine(gameData->cutscene, Bigma, "No, YOU'RE the ones who've changed!", false, -1);
            addCutsceneLine(gameData->cutscene, SawtoothPostReveal, "What?", false, -1);
            addCutsceneLine(gameData->cutscene, Bigma, "We used to be underground! We were weird! We made art! But now? Look around you!", false, -1);
            addCutsceneLine(gameData->cutscene, Pulse, "Huh?", false, -1);
            addCutsceneLine(gameData->cutscene, Bigma, "It's all so BIG. And LOUD. There's too many people. Too many crowds. Things are just too... ...mainstream!", false, -1);
            addCutsceneLine(gameData->cutscene, SystemText, "BIGMA throws his head back and lets out a colossus roar.", false, 0);
            addCutsceneLine(gameData->cutscene, Bigma, "I just want things to go back the way they were! And if that means taking you down to do it...", false, -1);
            addCutsceneLine(gameData->cutscene, SawtoothPostReveal, "... Then bring it.", false, -1);
*/