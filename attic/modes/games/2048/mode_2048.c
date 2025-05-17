/**
 * @file mode_2048.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
 * @version 1.5.1
 * @date 2024-06-28
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>

#include "mode_2048.h"
#include "2048_game.h"
#include "2048_menus.h"
#include "textEntry.h"

#include "advanced_usb_control.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static void t48EnterMode(void);
static void t48ExitMode(void);
static void t48MainLoop(int64_t elapsedUs);
static void t48BgmCb(void);
static void t48InitHighScores(void);
static void t48SortHighScores(void);
static paletteColor_t t48_generateRainbow(void);
static void t48_fadeLEDs(int32_t elapsedUs);
static void t48_chaseLEDs(int32_t elapseUs);
static led_t t48_randColor(void);
static void t48BackgroundDraw(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

//==============================================================================
// Const Variables
//==============================================================================

const char t48Name[]           = "2048";
static const char youWin[]     = "You got 2048!";
static const char continueAB[] = "Press A or B to continue";

static const char* loadGameStrs[] = {"Load save?", "Press A to load saved game", "Press B to make a new game",
                                     "Press any other key return to title screen. Will not delete your save."};

static const char* tileSpriteNames[] = {
    "Tile-Blue-Diamond.wsg", "Tile-Blue-Square.wsg",  "Tile-Cyan-Legs.wsg",      "Tile-Green-Diamond.wsg",
    "Tile-Green-Octo.wsg",   "Tile-Green-Square.wsg", "Tile-Mauve-Legs.wsg",     "Tile-Orange-Legs.wsg",
    "Tile-Pink-Diamond.wsg", "Tile-Pink-Octo.wsg",    "Tile-Pink-Square.wsg",    "Tile-Purple-Legs.wsg",
    "Tile-Red-Octo.wsg",     "Tile-Red-Square.wsg",   "Tile-Yellow-Diamond.wsg", "Tile-Yellow-Octo.wsg",
};

static const char* sparkleSpriteNames[] = {
    "Sparkle_Blue.wsg", "Sparkle_Cyan.wsg",   "Sparkle_Green.wsg", "Sparkle_Orange.wsg",
    "Sparkle_Pink.wsg", "Sparkle_Purple.wsg", "Sparkle_Red.wsg",   "Sparkle_Yellow.wsg",
};

static const char* newSparkleSprNames[] = {
    "New_Dot.wsg",
    "New_Small_Star.wsg",
    "New_Med_Star.wsg",
};

const char highScoreKey[T48_HS_COUNT][T48_HS_KEYLEN] = {
    "t48HighScore0", "t48HighScore1", "t48HighScore2", "t48HighScore3", "t48HighScore4",
};

const char highScoreInitialsKey[T48_HS_COUNT][T48_HS_KEYLEN] = {
    "t48HSInitial0", "t48HSInitial1", "t48HSInitial2", "t48HSInitial3", "t48HSInitial4",
};

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t t48Mode = {
    .modeName                 = t48Name,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = t48EnterMode,
    .fnExitMode               = t48ExitMode,
    .fnMainLoop               = t48MainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = t48BackgroundDraw,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

t48_t* t48;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter 2048 mode and set everything up
 */
