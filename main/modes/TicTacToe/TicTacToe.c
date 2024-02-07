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
 * VERSION PLANS:
 * 0.1.0: TTT functions as per the classic game of TTT. Score is properly tracked betweeen players. All text indicates the correct players. All buttons shoudl work.
 * 0.2.0: TTG Functions as provided by Bryce.
 * 0.3.0: Visual cleanup pass, changing X and O tokens to wsg files. Audio is added.
 * 0.4.0: UI Cleanup, ("Are you sure you want to quit" dialogue)
 * 0.5.0: P2P multiplayer implementation
 */

/*
 * Main Tasks
 * - Add pngs for X's and O's / Draw in place
 * - Scale grid to state->size
 * - include buffer for some text at the top
 * - Draw tokens to grid
 * - Draw a cursor
 * - Make notification screen
 * - Make score screen
 * - Doxygen comments
 * - Figure out how to automatically style code
 * - Fix case and style of vars, funcs, etc
 * - Add ttt to swadge menu
 * - Sound
 * - Make ttg logic
 * - Add p2p networking
 */

// ===== Includes =====

#include "TicTacToe.h"

// ===== Defines =====

#define V_SCREEN_SIZE 240
#define H_SCREEN_SIZE 320
#define BORDER 20
#define NOTIFICATION_DURATION 3000000

// ===== Constants =====

const int16_t usableBoardSpaceV = V_SCREEN_SIZE - (2 * BORDER);
const int16_t boardStartH = (H_SCREEN_SIZE / 2) - (usableBoardSpaceV / 2); //FIXME: Assumes H pixels are greater than V pixels. Safe assumption, but an assumption.

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
    modes_t prevMode;               // Mode to return to after notification finishes
    bool showScore;                 // If "start" button has been toggled

    // Resources
    font_t mainFont;                // Default font for mode
    wsg_t xToken;                   // X token image
    wsg_t oToken;                   // O token image

    // Gameplay
    players_t currPlayer;           // Active player
    players_t devicePlayer;         // Player using device. For p2p.
    boardStates_t tokensAssignment[2];  //Which token is assigned too which player 
    p2pInfo opponent;               // Connection details of opponent
    int8_t score[3];                // Scoreboard [P1, P2, draws]
    boardSizes_t size;              // Size of board
    int16_t boxSize;                // SIze of each box in pixels
    int8_t boardLen;                // Absolute size of the board
    boardStates_t gameboard[25];    // Game board state array
    int8_t cursorPos;               // Position of the cursor

} internal_state_t;

// ===== Function Protoypes =====

static void tttEnterMode(void);
static void tttExitMode(void);
static void tttMainLoop(int64_t);
static void tttMenuCB(const char*, bool, uint32_t);
static void setupGameboardSize(char);
static void notify(const char*, int32_t);
static void ttt(void);
static void tttSetup(void);
static void addToken(players_t, int8_t);
static bool tttCheckForWin(players_t);
static bool checkGameIsDraw();
static bool checkRow(boardStates_t, int8_t);
static bool checkCol(boardStates_t, int8_t);
static bool checkDiag(boardStates_t);
static void ttg(void);
static void connectToPeer(void);
static void drawCB(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t);
static void drawBoard(void);
static void drawTokens(void);
static void drawXToken(int16_t, int16_t);
static void drawOToken(int16_t, int16_t);
static void drawCursor(int8_t);
static void drawLineFromArray(paletteColor_t, int8_t, int8_t);
static void displayNotification(void);
static void displayScore(void);

// ===== Strings =====

static const char tttName[] = "Tic-Tac-Toe";

// Menu options
static const char ttgName[] = "Tic-Tac-GO!";
static const char p2p[] = "Connect to another player";
static const char boardSize[] = "Size of game board";
static const char board3x3[] = "3 x 3 game board";
static const char board4x4[] = "4 x 4 game board";
static const char board5x5[] = "5 x 5 game board";

// Notification
static const char notImplemented[] = "Mode not implemented, sorry.";
static const char error[] = "Something went horribly wrong.";
static const char p1notification[] = "You are player 1!";               // Notifications needed for p2p
static const char p2notification[] = "You are player 2!";               // Notifications needed for p2p
static const char p1turn[] = "Player 1's turn!";
static const char p2turn[] = "Player 2's turn!";

