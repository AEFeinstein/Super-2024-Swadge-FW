//==============================================================================
// Includes
//==============================================================================
#include "faceFinder.h"
#include <time.h>
#include <math.h>
#include "mainMenu.h"
// #include "settingsManager.h"
// #include "textEntry.h"

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    bool visible;
    int32_t faceNum;
    int32_t danceDuration; // Milliseconds that the face will be moving in the current direction before moving in a new
                           // direction
    vec_t pos;
    vec_t movementSpeed; //.x and .y denote what signed percent of elapsedUs to apply to their position, using
                         //millisPerPixel to move at a rate scaled to the pointer
} face_t;

typedef struct
{
    font_t* ibm;
    paletteColor_t text;
    paletteColor_t background;
    wsg_t pointer;
    menu_t* mainMenu;
    menuMegaRenderer_t* renderer;

    midiFile_t bgm_fast;
    midiFile_t bgm_med;
    midiFile_t bgm_slow;
    midiFile_t bgm_zen;
    int musicNum; //0=zen, 1=slow, 2=med, 3=fast
    midiFile_t right;
    midiFile_t wrong;
    midiFile_t die;

    wsg_t faces[7];
    int64_t timer;
    int64_t score;
    int32_t stage;

    list_t* faceList;

    vec_t pointerCoords; // x and y are int32 allowing us to turn 2billies into 280 X_pixels
    bool pointingRight;
    bool pointingLeft;
    bool pointingUp;
    bool pointingDown;

    bool ZenMode;
    bool ShowMenu;
    bool newGame;

    int32_t millisInstructing;
    bool displayingScore;
} finder_t;

//==============================================================================
// Const variables
//==============================================================================

static const char findingFacesModeName[] = "Mascot Madness";
static const char GameOverText[]         = "Game over!";
static const char TimeText[]             = "Time Attack Mode!";
static const char ZenText[]              = "Zen Mode!";
static const char ExitText[]              = "Exit!";
static const char ZenScore[]             = "Zen :)";
static const int32_t startingFaces       = 4;
static const int32_t facesPerLevel       = 2;
static const int32_t MaxStaticFaces      = 42;
static const int32_t StartTime           = 10000000;
static const int32_t TimePerLevel        = 5000000; // 20 seconds per stage, additive
static const int32_t TimePerInstruction  = 4000000; // How long is each instruction page, with ability to be skipped
static const int32_t MillisPerPixel      = 6900; // How many milliseconds of "moving right" to move one pixel to the right
static const int32_t DanceDurationMult   = 200;
static const int32_t PenaltyMillis       = 3000000; // Milliseconds penalized from the timer on a wrong click

//==============================================================================
// Function Definitions
//==============================================================================
static void findingEnterMode(void);
static void findingExitMode(void);
static void findingMainLoop(int64_t);
static void randomizeFaces(finder_t*);
static void addNewFace(finder_t*);
static void startNewGame(finder_t*);
static vec_t faceDance(void);
bool finderMainMenuCb(const char*, bool, uint32_t);

//==============================================================================
// Variables
//==============================================================================
swadgeMode_t findingFacesMode = {
    .modeName          = findingFacesModeName,
    .wifiMode          = NO_WIFI,
    .overrideUsb       = false,
    .usesAccelerometer = false,
    .usesThermometer   = false,
    .overrideSelectBtn = false,
    .fnEnterMode       = findingEnterMode,
    .fnExitMode        = findingExitMode,
    .fnMainLoop        = findingMainLoop,
    //.trophyData              = &findingTrophyData,
    .fnBackgroundDrawCallback = NULL,
};
finder_t* finder;

