//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>
#include "ultimateTTThowTo.h"
#include "ultimateTTTgame.h"

//==============================================================================
// Defines
//==============================================================================

#define TEXT_MARGIN 18

static void instructionSetupBoard(ultimateTTT_t* ttt);
static void instructionSetupSmallWin(ultimateTTT_t* ttt);
static void instructionSetupInvalidMove(ultimateTTT_t* ttt);
static void instructionSetupGameWin(ultimateTTT_t* ttt);

//==============================================================================
// Variables
//==============================================================================

typedef enum
{
    INSTRUCTION_TEXT,
    INSTRUCTION_FUNC,
    INSTRUCTION_BUTTON,
    INSTRUCTION_BUTTON_SEL_GAME,
    INSTRUCTION_NOP,
} instructionType_t;

typedef struct
{
    instructionType_t type;
    union
    {
        const char* text;
        void (*instructionFunc)(ultimateTTT_t* ttt);
        buttonBit_t button;
    };
    struct
    {
        vec_t base;
        vec_t tip;
    } iArrow;
} instructionPage_t;

static const instructionPage_t howToPages[] = {
    {
        .type = INSTRUCTION_TEXT,
        .text = "Ultimate TTT is a big game of tic-tac-toe made up of nine small games of tic-tac-toe.",
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_FUNC,
        .instructionFunc = instructionSetupBoard,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_TEXT,
        .text = "Players take turns placing markers in the small games of tic-tac-toe.",
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_TEXT,
        .text = "The starting player may place a marker anywhere.",
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_NOP,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_DOWN,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_RIGHT,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_RIGHT,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_A,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_UP,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_TEXT,
        .text = "The square a marker is placed in determines the next small game a player must play in.",
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_TEXT,
        .text = "A marker placed in a top-middle square means the next marker must be placed in the top-middle game.",
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_NOP,
        .iArrow = {
            .base = {
                .x = 7,
                .y = 3,
            },
            .tip = {
                .x = 4,
                .y = 1
            }
        },
    },
    {
        .type = INSTRUCTION_BUTTON_SEL_GAME,
        .button = PB_A,
        .iArrow = {
            .base = {
                .x = 7,
                .y = 3,
            },
            .tip = {
                .x = 4,
                .y = 1
            }
        },
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_A,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_LEFT,
        .iArrow = {
            .base = {
                .x = 3,
                .y = 1,
            },
            .tip = {
                .x = 1,
                .y = 4
            }
        },
    },
    {
        .type = INSTRUCTION_BUTTON_SEL_GAME,
        .button = PB_A,
        .iArrow = {
            .base = {
                .x = 3,
                .y = 1,
            },
            .tip = {
                .x = 1,
                .y = 4
            }
        },
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_A,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_DOWN,
        .iArrow = {
            .base = {
                .x = 1,
                .y = 5,
            },
            .tip = {
                .x = 4,
                .y = 7
            }
        },
    },
    {
        .type = INSTRUCTION_BUTTON_SEL_GAME,
        .button = PB_A,
        .iArrow = {
            .base = {
                .x = 1,
                .y = 5,
            },
            .tip = {
                .x = 4,
                .y = 7
            }
        },
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_A,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_TEXT,
        .text = "When a small game is won, markers may not be placed there anymore.",
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_FUNC,
        .instructionFunc = instructionSetupSmallWin,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_RIGHT,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_RIGHT,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_A,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_TEXT,
        .text = "If a marker cannot be placed in a small game because the game is won, it may be placed anywhere.",
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_FUNC,
        .instructionFunc = instructionSetupInvalidMove,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_DOWN,
        .iArrow = {
            .base = {
                .x = 1,
                .y = 7,
            },
            .tip = {
                .x = 4,
                .y = 4
            }
        },
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_A,
        .iArrow = {
            .base = {
                .x = 1,
                .y = 7,
            },
            .tip = {
                .x = 1,
                .y = 1
            }
        },
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_RIGHT,
        .iArrow = {
            .base = {
                .x = 1,
                .y = 7,
            },
            .tip = {
                .x = 4,
                .y = 1
            }
        },
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_A,
        .iArrow = {
            .base = {
                .x = 1,
                .y = 7,
            },
            .tip = {
                .x = 4,
                .y = 1
            }
        },
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_DOWN,
        .iArrow = {
            .base = {
                .x = 1,
                .y = 7,
            },
            .tip = {
                .x = 4,
                .y = 2
            }
        },
    },
    {
        .type = INSTRUCTION_BUTTON_SEL_GAME,
        .button = PB_A,
        .iArrow = {
            .base = {
                .x = 4,
                .y = 2,
            },
            .tip = {
                .x = 4,
                .y = 7
            }
        },
    },
    {
        .type = INSTRUCTION_TEXT,
        .text = "The goal is to win three games of tic-tac-toe in a row.",
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_FUNC,
        .instructionFunc = instructionSetupGameWin,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_RIGHT,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_RIGHT,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_A,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_RIGHT,
        .iArrow = {},
    },
    {
        .type = INSTRUCTION_BUTTON,
        .button = PB_A,
        .iArrow = {
            .base = {
                .x = 9,
                .y = -1,
            },
            .tip = {
                .x = -1,
                .y = 9
            }
        },
    },
    {
        .type = INSTRUCTION_TEXT,
        .text = "Win more games to unlock more markers!",
        .iArrow = {},
    },
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Handle a button input when How To is being shown
 *
 * @param ttt The entire game state
 * @param evt The button event
 */
void tttInputHowTo(ultimateTTT_t* ttt, buttonEvt_t* evt)
{
    // If the button was pressed
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_B:
            case PB_LEFT:
            {
                // These buttons scroll back
                if (ttt->pageIdx > 0)
                {
                    // Undo the current operation
                    switch (howToPages[ttt->pageIdx].type)
                    {
                        case INSTRUCTION_BUTTON:
                        case INSTRUCTION_BUTTON_SEL_GAME:
                        case INSTRUCTION_FUNC:
                        {
                            tttGameData_t* priorState = pop(&ttt->instructionHistory);
                            memcpy(&ttt->game, priorState, sizeof(tttGameData_t));
                            free(priorState);
                        }
                        case INSTRUCTION_NOP:
                        case INSTRUCTION_TEXT:
                        default:
                        {
                            // Don't undo anything
                            break;
                        }
                    }

                    // Decrement the page
                    ttt->pageIdx--;

                    // Set up the new page's arrow
                    vec_t cellOffset = {
                        .x = ttt->cellSize / 2 - 1,
                        .y = ttt->cellSize / 2 - 1,
                    };
                    ttt->instructionArrow = initArrow(
                        addVec2d(cellOffset, addVec2d(ttt->gameOffset,
                                                      mulVec2d(howToPages[ttt->pageIdx].iArrow.base, ttt->cellSize))),
                        addVec2d(cellOffset, addVec2d(ttt->gameOffset,
                                                      mulVec2d(howToPages[ttt->pageIdx].iArrow.tip, ttt->cellSize))),
                        16);
                }
                else if (ttt->tutorialRead && evt->button == PB_B)
                {
                    // Return to main menu if going back from page 0, only if the rules have been read
                    tttShowUi(TUI_MENU);
                }
                break;
            }
            case PB_A:
            case PB_RIGHT:
            {
                // These buttons scroll forward
                if ((ARRAY_SIZE(howToPages) - 1) != ttt->pageIdx)
                {
                    ttt->pageIdx++;

                    switch (howToPages[ttt->pageIdx].type)
                    {
                        case INSTRUCTION_BUTTON:
                        case INSTRUCTION_BUTTON_SEL_GAME:
                        case INSTRUCTION_FUNC:
                        {
                            // Push history onto the list
                            tttGameData_t* oldData
                                = (tttGameData_t*)heap_caps_calloc(1, sizeof(tttGameData_t), MALLOC_CAP_SPIRAM);
                            memcpy(oldData, &ttt->game, sizeof(tttGameData_t));
                            push(&ttt->instructionHistory, oldData);
                            break;
                        }
                        default:
                        case INSTRUCTION_NOP:
                        case INSTRUCTION_TEXT:
                        {
                            break;
                        }
                    }

                    // Set up the arrow
                    vec_t cellOffset = {
                        .x = ttt->cellSize / 2 - 1,
                        .y = ttt->cellSize / 2 - 1,
                    };
                    ttt->instructionArrow = initArrow(
                        addVec2d(cellOffset, addVec2d(ttt->gameOffset,
                                                      mulVec2d(howToPages[ttt->pageIdx].iArrow.base, ttt->cellSize))),
                        addVec2d(cellOffset, addVec2d(ttt->gameOffset,
                                                      mulVec2d(howToPages[ttt->pageIdx].iArrow.tip, ttt->cellSize))),
                        16);

                    // If the new page is a render
                    switch (howToPages[ttt->pageIdx].type)
                    {
                        case INSTRUCTION_BUTTON:
                        case INSTRUCTION_BUTTON_SEL_GAME:
                        {
                            // Process the button input
                            buttonEvt_t simEvt = {
                                .button = howToPages[ttt->pageIdx].button,
                                .down   = true,
                                .state  = howToPages[ttt->pageIdx].button,
                            };
                            tttHandleGameInput(ttt, &simEvt);

                            // If the game should be selected instead of the cell, do that
                            if (INSTRUCTION_BUTTON_SEL_GAME == howToPages[ttt->pageIdx].type)
                            {
                                ttt->game.cursor     = ttt->game.selectedSubgame;
                                ttt->game.cursorMode = SELECT_SUBGAME;
                            }

                            // If the player is now waiting
                            if (TGS_WAITING == ttt->game.state)
                            {
                                // Change the active player
                                if (GOING_FIRST != ttt->game.p2p.cnc.playOrder)
                                {
                                    ttt->game.p2p.cnc.playOrder = GOING_FIRST;
                                }
                                else
                                {
                                    ttt->game.p2p.cnc.playOrder = GOING_SECOND;
                                }
                                // Set the player to place a marker
                                ttt->game.state = TGS_PLACING_MARKER;
                            }
                            break;
                        }
                        case INSTRUCTION_FUNC:
                        {
                            // Execute the function
                            howToPages[ttt->pageIdx].instructionFunc(ttt);
                            break;
                        }
                        default:
                        case INSTRUCTION_NOP:
                        case INSTRUCTION_TEXT:
                        {
                            break;
                        }
                    }
                }
                else
                {
                    // Mark the tutorial as completed
                    if (false == ttt->tutorialRead)
                    {
                        ttt->tutorialRead = true;
                        writeNvs32(tttTutorialKey, ttt->tutorialRead);
                    }
                    // If a marker hasn't been selected
                    if (-1 == ttt->activeMarkerIdx)
                    {
                        // Select the marker after reading the tutorial
                        tttShowUi(TUI_MARKER_SELECT);
                    }
                    else
                    {
                        // Return to main menu if going forward from the last page
                        tttShowUi(TUI_MENU);
                    }
                }
                break;
            }
            default:
            {
                // Unused buttons
                break;
            }
        }
    }
}