// Score
static const char Score[] = "Scores";
static const char p1Score[] = "Player 1: ";
static const char p2Score[] = "Player 2: ";
static const char drawScore[] = "Draws: ";

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
    addSingleItemToMenu(state->menu, ttgName);
    addSingleItemToMenu(state->menu, p2p);
    state->menu = startSubMenu(state->menu, boardSize);
    addSingleItemToMenu(state->menu, board3x3);
    addSingleItemToMenu(state->menu, board4x4);
    addSingleItemToMenu(state->menu, board5x5);
    state->menu = endSubMenu(state->menu);
    
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
    // Run whatever mode is active
    buttonEvt_t evt = {0};
    switch (state->currMode)
    {
    case MENU:          // Main Menu
        while (checkButtonQueueWrapper(&evt)) {
            state->menu = menuButton(state->menu, evt);
        }
        drawMenuLogbook(state->menu, state->mlbr, elapsedUs);
        break;
    
    case GAME1:         // Tic-Tac-Toe
        ttt();
        break;
    
    case GAME2:         // Tic-Tac-Go
        ttg();
        break;

    case CONNECTING:    // P2P connection lobby
        connectToPeer();
        break;

    case NOTIFICATION:  // Notifying players of something, don't process anything.
        displayNotification();
        if (state->notificationTimer >= 0){
            state->currMode = state->prevMode;
        } else {
            state->notificationTimer -= elapsedUs;
        }
        break;

    case SCORE:         // Display score
        // Any input will hide scoreboard
        displayScore();
        while (checkButtonQueueWrapper(&evt)) {
            if (evt.down){
                state->currMode = state->prevMode;
            }
        }
        break;

    default:
        notify(error, NOTIFICATION_DURATION);
        break;
    }
}

// Menu funcs

static void tttMenuCB(const char* label, bool selected, uint32_t settingVal){
    if (selected){
        if (label == tttName){
            tttSetup();
            state->currMode = GAME1;
        } else if (label == ttgName){
            // TODO: Initialize
            state->currMode = GAME2;
        } else if (label == p2p){
            state->currMode = CONNECTING;
        } else if (label == boardSize){
            // Do nothing? Should let you pick board sizes
        } else if (label == board3x3){
            setupGameboardSize(THREE);
        } else if (label == board4x4){
            setupGameboardSize(FOUR);
        } else if (label == board5x5){
            setupGameboardSize(FIVE);
        }
    }
}

static void setupGameboardSize(char sz){
    state->size = sz;
    state->boardLen = sz * sz;
    state->boxSize = usableBoardSpaceV / state->size;
}

static void notify(const char* message, int32_t duration){

    state->notificationMessage = message;
    state->notificationTimer = duration;
    state->prevMode = state->currMode;
    state->currMode = NOTIFICATION;
}

// TicTacToe
/* Rules:
    - 3x3 Grid starts fully empty
    - Each player must add one piece to an empty space on their turn
    - When three in a row is acheived, that player scores a win
    - If board is filled, the result is a draw
*/
static void ttt(void){
    // Start by checking if anyone has won.
    bool player1Result = tttCheckForWin(PLAYER1);
    bool player2Result = tttCheckForWin(PLAYER2);
    bool isADraw = checkGameIsDraw();
    if(player1Result || player2Result || isADraw){
        state->currMode = SCORE;
        state->prevMode = GAME1;
    } else {
        // Game is still being played
        // TODO: Segregate when this can be done by active player when networking
        buttonEvt_t evt = {0};
        while (checkButtonQueueWrapper(&evt)){
            // Move cursor if local or active player. 
            if (PB_RIGHT & evt.state){
                state->cursorPos += 1;
            } else if (PB_LEFT & evt.state){
                state->cursorPos -= 1;
            } else if (PB_UP & evt.state){
                state->cursorPos -= state->size;
            } else if (PB_DOWN & evt.state){
                state->cursorPos += state->size;
            }
            // Loop if past edge of board.
            if (state->cursorPos >= state->boardLen){
                state->cursorPos -= state->boardLen;
            } else if (state->cursorPos < 0){
                state->cursorPos += state->boardLen;
            }
            // Add token to board at position if "A" is pressed.
            if (PB_A & evt.state){
                addToken(state->currPlayer, state->cursorPos);
            }
            // If start button is pressed, return to menu
            if (PB_START & evt.state){
                state->currMode = MENU; // FIXME: Abrupt, needs guard rails
            }
            // If B is pressed, show score
            if (PB_B & evt.state){
                state->prevMode = GAME1;
                state->currMode = SCORE;
            }
        }
    }
    // Draw
    drawBoard();
    drawTokens();
    drawCursor(state->cursorPos);
}

static void tttSetup(void){
    // Set other player to go first
    if (state->currPlayer == PLAYER2){
        state->currPlayer = PLAYER1; 
    } else {
        state->currPlayer = PLAYER2;
    }
    // Clear game board
    for (int i = 0; i < state->boardLen; i++){
        state->gameboard[i] = EMPTY;
    }
}

