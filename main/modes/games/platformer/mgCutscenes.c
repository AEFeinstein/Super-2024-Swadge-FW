#include "mgCutscenes.h"

void levelStartCutscene(mgGameData_t* gameData)
{
    switch(gameData->level)
    {
        case 1:
        {
            addCutsceneLine(gameData->cutscene, Sawtooth, "We're too late... the MAGiX virus is already spreading.", true, 1);
            addCutsceneLine(gameData->cutscene, Pulse, "This is nuts! What even is this place?", false, 0);
            addCutsceneLine(gameData->cutscene, Sawtooth, "It used to be our home base... before it was corrupted. We got a ping near the Main Stage. Let's move!", true, 0);
            addCutsceneLine(gameData->cutscene, Pulse, "Wait, what am I supposed to do?", false, 4);
            addCutsceneLine(gameData->cutscene, Sawtooth, "Use your rhythm-feel it out! You'll learn fast. Just GO!", true, 3);
            break;
        }
        case 2:
        {
            break;
        }
        case 3:
        {
            break;
        }
        case 4:
        {
            break;
        }
        case 5:
        {
            break;
        }
        case 6:
        {
            break;
        }
        case 7:
        {
            break;
        }
        case 8:
        {
            break;
        }
        case 9:
        {
            break;
        }
        case 10:
        {
            break;
        }
        case 11:
        {
            break;
        }
        default://12
        {
            break;
        }
    }
}

void bossStartCutscene(mgGameData_t* gameData)
{
    gameData->changeState = MG_ST_CUTSCENE;
    switch(gameData->level)
    {
        case 1:
        {
            addCutsceneLine(gameData->cutscene, Sawtooth, "Show yourself!", false, -1);
            addCutsceneLine(gameData->cutscene, Bigma, "Still barking orders, I see. Some things never change.", false, -1);
            addCutsceneLine(gameData->cutscene, Pulse, "That voice... no way. That's... BIGMA?!", false, -1);
            addCutsceneLine(gameData->cutscene, Bigma, "You remember me. Good. Saves us time. Let's skip the reunion speech and get to the main event.", false, -1);
            addCutsceneLine(gameData->cutscene, Sawtooth, "What happened to you? You built this place with us!", false, -1);
            addCutsceneLine(gameData->cutscene, Bigma, "Built it. Improved it. Outgrew it. I'm remixing this whole world. But you? You're just stuck on the opening track", false, -1);
            addCutsceneLine(gameData->cutscene, Sawtooth, "You've lost it!", false, -1);
            addCutsceneLine(gameData->cutscene, Bigma, "No. I finally found my groove.", false, -1);
            break;
            break;
        }
        case 2:
        {
            break;
        }
        case 3:
        {
            break;
        }
        case 4:
        {
            break;
        }
        case 5:
        {
            break;
        }
        case 6:
        {
            break;
        }
        case 7:
        {
            break;
        }
        case 8:
        {
            break;
        }
        case 9:
        {
            break;
        }
        case 10:
        {
            break;
        }
        case 11:
        {
            break;
        }
        default://12
        {
            break;
        }
    }
}

void bossDeathCutscene(mgGameData_t* gameData)
{
    gameData->changeState = MG_ST_CUTSCENE;
    switch(gameData->level)
    {
        case 1:
        {
            addCutsceneLine(gameData->cutscene, Pulse, "Huff... huff... You're fast, but I'm faster. It's over!", false, -1);
            addCutsceneLine(gameData->cutscene, Bigma, "Is it, now? You're still dancing to someone else's tune.", false, -1);
            addCutsceneLine(gameData->cutscene, Pulse, "Let's end this. Come home, Bigma. We can fix this, together..", false, -1);
            addCutsceneLine(gameData->cutscene, Bigma, "Hmmm... A fresh start sounds nice...", false, -1);
            addCutsceneLine(gameData->cutscene, Bigma, "... but I work solo.", false, -1);
            addCutsceneLine(gameData->cutscene, Bigma, "ROOOAAAAAARRRRRRRR!", false, -1);
            addCutsceneLine(gameData->cutscene, Pulse, "Aghh! What... was that!?", false, -1);
            addCutsceneLine(gameData->cutscene, Bigma, "This was just a warm-up, Pulse! Catch me on the big stage-if you can keep the beat!", false, -1);
            
            break;
        }
        case 2:
        {
            break;
        }
        case 3:
        {
            break;
        }
        case 4:
        {
            break;
        }
        case 5:
        {
            break;
        }
        case 6:
        {
            break;
        }
        case 7:
        {
            break;
        }
        case 8:
        {
            break;
        }
        case 9:
        {
            break;
        }
        case 10:
        {
            break;
        }
        case 11:
        {
            break;
        }
        default://12
        {
            break;
        }
    }
}