/**
 * @brief Draw the How To UI
 *
 * @param ttt The entire game state
 * @param elapsedUs The time elapsed since this was last called
 */
void tttDrawHowTo(ultimateTTT_t* ttt, int64_t elapsedUs)
{
    // Always set LEDs off
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);

    paletteColor_t textColor = c000;
    switch (howToPages[ttt->pageIdx].type)
    {
        case INSTRUCTION_FUNC:
        case INSTRUCTION_BUTTON:
        case INSTRUCTION_BUTTON_SEL_GAME:
        case INSTRUCTION_NOP:
        {
            // Render the game board
            tttDrawGame(ttt);
            textColor = c555;
            break;
        }
        case INSTRUCTION_TEXT:
        {
            // Draw the background
            drawMenuMania(ttt->bgMenu, ttt->menuRenderer, 0);

            // Draw the text here
            int16_t xOff = TEXT_MARGIN;
            int16_t yOff = MANIA_TITLE_HEIGHT + 8;
            // Draw the text and save the next page
            drawTextWordWrap(&ttt->font_rodin, c000, howToPages[ttt->pageIdx].text, &xOff, &yOff,
                             TFT_WIDTH - TEXT_MARGIN, TFT_HEIGHT);
            break;
        }
    }

    char pageText[32];
    snprintf(pageText, sizeof(pageText) - 1, "%" PRId32 "/%" PRId32 "", 1 + ttt->pageIdx,
             (int32_t)ARRAY_SIZE(howToPages));

    int16_t tWidth = textWidth(&ttt->font_rodin, pageText);
    drawText(&ttt->font_rodin, textColor, pageText, TFT_WIDTH - 30 - tWidth, TFT_HEIGHT - ttt->font_rodin.height);

    // Blink the arrows
    ttt->arrowBlinkTimer += elapsedUs;
    while (ttt->arrowBlinkTimer >= ARROW_BLINK_PERIOD)
    {
        ttt->arrowBlinkTimer -= ARROW_BLINK_PERIOD;
    }

    if (ttt->arrowBlinkTimer < (ARROW_BLINK_PERIOD / 2))
    {
        // Draw arrows to indicate this can be scrolled
        if (0 != ttt->pageIdx)
        {
            // Draw left arrow if not on the first page
            drawText(&ttt->font_rodin, textColor, "<", 0, (TFT_HEIGHT - ttt->font_rodin.height) / 2);
        }

        if ((ARRAY_SIZE(howToPages) - 1) != ttt->pageIdx)
        {
            // Draw right arrow if not on the last page
            drawText(&ttt->font_rodin, textColor, ">", TFT_WIDTH - textWidth(&ttt->font_rodin, ">"),
                     (TFT_HEIGHT - ttt->font_rodin.height) / 2);
        }
    }

    // Draw arrow if there is one
    if ((ttt->instructionArrow.base.x != ttt->instructionArrow.tip.x)
        || (ttt->instructionArrow.base.y != ttt->instructionArrow.tip.y))
    {
        drawLine(ttt->instructionArrow.tip.x, ttt->instructionArrow.tip.y, ttt->instructionArrow.base.x,
                 ttt->instructionArrow.base.y, c550, 0);
        drawLine(ttt->instructionArrow.tip.x, ttt->instructionArrow.tip.y, ttt->instructionArrow.wing1.x,
                 ttt->instructionArrow.wing1.y, c550, 0);
        drawLine(ttt->instructionArrow.tip.x, ttt->instructionArrow.tip.y, ttt->instructionArrow.wing2.x,
                 ttt->instructionArrow.wing2.y, c550, 0);
    }
}

