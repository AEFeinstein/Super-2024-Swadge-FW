//==============================================================================
// Includes
//==============================================================================
#include "faceFinder.h"
#include <time.h>
#include <math.h>
// #include "mainMenu.h"
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

    midiFile_t bgm_fast;
    midiFile_t bgm_med;
    midiFile_t bgm_slow;
    midiFile_t bgm_zen;
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

    int32_t millisInstructing;
    bool displayingScore;
} finder_t;

//==============================================================================
// Const variables
//==============================================================================

static const char findingFacesModeName[] = "Mascot Madness";
static const char GameOverText[]         = "Game over!";
static const int32_t startingFaces       = 4;
static const int32_t facesPerLevel       = 2;
static const int32_t MaxStaticFaces      = 40;
static const int32_t StartTime           = 10000000;
static const int32_t TimePerLevel        = 5000000; // 20 seconds per stage, additive
static const int32_t TimePerInstruction  = 4000000; // How long is each instruction page, with ability to be skipped
static const int32_t MillisPerPixel = 7500; // How many milliseconds of "moving right" to move one pixel to the right
static const int32_t DanceDurationMult = 200;
static const int32_t PenaltyMillis     = 3000000; // Milliseconds penalized from the timer on a wrong click

//==============================================================================
// Function Definitions
//==============================================================================
static void findingEnterMode(void);
static void findingExitMode(void);
static void findingMainLoop(int64_t elapsedUs);
static void randomizeFaces(finder_t*);
static void addNewFace(finder_t*);
static vec_t faceDance(void);

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

    initShapes();

    loadWsg(SWSN_POINTER_NO_GLOVE_NO_LOVE_WSG, &finder->pointer, true);
    loadWsg(FINDER_BATTRICE_WSG, &finder->faces[0], true);
    loadWsg(FINDER_BIGMA_WSG, &finder->faces[1], true);
    loadWsg(FINDER_DUSTCAP_WSG, &finder->faces[2], true);
    loadWsg(FINDER_GARBOTNIK_WSG, &finder->faces[3], true);
    loadWsg(FINDER_KINETIC_DONUT_WSG, &finder->faces[4], true);
    loadWsg(FINDER_PULSE_WSG, &finder->faces[5], true);
    loadWsg(FINDER_SAWTOOTH_WSG, &finder->faces[6], true);

    initGlobalMidiPlayer();
    loadMidiFile(FINDER_BGM_FAST_MID, &finder->bgm_fast, true);
    loadMidiFile(FINDER_BGM_MED_MID, &finder->bgm_med, true);
    loadMidiFile(FINDER_BGM_SLOW_MID, &finder->bgm_slow, true);
    loadMidiFile(FINDER_BGM_ZEN_MID, &finder->bgm_zen, true);
    loadMidiFile(FINDER_RIGHT_MID, &finder->right, true);
    loadMidiFile(FINDER_WRONGER_MID, &finder->wrong, true);
    loadMidiFile(FINDER_DIE_MID, &finder->die, true);
    globalMidiPlayerPlaySong(&finder->bgm_med, MIDI_BGM);
    globalMidiPlayerGet(MIDI_BGM)->loop = true;
    

    initGlobalMidiPlayer();
    loadMidiFile(FINDER_BGM_FAST_MID, &finder->bgm_fast, true);
    loadMidiFile(FINDER_BGM_MED_MID, &finder->bgm_med, true);
    loadMidiFile(FINDER_BGM_SLOW_MID, &finder->bgm_slow, true);
    loadMidiFile(FINDER_BGM_ZEN_MID, &finder->bgm_zen, true);
    loadMidiFile(FINDER_RIGHT_MID, &finder->right, true);
    loadMidiFile(FINDER_WRONGER_MID, &finder->wrong, true);
    loadMidiFile(FINDER_DIE_MID, &finder->die, true);
    globalMidiPlayerPlaySong(&finder->bgm_med, MIDI_BGM);
    globalMidiPlayerGet(MIDI_BGM)->loop = true;
    
    // Load all faces
    finder->faceList = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_8BIT);

    for (int i = 0; i < startingFaces; i++)
    {
        addNewFace(finder);
    }

    randomizeFaces(finder);

    finder->millisInstructing = TimePerInstruction;
    finder->displayingScore   = false;
}
static void findingMainLoop(int64_t elapsedUs)
{
    char outNums[32] = {0};
    if (finder->millisInstructing > 0)
    {
        // Draw an instruction screen
        drawRectFilled(0, 0, 280, 240, c000);
        drawText(finder->ibm, finder->text, "Find this face!", 20, 30);

        drawWsgSimpleScaled(&finder->faces[0], 40, 60, 3, 3);
        finder->millisInstructing -= elapsedUs;
    }
    else if (finder->displayingScore)
    {
        drawRectFilled(0, 0, 280, 240, c000);
        drawText(finder->ibm, finder->text, GameOverText, 140 - textWidth(finder->ibm, GameOverText) / 2, 55);
        snprintf(outNums, sizeof(outNums) - 1, "Final Score: %d", (int)finder->score / 1000000);
        drawText(finder->ibm, finder->text, outNums, 140 - textWidth(finder->ibm, outNums) / 2, 90);
    }
    else
    {
        // Decrement the timer by the elapsed us
        finder->timer -= elapsedUs;
        if (finder->timer < 0)
        {
            finder->displayingScore = true;
            globalMidiPlayerPlaySong(&finder->bgm_zen, MIDI_BGM);
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
        drawRectFilled(0, 0, 280, 240, c234); // background

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
            if (currentFace->pos.x < -60 * MillisPerPixel)
            {
                currentFace->pos.x = 339 * MillisPerPixel;
            }
            if (currentFace->pos.y < -60 * MillisPerPixel)
            {
                currentFace->pos.y = 309 * MillisPerPixel;
            }
            if (currentFace->pos.x > 339 * MillisPerPixel)
            {
                currentFace->pos.x = -60 * MillisPerPixel;
            }
            if (currentFace->pos.y > 309 * MillisPerPixel)
            {
                currentFace->pos.y = -60 * MillisPerPixel;
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

        // Draw current score and timer

        snprintf(outNums, sizeof(outNums) - 1, "%d", (int)finder->score / 1000000);
        drawText(finder->ibm, finder->text, outNums, 260 - textWidth(finder->ibm, outNums), 220);
        snprintf(outNums, sizeof(outNums) - 1, "%d", (int)finder->timer / 1000000);
        drawText(finder->ibm, finder->text, outNums, 140 - textWidth(finder->ibm, outNums) / 2, 8);

        // Draw pointer. This should always be last so we don't get lost in the shuffle
        drawWsgSimpleScaled(&finder->pointer, finder->pointerCoords.x / MillisPerPixel,
                            finder->pointerCoords.y / MillisPerPixel, 2, 2);
    }

    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
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
                        globalMidiPlayerPlaySong(&finder->bgm_fast, MIDI_BGM);
                        finder->displayingScore = false;
                        finder->score           = 0;
                        finder->timer           = StartTime;
                        finder->stage           = 0;
                        clear(finder->faceList);
                        for (int i = 0; i < startingFaces; i++)
                        {
                            addNewFace(finder);
                        }
                        randomizeFaces(finder);
                        finder->millisInstructing = TimePerInstruction;
                    }
                    else
                    {
                        // Check if the pointer is on the first face
                        node_t* firstNode = finder->faceList->first;
                        face_t* firstFace = (face_t*)firstNode->val;
                        //The pointer coords should be as close to the face coords + [32,32] as possible
                        if(  abs( firstFace->pos.x/MillisPerPixel +32 - finder->pointerCoords.x/MillisPerPixel) < 22
                          && abs( firstFace->pos.y/MillisPerPixel +32 - finder->pointerCoords.y/MillisPerPixel) < 22){
                            //We have found the face!
                            finder->stage++;
                            
                            finder->score += finder->timer;
                            finder->timer += TimePerLevel;
                            finder->millisInstructing = TimePerInstruction;
                            for(int i=0; i<facesPerLevel; i++){
                                addNewFace(finder);
                            }

                            randomizeFaces(finder);

                            if(finder->timer > 1000){
                                globalMidiPlayerPlaySong(&finder->bgm_zen, MIDI_SFX);
                            }else if(finder->timer > 150){
                                globalMidiPlayerPlaySong(&finder->bgm_slow, MIDI_SFX);
                            }else if(finder->timer > 30){
                                globalMidiPlayerPlaySong(&finder->bgm_med, MIDI_SFX);
                            }else{
                                globalMidiPlayerPlaySong(&finder->bgm_fast, MIDI_SFX);
                            }
                        }else{
                            //This was the wrong face
                            globalMidiPlayerPlaySong(&finder->wrong, MIDI_SFX);
                            finder->timer -= PenaltyMillis;
                        }
                    }

                    break;
                }
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
        else
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
}
static void findingExitMode(void)
{
    clear(finder->faceList);
    // heap_caps_free(finder->faceList);
    // heap_caps_free(finder->ibm);
    heap_caps_free(finder);
}