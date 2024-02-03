/**
 * @file TicTacToe.c
 * @author Jeremy Stintzcum (Jeremy.Stintzcum@gmail.com)
 * @brief A simple game of TicTacToe for the Super 2024 Swadge
 * @version 0.1.0
 * @date 2024-02-02
 * 
 * @copyright Copyright (c) 2024
 * 
 */

/*
 * Main Tasks
 * - Add pngs for X's and O's
 * - Draw pngs in appropriate spots
 * - Make notification screen
 * - Make ttt logic
 * - Make ttg logic
 * - Add p2p networking
 * - Doxygen comments
 * - Figure out how to automatically style code
 */

// ===== Includes =====

#include "TicTacToe.h"

// ===== Defines =====

// ===== Enums =====

typedef enum {
    MENU,           // Main Menu
    GAME1,          // Tic-Tac-Toe
    GAME2,          // Tic-Tac-Go - See 'ttg()' for description
    CONNECTING,     // Waiting for connection
    NOTIFICATION,   // Displaying a notification
    SCORE,          // Displays Scoreboard
} modes_t;

typedef enum {
    PLAYER1,
    PLAYER2,
} players_t;

typedef enum {
    EMPTY,
    X,
    O,
} boardStates_t;

typedef enum {
    THREE = 3,
    FOUR = 4,
    FIVE = 5,
} boardSizes_t;

// ===== Structs =====

typedef struct {
    // Meta
    modes_t currMode;               // Which screen is active
    menu_t* menu;                   // Main menu object
    menuLogbookRenderer_t* mlbr;    // Main Menu renderer object
    int32_t notificationTimer;      // Timer for pop up notifications
    char* notificationMessage;      // Message for notification
    bool showScore;                 // If "start" button has been toggled

    // Resources
    font_t mainFont;                // Default font for mode
    wsg_t xToken;                   // X token image
    wsg_t oToken;                   // O token image

    // Gameplay
    players_t currPlayer;           // Active player
    players_t devicePlayer;         // Player using device. For p2p.
    p2pInfo opponent;               // Connection details of opponent
    int8_t score[3];                // Scoreboard [P1, P2, draws]
    boardSizes_t size;              // Size of board
    int8_t boardLen;                // Absolute size of the board
    boardStates_t gameboard[25];    // Game board state array

} internal_state_t;

// ===== Function Protoypes =====

static void tttEnterMode(void);
static void tttExitMode(void);
static void tttMainLoop(int64_t);
static void tttMenuCB(const char*, bool, uint32_t);
static void ttt(void);
static void ttg(void);
static void connectToPeer(void);
static void drawCB(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t);
static void drawTokens(void);
static void notify(const char*);
static void displayScore(void);

// ===== Strings =====

static const char tttName[] = "Tic-Tac-Toe";

// Menu options
static const char ttg[] = "Tic-Tac-GO!";
static const char p2p[] = "Connect to another player";
static const char boardSize[] = "Size of game board";
static const char board3x3[] = "3 x 3 game board";
static const char board4x4[] = "4 x 4 game board";
static const char board5x5[] = "5 x 5 game board";

// Notifications
static const char notImplemented[] = "Mode not implemented, sorry.";
static const char error[] = "Something went horribly wrong.";
static const char p1notification[] = "You are player 1!";               // Notifications needed for p2p
static const char p2notification[] = "You are player 2!";               // Notifications needed for p2p
static const char p1turn[] = "Player 1's turn!";
static const char p2turn[] = "Player 2's turn!";

// ===== Variables =====

swadgeMode_t ticTacMode = {
    .modeName = tttName,
    .wifiMode = NO_WIFI,
    .overrideUsb = false,
    .usesAccelerometer = false,
    .usesThermometer = false,
    .overrideSelectBtn = false,
    .fnEnterMode = tttEnterMode,
    .fnExitMode = tttExitMode,
    .fnMainLoop = tttMainLoop,
    .fnAudioCallback = NULL,
    .fnBackgroundDrawCallback = drawCB,
    .fnEspNowRecvCb = NULL,
    .fnEspNowSendCb = NULL,
    .fnAdvancedUSB = NULL
};

internal_state_t* state = NULL;

// ===== Functions =====

// Main funcs

static void tttEnterMode(void){
    // Initialize state struct
    state = calloc(1, sizeof(internal_state_t));
    
    // Initialize external resources
    loadFont("ibm_vga8.font", &state->mainFont, false);
    // loadWsg("xToken.wsg", &state->xToken, false);
    // loadWsg("oToken.wsg", &state->oToken, false);

    // Initialize the menu
    state->menu = initMenu(tttName, tttMenuCB);
    state->mlbr = initMenuLogbookRenderer(&state->mainFont);
    addSingleItemToMenu(state->menu, tttName);
    addSingleItemToMenu(state->menu, ttg);
    addSingleItemToMenu(state->menu, p2p);
    startSubMenu(state->menu, boardSize);
    addSingleItemToMenu(state->menu, board3x3);
    addSingleItemToMenu(state->menu, board4x4);
    addSingleItemToMenu(state->menu, board5x5);
    endSubMenu(state->menu);
    
    // Initialize state
    state->size = THREE;
    state->boardLen = 9;
    state->currMode = MENU;
}