/**
 * @brief Clear the board to draw instructional diagrams
 *
 * @param ttt The entire game state
 */
static void instructionSetupBoard(ultimateTTT_t* ttt)
{
    // Clear the board
    memset(&ttt->game.subgames[0][0], 0, sizeof(ttt->game.subgames));

    // Reset the cursor
    ttt->game.cursor.x          = 0;
    ttt->game.cursor.y          = 0;
    ttt->game.selectedSubgame.x = 0;
    ttt->game.selectedSubgame.y = 0;
    ttt->game.cursorMode        = SELECT_SUBGAME;

    // Set the state as not playing yet
    ttt->game.state = TGS_PLACING_MARKER;

    // Default indices
    ttt->game.p1MarkerIdx = 0;
    ttt->game.p2MarkerIdx = 1;

    // Fake this
    ttt->game.p2p.cnc.playOrder = GOING_FIRST;

    ttt->showingInstructions = true;
}

/**
 * @brief Setup the board to demonstrate winning a subgame
 *
 * @param ttt The entire game state
 */
static void instructionSetupSmallWin(ultimateTTT_t* ttt)
{
    instructionSetupBoard(ttt);

    ttt->game.subgames[1][1].game[0][0] = TTT_P1;
    ttt->game.subgames[1][1].game[1][1] = TTT_P1;
    ttt->game.subgames[1][1].game[0][1] = TTT_P2;
    ttt->game.subgames[1][1].game[2][1] = TTT_P2;

    ttt->game.cursor.x          = 0;
    ttt->game.cursor.y          = 2;
    ttt->game.selectedSubgame.x = 1;
    ttt->game.selectedSubgame.y = 1;
    ttt->game.cursorMode        = SELECT_CELL;
}

