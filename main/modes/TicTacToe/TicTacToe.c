/**
 * @file TicTacToe.c
 * @author Jeremy Stintzcum (Jeremy.Stintzcum@gmail.com)
 * @brief A simple game of TicTacToe for the Super 2024 Swadge
 * @version 0.1.1
 * @date 2024-02-08
 *
 * @copyright Copyright (c) 2024
 *
 */

/*
 * VERSION PLANS:
 * 0.2.0: TTG Functions as provided by Bryce. 
 * 0.3.0: Visual cleanup pass,
 * changing X and O tokens to wsg files. Audio is added. 
 * 0.4.0: UI Cleanup, ("Are you sure you want to quit" dialogue)
 * 0.5.0: P2P multiplayer implementation
 */

/*
 * Main Tasks
 * - Add pngs for X's and O's, make graphic prettier
 * - Add pause between rounds where you can see the final result
 * - Add "Are you sure you want to quit?" menu
 * - Sound
 * - Make ttg logic
 * - Add p2p networking
 */

// ===== Includes =====

#include "ticTacToe.h"

// ===== Defines =====

#define V_SCREEN_SIZE         240
#define H_SCREEN_SIZE         280
#define BORDER                20
#define NOTIFICATION_DURATION 1500

// ===== Constants =====

const int16_t usableBoardSpaceV = V_SCREEN_SIZE - (2 * BORDER);
const int16_t boardStartH       = (H_SCREEN_SIZE / 2) - (usableBoardSpaceV / 2);

// ===== Enums =====

typedef enum
{
    MENU,         // Main Menu
    GAME1,        // Tic-Tac-Toe
    GAME2,        // Tic-Tac-Go - See 'ttg()' for description
    CONNECTING,   // Waiting for connection
    NOTIFICATION, // Displaying a notification
    SCORE,        // Displays Scoreboard
} modes_t;

typedef enum
{
    PLAYER1,
    PLAYER2,
} players_t;

typedef enum
{
    EMPTY,
    X,
    O,
} boardStates_t;

typedef enum
{
    THREE = 3,
    FOUR  = 4,
    FIVE  = 5,
} boardSizes_t;

// ===== Structs =====

typedef struct
{
    // Meta
    modes_t currMode;            // Which screen is active
    menu_t* menu;                // Main menu object
    menuLogbookRenderer_t* mlbr; // Main Menu renderer object
    int64_t notificationTimer;   // Timer for pop up notifications
    char* notificationMessage;   // Message for notification
    modes_t prevMode;            // Mode to return to after notification finishes
    bool showScore;              // If "start" button has been toggled
    int64_t ms;                  // Milliseconds in main loop

    // Resources
    font_t mainFont; // Default font for mode
    wsg_t xToken;    // X token image
    wsg_t oToken;    // O token image

    // Gameplay
    players_t currPlayer;              // Active player
    players_t devicePlayer;            // Player using device. For p2p.
    boardStates_t tokensAssignment[2]; // Which token is assigned to which player
    p2pInfo opponent;                  // Connection details of opponent
    int8_t score[3];                   // Scoreboard [P1, P2, draws]
    boardSizes_t size;                 // Size of board
    int16_t boxSize;                   // SIze of each box in pixels
    int8_t boardLen;                   // Absolute size of the board
    boardStates_t gameboard[25];       // Game board state array
    int8_t cursorPos;                  // Position of the cursor

} internal_state_t;

// ===== Function Protoypes =====

static void tttEnterMode(void);
static void tttExitMode(void);
static void tttMainLoop(int64_t);
static void tttMenuCB(const char*, bool, uint32_t);
static void tttSetupGameboardSize(char);
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
static void printScore(paletteColor_t, const char*, int32_t, int16_t, int16_t);

// ===== Strings =====

static const char tttName[] = "Tic-Tac-Toe";

// Menu options
static const char ttgName[]   = "Tic-Tac-GO!";
static const char p2p[]       = "Connect to another player";
static const char boardSize[] = "Size of game board";
static const char board3x3[]  = "3 x 3 game board";
static const char board4x4[]  = "4 x 4 game board";
static const char board5x5[]  = "5 x 5 game board";

// Notification
static const char notImplemented[] = "Mode not implemented, sorry.";
static const char error[]          = "Something went horribly wrong.";
// static const char p1notification[] = "You are player 1!";               // Notifications needed for p2p
// static const char p2notification[] = "You are player 2!";               // Notifications needed for p2p
static const char p1turn[] = "Player 1's turn!";
static const char p2turn[] = "Player 2's turn!";
static const char p1win[]  = "Player 1 wins!";
static const char p2win[]  = "Player 2 wins!";
static const char noWin[]  = "It's a draw!";