static void t48EnterMode(void)
{
    setFrameRateUs(16667); // 60 FPS

    // Init Mode & resources
    t48 = heap_caps_calloc(sizeof(t48_t), 1, MALLOC_CAP_8BIT);

    // Load fonts
    loadFont("ibm_vga8.font", &t48->font, false);
    loadFont("sonic.font", &t48->titleFont, false);
    makeOutlineFont(&t48->titleFont, &t48->titleFontOutline, false);

    // Load images
    t48->tiles = heap_caps_calloc(ARRAY_SIZE(tileSpriteNames), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int32_t tIdx = 0; tIdx < ARRAY_SIZE(tileSpriteNames); tIdx++)
    {
        loadWsg(tileSpriteNames[tIdx], &t48->tiles[tIdx], true);
    }

    t48->sparkleSprites = heap_caps_calloc(ARRAY_SIZE(sparkleSpriteNames), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(sparkleSpriteNames); sIdx++)
    {
        loadWsg(sparkleSpriteNames[sIdx], &t48->sparkleSprites[sIdx], true);
    }

    t48->newSparkles = heap_caps_calloc(ARRAY_SIZE(newSparkleSprNames), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(newSparkleSprNames); sIdx++)
    {
        loadWsg(newSparkleSprNames[sIdx], &t48->newSparkles[sIdx], true);
    }

    // Load sounds
    loadMidiFile("lullaby_in_numbers.mid", &t48->bgm, true);
    loadMidiFile("sndBounce.mid", &t48->click, true);

    // Init sound player
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    midiGmOn(player);

    // Init Text Entry
    textEntryInit(&t48->font, 4, t48->playerInitials);
    textEntrySetBGColor(c001);
    textEntrySetEmphasisColor(c500);
    textEntrySetNewCapsStyle(true);
    textEntrySetNewEnterStyle(true);
    textEntrySetCapMode();

    // Initialize the scores
    t48InitHighScores();

    // Initialize the game
    t48->state = T48_START_SCREEN;
    t48_initStartScreen(t48);
}

/**
 * @brief Exit 2048 mode and free all resources
 */
static void t48ExitMode(void)
{
    freeFont(&t48->titleFontOutline);
    freeFont(&t48->titleFont);
    freeFont(&t48->font);

    for (uint8_t i = 0; i < ARRAY_SIZE(sparkleSpriteNames); i++)
    {
        freeWsg(&t48->sparkleSprites[i]);
    }
    heap_caps_free(t48->sparkleSprites);

    for (uint8_t i = 0; i < ARRAY_SIZE(tileSpriteNames); i++)
    {
        freeWsg(&t48->tiles[i]);
    }
    heap_caps_free(t48->tiles);

    for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(newSparkleSprNames); sIdx++)
    {
        freeWsg(&t48->newSparkles[sIdx]);
    }
    heap_caps_free(t48->newSparkles);

    soundStop(true);
    unloadMidiFile(&t48->bgm);
    unloadMidiFile(&t48->click);

    heap_caps_free(t48);
}

/**
 * @brief The main game loop, responsible for input handling, game logic, and animation
 *
 * @param elapsedUs The time since this was last called, for animation
 */