/**
 * @brief Setup the board to demonstrate invalid placement
 *
 * @param ttt The entire game state
 */
static void instructionSetupInvalidMove(ultimateTTT_t* ttt)
{
    ttt->game.subgames[0][2].game[0][0] = TTT_P1;
    ttt->game.subgames[0][2].game[2][2] = TTT_P1;
    ttt->game.subgames[0][2].game[0][1] = TTT_P2;
    // ttt->game.subgames[0][2].game[2][1] = TTT_P2;

    ttt->game.cursor.x          = 1;
    ttt->game.cursor.y          = 0;
    ttt->game.selectedSubgame.x = 0;
    ttt->game.selectedSubgame.y = 2;
    ttt->game.cursorMode        = SELECT_CELL;
}

/**
 * @brief Setup the board to demonstrate winning the game
 *
 * @param ttt The entire game state
 */
static void instructionSetupGameWin(ultimateTTT_t* ttt)
{
    instructionSetupBoard(ttt);

    ttt->game.subgames[0][2].winner = TTT_P1;
    ttt->game.subgames[1][1].winner = TTT_P1;
    ttt->game.subgames[0][1].winner = TTT_P2;
    ttt->game.subgames[2][1].winner = TTT_P2;

    ttt->game.subgames[2][0].game[0][0] = TTT_P1;
    ttt->game.subgames[2][0].game[1][1] = TTT_P1;
    ttt->game.subgames[2][0].game[0][1] = TTT_P2;
    ttt->game.subgames[2][0].game[2][1] = TTT_P2;

    ttt->game.cursor.x   = 0;
    ttt->game.cursor.y   = 0;
    ttt->game.cursorMode = SELECT_SUBGAME;
}