static void tttExitMode(void){
    // Free assets
    freeFont(&state->mainFont);
    //freeWsg(&state->xToken);
    //freeWsg(&state->oToken);

    // Lastly, free state structure
    free(state);
}

static void tttMainLoop(int64_t elapsedUs){
    // Timers
    if (state->notificationTimer > 0){
        state->currMode = NOTIFICATION;
        state->notificationTimer = state->notificationTimer - elapsedUs;
    }
    // Run whatever mode is active
    switch (state->currMode)
    {
    case MENU:          // Main Menu
        buttonEvt_t evt = {0};
        while (checkButtonQueueWrapper(&evt)) {
            state->menu = menuButton(state->menu, evt);
        }
        drawMenuLogbook(state->menu, state->mlbr, elapsedUs);
        break;
    
    case GAME1:         // Basic Tic-Tac-Toe
        ttt();
        break;
    
    case GAME2:         // Tic-Tac-Go
        ttg();
        break;

    case CONNECTING:    // P2P connection lobby
        connectToPeer();
        break;

    case NOTIFICATION:  // Notifying players of something
        notify(state->notificationMessage);
        break;

    case SCORE:         // Display score

    default:
        state->notificationMessage = error;
        state->notificationTimer = 3000;
        break;
    }
}

// Menu funcs

static void tttMenuCB(const char* label, bool selected, uint32_t settingVal){
    if (selected){
        if (label == tttName){
            state->currMode = GAME1;
            state->currPlayer = PLAYER1; //TODO: Make random
        } else if (label == ttg){
            state->currMode = GAME2;
            state->currPlayer = PLAYER1; //TODO: Make random
        } else if (label == p2p){
            state->currMode = CONNECTING;
        } else if (label == boardSize){
            // Do nothing? Should let you pick board sizes
        } else if (label == board3x3){
            state->size = THREE;
            state->boardLen = THREE * THREE;
        } else if (label == board4x4){
            state->size = FOUR;
            state->boardLen = FOUR * FOUR;
        } else if (label == board5x5){
            state->size = FIVE;
            state->boardLen = FIVE * FIVE;
        }
    }
}

// TicTacToe
/* Rules:
    - 3x3 Grid starts fully empty
    - Each player must add one piece to an empty space on their turn
    - When three in a row is acheived, that player scores a win
    - If board is filled, the result is a draw
*/
static void ttt(void){
    if (state->notificationTimer > 0){
        notify(state->notificationMessage);
        return;
    }
    // Game logic

    // Draw
    drawTokens();
}

// TicTacGo
/* Rules:
    - 3x3 Grid
    - Both players start with three of their pieces (X's or O's) on the 'home row' closest to them.
    - Home row is exclusive to player
    - Each player must move one piece each turn
    - A piece may be moved to any adjacent, unoccupied space vertically, horizontally, or diagonally
    - When three in a row is acheived, that player scores a win
    - Home row is excluded from the valid options for each player
*/
static void ttg(void){
    if (state->notificationTimer > 0){
        notify(state->notificationMessage);
        return;
    }
    // Game logic
    
    // Draw
    drawTokens();
}

// Connect to peer

static void connectToPeer(void){
    //TODO: Add p2p connection waiting room and dialogue
}

// Drawing functions

static void drawCB(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum){
    SETUP_FOR_TURBO();
    // Precompute
    int16_t one_third_h = h / 3;
    int16_t one_third_w = w / 3;
    // Draw Tic-Tac-Toe grid
    for (int16_t yp = y; yp < y + h; yp++)
    {
        for (int16_t xp = x; xp < x + w; xp++)
        {
            if ((xp == one_third_w || xp == 2 * one_third_w) || (yp == one_third_h || yp == 2 * one_third_h))
            {
                TURBO_SET_PIXEL(xp, yp, c333);
            }
            else
            {
                TURBO_SET_PIXEL(xp, yp, c111);
            }
        }
    }
}

// Currently assuming top corner 
static void drawTokens(){
    for (int8_t pos = 0; pos < state->boardLen; pos++){
        int8_t xPos = pos / state->size;
        int8_t yPos = pos % state->size;
        //TODO: offset and draw correct image
    }
}

static void notify(const char* message){
    //TODO: Add notifications screen
}

static void displayScore(void){
    //TODO: Add score screen
}