// Score
static const char Score[]     = "Scores";
static const char p1Score[]   = "Player 1: ";
static const char p2Score[]   = "Player 2: ";
static const char drawScore[] = "Draws: ";

// ===== Variables =====

swadgeMode_t ticTacMode = {.modeName                 = tttName,
                           .wifiMode                 = NO_WIFI,
                           .overrideUsb              = false,
                           .usesAccelerometer        = false,
                           .usesThermometer          = false,
                           .overrideSelectBtn        = false,
                           .fnEnterMode              = tttEnterMode,
                           .fnExitMode               = tttExitMode,
                           .fnMainLoop               = tttMainLoop,
                           .fnAudioCallback          = NULL,
                           .fnBackgroundDrawCallback = drawCB,
                           .fnEspNowRecvCb           = NULL,
                           .fnEspNowSendCb           = NULL,
                           .fnAdvancedUSB            = NULL};

internal_state_t* state = NULL;

// ===== Functions =====

// Main funcs
/**
 * @brief Initialization step for the TTT Mode
 * 
 */
static void tttEnterMode(void)
{
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
    tttSetupGameboardSize(THREE);
    state->cursorPos  = 4;
    state->currPlayer = PLAYER1;
    state->ms         = 0;
    state->currMode   = MENU;
}

/**
 * @brief Destructor for TTT Mode.
 * 
 */
static void tttExitMode(void)
{
    // Free assets
    freeFont(&state->mainFont);
    // freeWsg(&state->xToken);
    // freeWsg(&state->oToken);

    // Lastly, free state structure
    free(state);
}

/**
 * @brief Main loop of the TTT Mode. COntains logic to switch between games, settings, and connect to a peer.
 * 
 * @param elapsedUs Microseconds since last loop.
 */
static void tttMainLoop(int64_t elapsedUs)
{
    // Run whatever mode is active
    state->ms += elapsedUs / 1000;
    buttonEvt_t evt = {0};
    switch (state->currMode)
    {
        case MENU: // Main Menu
            while (checkButtonQueueWrapper(&evt))
            {
                state->menu = menuButton(state->menu, evt);
            }
            drawMenuLogbook(state->menu, state->mlbr, elapsedUs);
            break;

        case GAME1: // Tic-Tac-Toe
            ttt();
            break;

        case GAME2: // Tic-Tac-Go
            ttg();
            break;

        case CONNECTING: // P2P connection lobby
            connectToPeer();
            break;

        case NOTIFICATION: // Notifying players of something, don't process anything.
            displayNotification();
            if (state->notificationTimer < state->ms)
            {
                state->currMode = state->prevMode;
            }
            break;

        case SCORE: // Display score
            // Any input will hide scoreboard
            displayScore();
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
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

/**
 * @brief Main menu callback that describes button behavior.
 * 
 * @param label Label of menu item in focus
 * @param selected If the item was selected or not, used to ignore navigation inputs
 * @param settingVal Value of the setting being changed
 */
static void tttMenuCB(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == tttName)
        {
            tttSetup();
            state->currMode = GAME1;
        }
        else if (label == ttgName)
        {
            // TODO: Initialize
            state->currMode = GAME2;
        }
        else if (label == p2p)
        {
            state->currMode = CONNECTING;
        }
        else if (label == board3x3)
        {
            notify("Board size changed to 3x3", NOTIFICATION_DURATION);
            tttSetupGameboardSize(THREE);
        }
        else if (label == board4x4)
        {
            notify("Board size changed to 4x4", NOTIFICATION_DURATION);
            tttSetupGameboardSize(FOUR);
        }
        else if (label == board5x5)
        {
            notify("Board size changed to 5x5", NOTIFICATION_DURATION);
            tttSetupGameboardSize(FIVE);
        }
    }
}

/**
 * @brief Initializes the game board size when changed.
 * 
 * @param sz New size of the game board.
 */
static void tttSetupGameboardSize(char sz)
{
    state->size     = sz;
    state->boardLen = sz * sz;
    state->boxSize  = usableBoardSpaceV / state->size;
}

/**
 * @brief Causes a notification event, brining up a message and halting all other actions.
 * 
 * @param message The message content to print on screen.
 * @param duration The length of time in milliseconds for the message to remain on screen.
 */