static void addToken(players_t currPlayer, int8_t pos){}

static bool tttCheckForWin(players_t currPlayer){
    // Get appropriate token for player
    boardStates_t token = state->tokensAssignment[currPlayer];
    bool result = false;
    // Check for a win
    for (int8_t size = 0; size < state->size; size++){
        result |= checkRow(token, size);
        result |= checkCol(token, size);
    }
    result |= checkDiag(token);
    return result;
}

static bool checkGameIsDraw(){
    for (int8_t len = 0; len < state->boardLen; len++){
        if (state->gameboard[len] == EMPTY){
            return false;
        }
    }
    return true;
}

static bool checkRow(boardStates_t player, int8_t row){
    int8_t qty = 0;
    for (int8_t len = 0; len < state->size; len++){
        if (state->gameboard[(row * state->size) + len] == player){
            qty += 1;
        }
    }
    if (qty == state->size){
        drawLineFromArray(c030, row * state->size, row * state->size + (state->size - 1));
        return true;
    }
    return false;
}

static bool checkCol(boardStates_t player, int8_t col){
    int8_t qty = 0;
    for (int8_t len = 0; len < state->size; len++){
        if (state->gameboard[(state->size * len) + col] == player){
            qty += 1;
        }
    }
    if (qty == state->size){
        drawLineFromArray(c030, col, (state->size * (state->size - 1)) + col);
        return true;
    }
    return false;
}

static bool checkDiag(boardStates_t player){
    // Check top-left to bottom right
    int8_t qty = 0;
    for (int8_t len = 0; len < state->size; len++){
        if (state->gameboard[(state->size * len) + len] == player){
            qty += 1;
        }
    }
    if (qty == state->size){
        drawLineFromArray(c030, 0, state->boardLen);
        return true;
    }
    // Check top-right to bottom left
    qty = 0;
    for (int8_t len = 0; len < state->size; len++){
        if (state->gameboard[(state->size * len) + (state->size - 1 - len)] == player){
            qty += 1;
        }
    }
    if (qty == state->size){
        drawLineFromArray(c030, state->size - 1, state->size * (state->size - 1));
        return true;
    }
    return false;
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
    notify(notImplemented, NOTIFICATION_DURATION);
}

// Connect to peer

static void connectToPeer(void){
    //TODO: Add p2p connection waiting room and dialogue
    notify(notImplemented, NOTIFICATION_DURATION);
}

// Drawing functions

static void drawCB(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum){
    SETUP_FOR_TURBO();
    // Draw bg
    for (int16_t yp = y; yp < y + h; yp++)
    {
        for (int16_t xp = x; xp < x + w; xp++)
        {
            TURBO_SET_PIXEL(xp, yp, c001);
        }
    }
}

static void drawBoard(void){
    // Draw board based on size
    for (int i = 0; i < (state->size - 1); i++){
        // Vertical lines
        drawLineFast((i * state->boxSize) + BORDER, BORDER, (i * state->boxSize) + BORDER, V_SCREEN_SIZE - BORDER, c555);
        // Horizontal lines
        drawLineFast(boardStartH, (i * state->boxSize) + BORDER, boardStartH + usableBoardSpaceV, (i * state->boxSize) + BORDER, c555);
    }
    // Draw active player string
    if (state->currPlayer == PLAYER1){
        drawText(&state->mainFont, c555, p1turn, 5, 5);
    } else {
        drawText(&state->mainFont, c555, p2turn, 5, 5);
    }
    
}

static void drawTokens(void){
    // Draw tokens
    for (int8_t pos = 0; pos < state->boardLen; pos++){
        int8_t xPos = pos / state->size;
        int8_t yPos = pos % state->size;
        if (state->gameboard[pos] == X){
            drawXToken(xPos, yPos);
            drawOToken(xPos, yPos);
            drawCursor(pos);
        }
    }
}

static void drawXToken(int16_t xPos, int16_t yPos){
    // Red X
}

static void drawOToken(int16_t xPos, int16_t yPos){
    // Blue circle
}

static void drawCursor(int8_t arrPos){
    // Green Box
}

static void drawLineFromArray(paletteColor_t color, int8_t arrPosStart, int8_t arrPosEnd){

}

static void displayNotification(void){
    //TODO: Add notifications screen
}

static void displayScore(void){
    drawText(&state->mainFont, c555, Score, 5, 5);
    drawText(&state->mainFont, c555, p1Score, 5, 25);
    drawText(&state->mainFont, c555, p2Score, 5, 45);
    drawText(&state->mainFont, c555, drawScore, 5, 65);
    drawText(&state->mainFont, c555, p1notification, 5, 85);
    drawText(&state->mainFont, c555, p2notification, 5, 1055);
}