static void t48MainLoop(int64_t elapsedUs)
{
    // Dim LEDs
    t48_fadeLEDs(elapsedUs);

    // Play BGM if it's not playing
    if (!t48->bgmIsPlaying)
    {
        soundPlayBgmCb(&t48->bgm, MIDI_BGM, t48BgmCb);
        t48->bgmIsPlaying = true;
    }

    // Handle inputs
    buttonEvt_t evt;
    switch (t48->state)
    {
        case T48_IN_GAME:
        {
            // Get inputs
            while (checkButtonQueueWrapper(&evt))
            {
                bool isDpad = (PB_UP == evt.button) ||   //
                              (PB_DOWN == evt.button) || //
                              (PB_LEFT == evt.button) || //
                              (PB_RIGHT == evt.button);
                // Accept buttons if it's not tilt, or not D-Pad
                if (evt.down && (!t48->tiltControls || !isDpad))
                {
                    t48_gameInput(t48, evt.button);
                }
            }

            // Take accel input if cells aren't moving
            if (t48->tiltControls && !t48->cellsAnimating)
            {
                // int16_t ox, oy, oz;
                float q[4];
                esp_err_t aq = accelGetQuaternion(q);
                if (ESP_OK == aq)
                {
                    float qRelativeRotation[4];

                    // this takes about 36 flops.
                    mathComputeQuaternionDeltaBetweenQuaternions(qRelativeRotation, t48->quatBase, q);

                    // qRelativeRotation represents the rotation from the initial state to now.
                    // The quaternion is:  [w, x, y, z]
                    //  the w term, more like the part that sums everything to a unit quaternion.
                    //  the x term is the amount of rotation around the x axis.
                    //  the y term is the amount of rotation around the y axis.
                    //  the z term is the amount of rotation around the z axis.
                    // Not quite like euler angles, but kinda?
                    //
                    // I am not sure why our coordinate frame is different, but, we can go back to
                    // what the rest of the game logic is expecting here.
                    int oy = qRelativeRotation[1] * 512;
                    int ox = -qRelativeRotation[2] * 512;

                    const int trigger = 104;
                    const int release = 80;

                    bool bXPressed      = ABS(ox) > trigger;
                    bool bXReleased     = ABS(ox) < release;
                    bool bYWasTriggered = t48->receivedInputMask & 2;

                    bool bYPressed      = ABS(oy) > trigger;
                    bool bYReleased     = ABS(oy) < release;
                    bool bXWasTriggered = t48->receivedInputMask & 1;

                    if (!bYWasTriggered && bYPressed)
                    {
                        if (oy > 0)
                        {
                            t48_gameInput(t48, PB_DOWN);
                        }
                        else
                        {
                            t48_gameInput(t48, PB_UP);
                        }
                        bYWasTriggered = true;
                    }
                    if (bYWasTriggered && bYReleased)
                    {
                        bYWasTriggered = false;
                    }

                    if (!bXWasTriggered && bXPressed)
                    {
                        if (ox > 0)
                        {
                            t48_gameInput(t48, PB_LEFT);
                        }
                        else
                        {
                            t48_gameInput(t48, PB_RIGHT);
                        }
                        bXWasTriggered = true;
                    }
                    if (bXWasTriggered && bXReleased)
                    {
                        bXWasTriggered = false;
                    }

                    t48->receivedInputMask = (bYWasTriggered ? 2 : 0) | (bXWasTriggered ? 1 : 0);
                    t48->lastIMUx          = ox;
                    t48->lastIMUy          = oy;

                    // Handle pushing the zero around. You can push it by pushing past the endstops.
                    bool bXIsRailed = ABS(ox) > (trigger + 2);
                    bool bYIsRailed = ABS(ox) > (trigger + 2);
                    if (bXIsRailed || bYIsRailed)
                    {
                        const float fNudgeAmount = 0.04;
                        float qNudgeX[4]         = {0.999, 0.0, 0.0, 0.0};
                        if (bXIsRailed)
                        {
                            qNudgeX[2] = ox > 0 ? -fNudgeAmount : fNudgeAmount;
                        }
                        if (bYIsRailed)
                        {
                            qNudgeX[1] = oy < 0 ? -fNudgeAmount : fNudgeAmount;
                        }
                        mathQuatApply(t48->quatBase, t48->quatBase, qNudgeX);
                        mathQuatNormalize(t48->quatBase, t48->quatBase);
                    }
                }
            }

            // Loop the game
            t48_gameLoop(t48, elapsedUs);
            break;
        }
        case T48_START_SCREEN:
        {
            // Check any button is pressed
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && (PB_A == evt.button || PB_B == evt.button))
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    bool doTiltControls = PB_B == evt.button;

                    if (doTiltControls)
                    {
                        esp_err_t aq = accelGetQuaternion(t48->quatBase);
                        if (ESP_OK != aq)
                        {
                            doTiltControls = false;
                        }
                    }

                    t48->tiltControls = doTiltControls;

                    // If a save game is detected, ask if the player wants to load it.
                    size_t blob = sizeof(t48GameSaveData_t);
                    readNvsBlob(nvsSaved2048, NULL, &blob);
                    if (!readNvsBlob(nvsSaved2048, &t48->sd, &blob))
                    {
                        t48_gameInit(t48, false);
                        t48->state = T48_IN_GAME;
                    }
                    else
                    {
                        // Game data detected
                        t48->state = T48_LOAD_SAVE;
                    }
                    break;
                }
            }
            // Draw
            t48_drawStartScreen(t48, t48_generateRainbow(), elapsedUs);
            t48_chaseLEDs(elapsedUs);
            break;
        }
        case T48_LOAD_SAVE:
        {
            // AAsk player if they want to load their old save
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_A || evt.button & PB_B)
                    {
                        // Delete old save
                        eraseNvsKey(nvsSaved2048);

                        // Start new game
                        t48_gameInit(t48, evt.button == PB_A);
                        t48->state = T48_IN_GAME;
                    }
                    else
                    {
                        // Return to start screen without deleting save
                        t48->state = T48_START_SCREEN;
                    }
                }
            }
            // Draw load save/new game screen
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
            drawText(&t48->titleFont, c055, loadGameStrs[0],
                     (TFT_WIDTH - textWidth(&t48->titleFont, loadGameStrs[0])) >> 1, 48);
            drawText(&t48->font, c555, loadGameStrs[1], (TFT_WIDTH - textWidth(&t48->font, loadGameStrs[1])) >> 1,
                     TFT_HEIGHT - 128);
            drawText(&t48->font, c555, loadGameStrs[2], (TFT_WIDTH - textWidth(&t48->font, loadGameStrs[2])) >> 1,
                     TFT_HEIGHT - 96);
            int16_t xOff = 24; // To make it look centered, uneven
            int16_t yOff = TFT_HEIGHT - 64;
            drawTextWordWrap(&t48->font, c555, loadGameStrs[3], &xOff, &yOff, TFT_WIDTH - 16, TFT_HEIGHT);
            break;
        }
        case T48_WIN_SCREEN:
        {
            // Get inputs
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && (evt.button & PB_A || evt.button & PB_B))
                {
                    t48->state = T48_IN_GAME;
                }
            }
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
            drawText(&t48->titleFont, c055, youWin, (TFT_WIDTH - textWidth(&t48->titleFont, youWin)) >> 1, 48);
            drawText(&t48->font, c555, continueAB, (TFT_WIDTH - textWidth(&t48->font, continueAB)) >> 1,
                     TFT_HEIGHT - 64);
            break;
        }
        case T48_HS_SCREEN:
        {
            // Handle input in text entry object
            while (checkButtonQueueWrapper(&evt))
            {
                t48->textEntryDone = !textEntryInput(evt.down, evt.button);
            }

            // If the text entry is done, sort the scores, reset the text entry object,
            // and display the final screen
            if (t48->textEntryDone)
            {
                t48SortHighScores();
                textEntrySoftReset();
                t48->state = T48_END_SCREEN;
            }

            // Draw text entry
            textEntryDraw(elapsedUs);
            break;
        }
        case T48_END_SCREEN:
        {
            // Check any button is pressed
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && (evt.button & PB_A || evt.button & PB_B))
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48->state = T48_START_SCREEN;
                }
            }

            // Draw the final score screen
            t48_drawGameOverScreen(t48, t48->score, t48_generateRainbow());
            t48_chaseLEDs(elapsedUs);
            break;
        }
        default:
        {
            break;
        }
    }
}