//==============================================================================
// Functions
//==============================================================================
static void randomizeFaces(finder_t* myfind)
{
    for (int i = 6; i > 0; i--)
    {
        int j            = rand() % (i + 1);
        wsg_t hold       = myfind->faces[i];
        myfind->faces[i] = myfind->faces[j];
        myfind->faces[j] = hold;
    }

    node_t* currentNode = myfind->faceList->first;
    face_t* foo         = (face_t*)currentNode->val;
    if (myfind->stage % 2 && myfind->stage > 4)
    {
        // Movement stage
        foo->pos.x         = (rand() % 240) * MillisPerPixel;
        foo->pos.y         = (rand() % 200) * MillisPerPixel;
        foo->danceDuration = (rand() % 32767) * DanceDurationMult;
        foo->movementSpeed = faceDance();
        foo->faceNum       = 0;
        currentNode        = currentNode->next;

        while (currentNode != NULL)
        {
            foo                = (face_t*)currentNode->val;
            foo->pos.x         = (rand() % 240) * MillisPerPixel;
            foo->pos.y         = (rand() % 200) * MillisPerPixel;
            foo->danceDuration = (rand() % 32767) * DanceDurationMult;
            foo->movementSpeed = faceDance();
            foo->faceNum       = (rand() % 6) + 1; // Ensures there is only one face[0]
            currentNode        = currentNode->next;
        }
    }
    else
    {
        // Static Grid Stage
        int numFaces = fmin(startingFaces + (facesPerLevel * finder->stage), MaxStaticFaces);
        int curInd
            = rand() % numFaces; // The face we want will be at a random position, and the rest will fill in around
        int numRows = floor(sqrt(numFaces));
        int numCols = ceil(numFaces / numRows);

        while (currentNode != NULL)
        {
            foo                = (face_t*)currentNode->val;
            foo->pos.x         = ((248 / (numCols)) * (curInd % numCols)) * MillisPerPixel;
            foo->pos.y         = ((208 / (numRows)) * floor(curInd / numCols)) * MillisPerPixel;
            foo->danceDuration = myfind->timer;
            foo->movementSpeed = (vec_t){
                .x = 0,
                .y = 0,
            };

            // Ensures there is only one face[0]
            if (currentNode == myfind->faceList->first)
            {
                foo->faceNum = 0;
            }
            else
            {
                foo->faceNum = (rand() % 6) + 1;
            }
            curInd      = (curInd + 1) % numFaces;
            currentNode = currentNode->next;
        }
    }
}
static void addNewFace(finder_t* myfind)
{
    face_t* curFace = heap_caps_calloc(1, sizeof(face_t), MALLOC_CAP_8BIT);
    push(myfind->faceList, (void*)curFace);
}
static vec_t faceDance()
{
    return (vec_t){
        .x = 75 - (rand() % 150),
        .y = 75 - (rand() % 150),
    };
}
static void startNewGame(finder_t* myFind){
    if(myFind->ZenMode){
        globalMidiPlayerPlaySong(&myFind->bgm_zen, MIDI_BGM);
        myFind->musicNum = 0;
    }else{
        globalMidiPlayerPlaySong(&myFind->bgm_fast, MIDI_BGM);
        myFind->musicNum = 3;
    }
    
    myFind->displayingScore = false;
    myFind->score           = 0;
    myFind->timer           = StartTime;
    myFind->stage           = 0;
    clear(myFind->faceList);
    for (int i = 0; i < startingFaces; i++)
    {
        addNewFace(myFind);
    }
    randomizeFaces(myFind);
    myFind->millisInstructing = TimePerInstruction;
}
bool finderMainMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (label == TimeText)
        {
            if(finder->ZenMode){
                finder->ZenMode = false;
                startNewGame(finder);
            }
            
        }else if (label == ZenText)
        {
            if(!finder->ZenMode){
                finder->ZenMode = true;
                startNewGame(finder);
            }
            
        }else if (label == ExitText)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
        finder->ShowMenu = false;
        return true;
    }else{
        return false;
    }
}
static void findingEnterMode(void)
{
    srand(time(NULL));
    finder                = heap_caps_calloc(1, sizeof(finder_t), MALLOC_CAP_8BIT);
    finder->ibm           = getSysFont();
    finder->text          = c153;
    finder->background    = c011;
    finder->stage         = 0;
    finder->score         = 0;
    finder->timer         = StartTime;
    finder->pointingDown  = false;
    finder->pointingUp    = false;
    finder->pointingLeft  = false;
    finder->pointingRight = false;
    finder->pointerCoords = (vec_t){
        .x = 0,
        .y = 0,
    };

    loadWsg(SWSN_POINTER_NO_GLOVE_NO_LOVE_WSG, &finder->pointer, true);
    loadWsg(FINDER_BATTRICE_WSG, &finder->faces[0], true);
    loadWsg(FINDER_BIGMA_WSG, &finder->faces[1], true);
    loadWsg(FINDER_DUSTCAP_WSG, &finder->faces[2], true);
    loadWsg(FINDER_GARBOTNIK_WSG, &finder->faces[3], true);
    loadWsg(FINDER_KINETIC_DONUT_WSG, &finder->faces[4], true);
    loadWsg(FINDER_PULSE_WSG, &finder->faces[5], true);
    loadWsg(FINDER_SAWTOOTH_WSG, &finder->faces[6], true);

    //Music things
    initGlobalMidiPlayer();

    loadMidiFile(FINDER_BGM_FAST_MID, &finder->bgm_fast, true);
    loadMidiFile(FINDER_BGM_MED_MID, &finder->bgm_med, true);
    loadMidiFile(FINDER_BGM_SLOW_MID, &finder->bgm_slow, true);
    loadMidiFile(FINDER_BGM_ZEN_MID, &finder->bgm_zen, true);
    loadMidiFile(FINDER_RIGHT_MID, &finder->right, true);
    loadMidiFile(FINDER_WRONGER_MID, &finder->wrong, true);
    loadMidiFile(FINDER_DIE_MID, &finder->die, true);

    finder->mainMenu = initMenu(findingFacesModeName, finderMainMenuCb);
    finder->renderer = initMenuMegaRenderer(NULL, NULL, NULL);
    addSingleItemToMenu(finder->mainMenu, TimeText);
    addSingleItemToMenu(finder->mainMenu, ZenText);
    addSingleItemToMenu(finder->mainMenu, ExitText);
    finder->ShowMenu = true;
    

    globalMidiPlayerPlaySong(&finder->bgm_fast, MIDI_BGM);
    globalMidiPlayerGet(MIDI_BGM)->loop = true;
    finder->musicNum = 3;
    
    // Load all faces
    finder->faceList = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);
    startNewGame(finder);
}
static void findingMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && !finder->ShowMenu)
        {
            switch (evt.button)
            {
                case PB_LEFT:
                {
                    finder->pointingLeft = true;
                    break;
                }
                case PB_UP:
                {
                    finder->pointingUp = true;
                    break;
                }
                case PB_RIGHT:
                {
                    finder->pointingRight = true;
                    break;
                }
                case PB_DOWN:
                {
                    finder->pointingDown = true;
                    break;
                }
                case PB_A:
                {
                    if(finder->millisInstructing > 0)
                    {
                        finder->millisInstructing = 0;
                    }else if (finder->displayingScore)
                    {
                        finder->ShowMenu = true;
                    }else
                    {
                        // Check if the pointer is on the first face
                        node_t* firstNode = finder->faceList->first;
                        face_t* firstFace = (face_t*)firstNode->val;
                        //The pointer coords should be as close to the face coords + [32,32] as possible
                        if(  abs( firstFace->pos.x/MillisPerPixel +32 - finder->pointerCoords.x/MillisPerPixel) < 22
                          && abs( firstFace->pos.y/MillisPerPixel +32 - finder->pointerCoords.y/MillisPerPixel) < 22){
                            //We have found the face!
                            globalMidiPlayerPlaySong(&finder->right, MIDI_SFX);
                            finder->stage++;
                            
                            if(!finder->ZenMode)
                            {
                                finder->score += finder->timer/1000000;
                                finder->timer += TimePerLevel;
                            }else{
                                finder->score++;
                            }
                            finder->millisInstructing = TimePerInstruction;
                            for(int i=0; i<facesPerLevel; i++){
                                addNewFace(finder);
                            }
                            randomizeFaces(finder);
                        }else{
                            //This was the wrong face
                            globalMidiPlayerPlaySong(&finder->wrong, MIDI_SFX);
                            finder->timer -= PenaltyMillis;
                        }
                    }

                    break;
                }
                case PB_B:
                    finder->ShowMenu = true;
                    break;
                case PB_START:
                {
                    if(finder->millisInstructing <= 0){
                        finder->millisInstructing = 1000000000; //Lets you pause for 1000 seconds to refresh your memory on who you're finding
                    }else{
                        finder->timer += 10000000;
                    }
                }
            }
        }
        else if(evt.down && finder->ShowMenu){
            finder->mainMenu = menuButton(finder->mainMenu, evt);
        }else
        {
            // stop moving the pointer
            switch (evt.button)
            {
                case PB_LEFT:
                {
                    finder->pointingLeft = false;
                    break;
                }
                case PB_UP:
                {
                    finder->pointingUp = false;
                    break;
                }
                case PB_RIGHT:
                {
                    finder->pointingRight = false;
                    break;
                }
                case PB_DOWN:
                {
                    finder->pointingDown = false;
                    break;
                }
            }
        }
    } // End button checking

    char outNums[32] = {0};
    if (finder->ShowMenu){
        drawMenuMega(finder->mainMenu, finder->renderer, elapsedUs);
    }
    else if (finder->displayingScore)
    {
        drawRectFilled(0, 0, 280, 240, c000);
        drawText(finder->ibm, finder->text, GameOverText, 140 - textWidth(finder->ibm, GameOverText) / 2, 55);
        snprintf(outNums, sizeof(outNums) - 1, "Final Score: %d", (int)finder->score);
        drawText(finder->ibm, finder->text, outNums, 140 - textWidth(finder->ibm, outNums) / 2, 90);
    }else if (finder->millisInstructing > 0)
    {
        // Draw an instruction screen
        drawRectFilled(0, 0, 280, 240, c000);
        drawText(finder->ibm, finder->text, "Find this face!", 20, 30);

        drawWsgSimpleScaled(&finder->faces[0], 40, 60, 3, 3);
        finder->millisInstructing -= elapsedUs;
    }else
    {
        // Decrement the timer by the elapsed us
        finder->timer -= elapsedUs;
        if (finder->timer < 0)
        {
            //check for game over, setting the music to zen mode if it is a game over
            finder->displayingScore = true;
            finder->musicNum = 0;
            globalMidiPlayerPlaySong(&finder->bgm_zen, MIDI_BGM);
        }

        //Change the music based on the current timer, ignoring tempo changes if we're already playing the current "correct" tempo
        if(!finder->ZenMode){
            switch (finder->musicNum)
            {
            case 1:
                if(finder->timer < 150000000){
                    finder->musicNum = 2;
                    globalMidiPlayerPlaySong(&finder->bgm_med, MIDI_BGM);
                }
                break;
            case 2:
                if(finder->timer < 30000000){
                    finder->musicNum = 3;
                    globalMidiPlayerPlaySong(&finder->bgm_fast, MIDI_BGM);
                }else if(finder->timer > 150000000){
                    finder->musicNum = 1;
                    globalMidiPlayerPlaySong(&finder->bgm_slow, MIDI_BGM);
                }
                break;
            case 3:
                if(finder->timer > 30000000){
                    finder->musicNum = 2;
                    globalMidiPlayerPlaySong(&finder->bgm_med, MIDI_BGM);
                }
                break;
            
            default:
                break;
            }
        }else{
            if(finder->musicNum != 0){
                finder->musicNum = 0;
                globalMidiPlayerPlaySong(&finder->bgm_zen, MIDI_BGM);
            }
            finder->timer = 1000000000; //Also make sure the player has 1000 seconds every frame, so they never ever die
        }

        // Move the pointer
        if (finder->pointingUp)
        {
            finder->pointerCoords.y -= elapsedUs;
            if (finder->pointerCoords.y < 0)
            {
                finder->pointerCoords.y = 239 * MillisPerPixel; // Wrap around to not quite the bottom pixel
            }
        }
        if (finder->pointingDown)
        {
            finder->pointerCoords.y += elapsedUs;
            if (finder->pointerCoords.y > 240 * MillisPerPixel)
            {
                finder->pointerCoords.y = MillisPerPixel; // Wrap around to not quite the top pixel
            }
        }
        if (finder->pointingLeft)
        {
            finder->pointerCoords.x -= elapsedUs;
            if (finder->pointerCoords.x < 0)
            {
                finder->pointerCoords.x = 279 * MillisPerPixel; // Wrap around to not quite the rightmost pixel
            }
        }
        if (finder->pointingRight)
        {
            finder->pointerCoords.x += elapsedUs;
            if (finder->pointerCoords.x > 280 * MillisPerPixel)
            {
                finder->pointerCoords.x = MillisPerPixel; // Wrap around to not quite the rightmost pixel
            }
        }

        // Draw the screen
        drawRectFilled(0, 0, 280, 240, finder->background); // background

        node_t* currentNode = finder->faceList->first;
        int drawnFaces      = 0;
        bool keepDrawing    = true;
        while (currentNode != NULL && keepDrawing)
        {
            face_t* currentFace = (face_t*)currentNode->val;
            currentFace->pos.x += (elapsedUs * currentFace->movementSpeed.x / 100);
            currentFace->pos.y += (elapsedUs * currentFace->movementSpeed.y / 100);
            currentFace->danceDuration -= elapsedUs;

            if (currentFace->danceDuration < 0)
            {
                currentFace->movementSpeed = faceDance();
                currentFace->danceDuration = (rand() % 32767) * DanceDurationMult;
            }
            //If the current face is too far off the screen, turn them around and make sure they come back
            if (currentFace->pos.x < -60 * MillisPerPixel)
            {
                currentFace->movementSpeed.x = -1 * currentFace->movementSpeed.x;
                currentFace->danceDuration += 15000 * MillisPerPixel / currentFace->movementSpeed.x;
            }
            if (currentFace->pos.y < -60 * MillisPerPixel)
            {
                currentFace->movementSpeed.y = -1 * currentFace->movementSpeed.y;
                currentFace->danceDuration += 15000 * MillisPerPixel / currentFace->movementSpeed.y;
            }
            if (currentFace->pos.x > 339 * MillisPerPixel)
            {
                currentFace->movementSpeed.x = -1 * currentFace->movementSpeed.x;
                currentFace->danceDuration += 15000 * MillisPerPixel / currentFace->movementSpeed.x;
            }
            if (currentFace->pos.y > 309 * MillisPerPixel)
            {
                currentFace->movementSpeed.y = -1 * currentFace->movementSpeed.y;
                currentFace->danceDuration += 15000 * MillisPerPixel / currentFace->movementSpeed.y;
            }

            drawWsg(&finder->faces[currentFace->faceNum], currentFace->pos.x / MillisPerPixel,
                    currentFace->pos.y / MillisPerPixel, false, false, 0);

            currentNode = currentNode->next;
            drawnFaces++;
            if (drawnFaces >= MaxStaticFaces && (finder->stage < 5 || finder->stage % 2 == 0))
            {
                keepDrawing = false;
            }
        }

        //Check for zen mode before drawing score and timer
        if(finder->ZenMode){
            drawText(finder->ibm, finder->text, ZenScore, 140 - textWidth(finder->ibm, ZenScore) / 2, 8);
        }else{
            // Draw current score and timer
            snprintf(outNums, sizeof(outNums) - 1, "%d", (int)finder->timer / 1000000);
            drawText(finder->ibm, finder->text, outNums, 140 - textWidth(finder->ibm, outNums) / 2, 8);
        }
        snprintf(outNums, sizeof(outNums) - 1, "%d", (int)finder->score);
        drawText(finder->ibm, finder->text, outNums, 260 - textWidth(finder->ibm, outNums), 220);
        

        // Draw pointer. This should always be last so we don't get lost in the shuffle
        drawWsgSimpleScaled(&finder->pointer, finder->pointerCoords.x / MillisPerPixel,
                            finder->pointerCoords.y / MillisPerPixel, 2, 2);
    }
 
}
static void findingExitMode(void)
{
    // Free the menu
    deinitMenu(finder->mainMenu);
    // Free the renderer
    deinitMenuMegaRenderer(finder->renderer);
    clear(finder->faceList);
    heap_caps_free(finder);
}