static void notify(const char* message, int32_t duration)
{
    state->notificationMessage = message;
    state->notificationTimer   = duration;
    state->ms                  = 0;
    state->prevMode            = state->currMode;
    state->currMode            = NOTIFICATION;
}

// TicTacToe
/* Rules:
    - Grid starts fully empty, set to either 3x3, 4x4, or 5x5.
    - Each player must add one piece to an empty space on their turn
    - When three, four or five in a row is achieved, that player scores a win
    - If board is filled, the result is a draw
    - Once the game ends, a new game is automatically started.
*/

/**
 * @brief Main TicTacToe game. Contains the control logic, draw commands, and checks the board for a winner.
 * 
 */
static void ttt(void)
{
    // Start by checking if anyone has won.
    if (tttCheckForWin(PLAYER1))
    {
        state->score[0] += 1;
        notify(p1win, NOTIFICATION_DURATION);
        tttSetup();
    }
    else if (tttCheckForWin(PLAYER2))
    {
        state->score[1] += 1;
        notify(p2win, NOTIFICATION_DURATION);
        tttSetup();
    }
    else if (checkGameIsDraw())
    {
        state->score[2] += 1;
        notify(noWin, NOTIFICATION_DURATION);
        tttSetup();
    }
    else
    {
        // Game is still being played
        // TODO: Segregate when this can be done by active player when networking
        buttonEvt_t evt = {0};
        while (checkButtonQueueWrapper(&evt))
        {
            // Move cursor if local or active player.
            if (PB_RIGHT & evt.state)
            {
                state->cursorPos += state->size;
            }
            else if (PB_LEFT & evt.state)
            {
                state->cursorPos -= state->size;
            }
            else if (PB_UP & evt.state)
            {
                state->cursorPos -= 1;
                if (state->cursorPos % state->size == state->size - 1 || state->cursorPos < 0)
                {
                    state->cursorPos += state->size;
                }
            }
            else if (PB_DOWN & evt.state)
            {
                state->cursorPos += 1;
                if (state->cursorPos % state->size == 0)
                {
                    state->cursorPos -= state->size;
                }
            }
            // Loop if past edge of board.
            if (state->cursorPos >= state->boardLen)
            {
                state->cursorPos -= state->boardLen;
            }
            else if (state->cursorPos < 0)
            {
                state->cursorPos += state->boardLen;
            }
            // Add token to board at position if "A" is pressed.
            if (PB_A & evt.state)
            {
                addToken(state->currPlayer, state->cursorPos);
            }
            // If start button is pressed, return to menu
            if (PB_START & evt.state)
            {
                state->currMode = MENU; // FIXME: Abrupt, needs guard rails
            }
            // If B is pressed, show score
            if (PB_B & evt.state)
            {
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

/**
 * @brief Initializes the TicTacToe game by making the loser go first with X's and then clearing the game board.
 * 
 */
static void tttSetup(void)
{
    // Set other player to go first
    if (state->currPlayer == PLAYER1)
    {
        state->tokensAssignment[0] = X;
        state->tokensAssignment[1] = O;
    }
    else
    {
        state->tokensAssignment[0] = O;
        state->tokensAssignment[1] = X;
    }
    // Clear game board
    for (int i = 0; i < state->boardLen; i++)
    {
        state->gameboard[i] = EMPTY;
    }
}

/**
 * @brief Adds a token for the currently active player to the game board as long as the space is empty.
 * 
 * @param currPlayer Active player who selected the position to play
 * @param pos Position being played
 */
static void addToken(players_t currPlayer, int8_t pos)
{
    boardStates_t activeToken = EMPTY;
    if (state->currPlayer == PLAYER1)
    {
        activeToken = state->tokensAssignment[0];
    }
    else
    {
        activeToken = state->tokensAssignment[1];
    }
    if (state->gameboard[pos] == EMPTY)
    {
        state->gameboard[pos] = activeToken;
        if (state->currPlayer == PLAYER1)
        {
            state->currPlayer = PLAYER2;
        }
        else
        {
            state->currPlayer = PLAYER1;
        }
    }
}

/**
 * @brief Checks if the indicated player has won.
 * 
 * @param currPlayer Player that the funstion is checking against
 * @return true if the player has a win codition
 * @return false if the player does not have a win condition
 */
static bool tttCheckForWin(players_t currPlayer)
{
    // Get appropriate token for player
    boardStates_t token = state->tokensAssignment[currPlayer];
    bool result         = false;
    // Check for a win
    for (int8_t size = 0; size < state->size; size++)
    {
        result |= checkRow(token, size);
        result |= checkCol(token, size);
    }
    result |= checkDiag(token);
    return result;
}

/**
 * @brief Checks if he game has come to a draw
 * 
 * @return true if the game board is filled
 * @return false if there are empty spots left to play
 */
static bool checkGameIsDraw()
{
    for (int8_t len = 0; len < state->boardLen; len++)
    {
        if (state->gameboard[len] == EMPTY)
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Checks a specific row for a full row of the given token.
 * 
 * @param token The token to check
 * @param row The specific row to be checked
 * @return true is all tokens are the indicated type
 * @return false otherwise
 */
static bool checkRow(boardStates_t token, int8_t row)
{
    int8_t qty = 0;
    for (int8_t len = 0; len < state->size; len++)
    {
        if (state->gameboard[(row * state->size) + len] == token)
        {
            qty += 1;
        }
    }
    if (qty == state->size)
    {
        drawLineFromArray(c050, row * state->size, row * state->size + (state->size - 1));
        return true;
    }
    return false;
}

/**
 * @brief Checks a specific column for a full column of the given token.
 * 
 * @param token The token to check
 * @param col The specific column to be checked
 * @return true is all tokens are the indicated type
 * @return false otherwise
 */
static bool checkCol(boardStates_t token, int8_t col)
{
    int8_t qty = 0;
    for (int8_t len = 0; len < state->size; len++)
    {
        if (state->gameboard[(state->size * len) + col] == player)
        {
            qty += 1;
        }
    }
    if (qty == state->size)
    {
        drawLineFromArray(c030, col, (state->size * (state->size - 1)) + col);
        return true;
    }
    return false;
}

/**
 * @brief Checks the diagonals of the grid for a full line
 * 
 * @param token Token to check against
 * @return true if either diagonal contains only the indicated token
 * @return false otherwise
 */
static bool checkDiag(boardStates_t token)
{
    // Check top-left to bottom right
    int8_t qty = 0;
    for (int8_t len = 0; len < state->size; len++)
    {
        if (state->gameboard[(state->size * len) + len] == player)
        {
            qty += 1;
        }
    }
    if (qty == state->size)
    {
        drawLineFromArray(c030, 0, state->boardLen);
        return true;
    }
    // Check top-right to bottom left
    qty = 0;
    for (int8_t len = 0; len < state->size; len++)
    {
        if (state->gameboard[(state->size * len) + (state->size - 1 - len)] == player)
        {
            qty += 1;
        }
    }
    if (qty == state->size)
    {
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
static void ttg(void)
{
    state->currMode = MENU;
    notify(notImplemented, NOTIFICATION_DURATION);
}

// Connect to peer

static void connectToPeer(void)
{
    state->currMode = MENU;
    notify(notImplemented, NOTIFICATION_DURATION);
}

// Drawing functions

/**
 * @brief Background callback.
 * 
 * @param x X offset
 * @param y Y offset
 * @param w Width of the screen
 * @param h Height of the screen
 * @param up ?
 * @param upNum ? 
 */
static void drawCB(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    SETUP_FOR_TURBO();
    // Draw bg
    for (int16_t yp = y; yp < y + h; yp++)
    {
        for (int16_t xp = x; xp < x + w; xp++)
        {
            TURBO_SET_PIXEL(xp, yp, c000);
        }
    }
}

/**
 * @brief Draws the game board depending on the size set up.
 * 
 */
static void drawBoard(void)
{
    // Draw board based on size
    for (int i = 0; i < (state->size - 1); i++)
    {
        // Vertical lines
        drawLineFast(((i + 1) * state->boxSize) + boardStartH, BORDER, ((i + 1) * state->boxSize) + boardStartH,
                     V_SCREEN_SIZE - BORDER, c555);
        // Horizontal lines
        drawLineFast(boardStartH, ((i + 1) * state->boxSize) + BORDER, boardStartH + usableBoardSpaceV,
                     ((i + 1) * state->boxSize) + BORDER, c555);
    }
    // Draw active player string
    if (state->currPlayer == PLAYER1)
    {
        drawText(&state->mainFont, c555, p1turn, 40, 5);
    }
    else
    {
        drawText(&state->mainFont, c555, p2turn, 40, 5);
    }
}

/**
 * @brief Draws all tokens and the cursor
 * 
 */
static void drawTokens(void)
{
    // Draw tokens
    for (int8_t pos = 0; pos < state->boardLen; pos++)
    {
        int8_t xPos = pos / state->size;
        int8_t yPos = pos % state->size;
        if (state->gameboard[pos] == X)
        {
            drawXToken(xPos, yPos);
        }
        else if (state->gameboard[pos] == O)
        {
            drawOToken(xPos, yPos);
        }
    }
}

/**
 * @brief Draws the X tokens.
 * 
 * @param xPos Position in the visible grid (non-pixel) in the X direction
 * @param yPos Position in the visible grid (non-pixel) in the Y direction
 */
static void drawXToken(int16_t xPos, int16_t yPos)
{
    // Red X
    drawLineFast(xPos * state->boxSize + (boardStartH + 5), yPos * state->boxSize + (BORDER + 5),
                 xPos * state->boxSize + (boardStartH + state->boxSize - 5),
                 yPos * state->boxSize + (BORDER + state->boxSize - 5), c500);
    drawLineFast(xPos * state->boxSize + (boardStartH + state->boxSize - 5), yPos * state->boxSize + (BORDER + 5),
                 xPos * state->boxSize + (boardStartH + 5), yPos * state->boxSize + (BORDER + state->boxSize - 5),
                 c500);
}

/**
 * @brief Draws the O token
 * 
 * @param xPos Position in the visible grid (non-pixel) in the X direction
 * @param yPos Position in the visible grid (non-pixel) in the Y direction
 */
static void drawOToken(int16_t xPos, int16_t yPos)
{
    // Blue circle
    drawCircle(xPos * state->boxSize + boardStartH + state->boxSize / 2,
               yPos * state->boxSize + BORDER + state->boxSize / 2, (state->boxSize / 2) - 5, c005);
}

/**
 * @brief Draws the cursor given a position along the gameboard state.
 * 
 * @param pos Position insdie the gameboard
 */
static void drawCursor(int8_t pos)
{
    // Green Box
    int8_t xPos = pos / state->size;
    int8_t yPos = pos % state->size;
    drawRect(xPos * state->boxSize + (boardStartH + 5), yPos * state->boxSize + (BORDER + 5),
             xPos * state->boxSize + (boardStartH + state->boxSize - 5),
             yPos * state->boxSize + (BORDER + state->boxSize - 5), c050);
}

/**
 * @brief Draws a line given two indices in the gameboard array.
 * 
 * @param color Color of the line to draw
 * @param arrPosStart Start indice
 * @param arrPosEnd End indice
 */
static void drawLineFromArray(paletteColor_t color, int8_t arrPosStart, int8_t arrPosEnd)
{
    int8_t x1Pos = arrPosStart / state->size;
    int8_t y1Pos = arrPosStart % state->size;
    int8_t x2Pos = arrPosEnd / state->size;
    int8_t y2Pos = arrPosEnd % state->size;
    drawLineFast(x1Pos * state->boxSize + (boardStartH + (state->boxSize / 2)),
                 y1Pos * state->boxSize + (BORDER + state->boxSize / 2),
                 x2Pos * state->boxSize + (boardStartH + (state->boxSize / 2)),
                 y2Pos * state->boxSize + (BORDER + state->boxSize / 2), color);
}

/**
 * @brief Displays a single line of text and block out whatever's behind it for this frame.
 * 
 */
static void displayNotification(void)
{
    drawText(&state->mainFont, c555, state->notificationMessage, 35, 120);
}

/**
 * @brief Draws the scores to the screen.
 * 
 */
static void displayScore(void)
{
    drawText(&state->mainFont, c555, Score, 5, 25);
    printScore(c555, p1Score, state->score[0], 5, 45);
    printScore(c555, p2Score, state->score[1], 5, 65);
    printScore(c555, drawScore, state->score[2], 5, 85);
}

/**
 * @brief Prints a string and a single number after it. Useful for provding variables to the user, such as score info.
 * 
 * @param color Color of the text
 * @param text Text to be displayed
 * @param val Integer value to display (max int32_t)
 * @param xPos Position in the visible grid (non-pixel) in the X direction
 * @param yPos Position in the visible grid (non-pixel) in the Y direction
 */
static void printScore(paletteColor_t color, const char* text, int32_t val, int16_t xPos, int16_t yPos)
{
    char scoreStr[16];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%s%" PRIu32, text, val);
    drawText(&state->mainFont, color, scoreStr, xPos, yPos);
}