/**
 * @brief Restarts the BGM once the track ends
 *
 */
static void t48BgmCb()
{
    t48->bgmIsPlaying = false;
}

/**
 * @brief Initializes the high scores based either from NVS or predetermined scores to beat
 *
 */
static void t48InitHighScores()
{
    // Init High scores
    for (int8_t i = 0; i < T48_HS_COUNT; i++)
    {
        if (!readNvs32(highScoreKey[i], &t48->highScore[i]))
        {
            switch (i)
            {
                case 0:
                    t48->highScore[i] = 96880;
                    break;
                case 1:
                    t48->highScore[i] = 69224;
                    break;
                case 2:
                    t48->highScore[i] = 24244;
                    break;
                case 3:
                    t48->highScore[i] = 11020;
                    break;
                case 4:
                    t48->highScore[i] = 5176;
                    break;
            }
            writeNvs32(highScoreKey[i], t48->highScore[i]);
        }
        size_t len = 4;
        if (!readNvsBlob(highScoreInitialsKey[i], &t48->hsInitials[i], &len))
        {
            static char buff[5];
            switch (i)
            {
                case 0:
                    strcpy(buff, "JW");
                    break;
                case 1:
                    strcpy(buff, "Pan");
                    break;
                case 2:
                    strcpy(buff, "Pix");
                    break;
                case 3:
                    strcpy(buff, "Poe");
                    break;
                case 4:
                    strcpy(buff, "DrG");
                    break;
            }
            strcpy(t48->hsInitials[i], buff);
            writeNvsBlob(highScoreInitialsKey[i], &t48->hsInitials[i], len);
        }
    }
}

/**
 * @brief Sorts the high scores and saves them to the NVM at the end of a game
 *
 */
static void t48SortHighScores()
{
    // 5th place needs to compare to the score
    if (t48->highScore[T48_HS_COUNT - 1] < t48->score)
    {
        t48->highScore[T48_HS_COUNT - 1] = t48->score;
        strcpy(t48->hsInitials[T48_HS_COUNT - 1], t48->playerInitials);
    }
    else
    {
        // Scores *should* be sorted already. Save cycles.
        return;
    }
    for (int8_t i = T48_HS_COUNT - 2; i >= 0; i--)
    {
        if (t48->highScore[i] < t48->highScore[i + 1])
        {
            // Swap
            int32_t swap          = t48->highScore[i];
            t48->highScore[i]     = t48->highScore[i + 1];
            t48->highScore[i + 1] = swap;
            char swapI[4];
            strcpy(swapI, t48->hsInitials[i]);
            strcpy(t48->hsInitials[i], t48->hsInitials[i + 1]);
            strcpy(t48->hsInitials[i + 1], swapI);
        }
    }
    // Save out the new scores
    for (int8_t i = 0; i < T48_HS_COUNT; i++)
    {
        writeNvs32(highScoreKey[i], t48->highScore[i]);
        writeNvsBlob(highScoreInitialsKey[i], &t48->hsInitials[i], 4);
    }
}

/**
 * @brief Generates a rainbow color in sequence
 *
 * @return paletteColor_t
 */
static paletteColor_t t48_generateRainbow()
{
    uint8_t hue = t48->hue++;
    uint8_t sat = 255;
    uint8_t val = 255;
    return paletteHsvToHex(hue, sat, val);
}

/**
 * @brief Dims the LEDs uniformly over time
 *
 * @param t48 Game data
 * @param elapsedUs Time since last frame
 */
static void t48_fadeLEDs(int32_t elapsedUs)
{
    t48->fadeTimer += elapsedUs;
    while (t48->fadeTimer >= T48_FADE_SPEED)
    {
        t48->fadeTimer -= T48_FADE_SPEED;
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            // Red
            if (t48->leds[i].r < 6)
            {
                t48->leds[i].r = 0;
            }
            else
            {
                t48->leds[i].r -= 6;
            }
            // Green
            if (t48->leds[i].g < 6)
            {
                t48->leds[i].g = 0;
            }
            else
            {
                t48->leds[i].g -= 6;
            }
            // Blue
            if (t48->leds[i].b < 6)
            {
                t48->leds[i].b = 0;
            }
            else
            {
                t48->leds[i].b -= 6;
            }
        }
    }
    setLeds(t48->leds, CONFIG_NUM_LEDS);
}

/**
 * @brief Chase LEDs around the swadge
 *
 * @param elapseUs Time since last update
 */
static void t48_chaseLEDs(int32_t elapseUs)
{
    t48->nextLedTimer += elapseUs;
    if (t48->nextLedTimer >= T48_NEXT_LED)
    {
        t48->nextLedTimer = 0;
        t48->currLED++;
        if (t48->currLED > CONFIG_NUM_LEDS - 1)
        {
            t48->currLED = 0;
        }
        t48->leds[t48->currLED] = t48_randColor();
    }
}

/**
 * @brief Generates a random LED color
 *
 * @return led_t Led object to set array to
 */
led_t t48_randColor()
{
    led_t col = {0};
    col.r     = 128 + (esp_random() % 127);
    col.g     = 128 + (esp_random() % 127);
    col.b     = 128 + (esp_random() % 127);
    return col;
}

/**
 * @brief Integrate the accelerometer in the background
 *
 * @param x Unused
 * @param y  Unused
 * @param w  Unused
 * @param h  Unused
 * @param up  Unused
 * @param upNum  Unused
 */
static void t48BackgroundDraw(int16_t x __attribute__((unused)), int16_t y __attribute__((unused)),
                              int16_t w __attribute__((unused)), int16_t h __attribute__((unused)),
                              int16_t up __attribute__((unused)), int16_t upNum __attribute__((unused)))
{
    // Allow IMU integration while in menu, to make it so when we set the zero, it
    // is at the time of the B press.
    if (t48->tiltControls || t48->state == T48_START_SCREEN)
    {
        accelIntegrate();
    }
}
