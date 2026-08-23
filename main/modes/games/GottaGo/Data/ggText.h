#pragma once

//==============================================================================
// Text
//==============================================================================

static const char* const warningText[] = {
    "ATTENTION",
    "This game contains 'suggestive themes' and may not be suitable for all audiences. Press A to continue and "
    "anything other button to back out.",
    "Suggestive themes is a stupid way to put 'this game shows butts.' Saturday morning cartoons shows butts. "
    "Everyone has one, this isn't going to cause someone to have some sort of awakening or turn your children into "
    "perverts, it's just some very low pixel count butts. Geting you underwear in a twist over this says more about "
    "you that you would probably like.",
};

typedef enum
{
    GG_WARNING_TEXT,
    GG_WARNING_MESSAGE,
    GG_WARNING_MANI,
} ggWarningText_t;

static const char* titleText = "Press 'A' to start!";

static const char* const levelText[] = {
    "Ready?",
    "3",
    "2",
    "1",
    "Paused",
    "Press A to continue, B to quit",
    "Good choice!",
    "You used a stall.",
    "Press any button to advance to next round",
    "You peed yourself",
    "You used a broken toilet",
    "All the stalls were filled",
    "Press any button to go back to the menu",
};

typedef enum
{
    GG_TEXT_READY,
    GG_TEXT_3,
    GG_TEXT_2,
    GG_TEXT_1,
    GG_TEXT_PAUSED,
    GG_TEXT_INSTR_PAUSE,
    GG_TEXT_GOOD_1,
    GG_TEXT_GOOD_2,
    GG_TEXT_INSTR_GOOD,
    GG_TEXT_BAD_1,
    GG_TEXT_BAD_2,
    GG_TEXT_BAD_3,
    GG_TEXT_INSTR_BAD,
} ggLevelText_t;

static const char* const menuText[] = {
    "Play!",
    "Rules",
    "High Scores",
    "Options",
    "Activate Helper Mode: ",
    "Touch entry: ",
    "Quit",
    "On",
    "Off",
};

typedef enum
{
    GG_TEXT_MENU_PLAY,
    GG_TEXT_MENU_RULES,
    GG_TEXT_MENU_HS,
    GG_TEXT_MENU_OPTIONS,
    GG_TEXT_MENU_COUNT
} ggMenuText_t;

typedef enum
{
    GG_TEXT_OPTIONS_HELPER = GG_TEXT_MENU_COUNT,
    GG_TEXT_OPTIONS_TOUCH,
    GG_TEXT_OPTIONS_COUNT
} ggOptionsText_t;

typedef enum
{
    GG_TEXT_QUIT = GG_TEXT_OPTIONS_COUNT,
    GG_TEXT_ON,
    GG_TEXT_OFF,
} ggSettingsText;

static const char* const rulesText[] = {
    "Rules",
    "Here's the rules book for Gotta Go! The rules should be instinctual for a lot of people, but for those that don't "
    "use urinals on a regular basis, this will help.",
    "Controls",
    "Use the left/right arrows to pick a urinal, A to select it, or B to use the stall. There is no pausing!",
    "How to play",
    "Once you're ready, you must quickly figure out the most ideal urinal out of the available options. Be quick, your "
    "bladder is about to burst!",
    "Urinals",
    "You always want to use the cleanest urinal. Obviously. Try not to stand in puddles, and if you use one that's out "
    "of order and increase the mess, expect to be judged.",
    "People",
    "People are weird. Try to stay as far away from them as possible. If you have to pick, always pick the most normal "
    "one. That means not the guy with his pants around his ankles or the guy that smells like onions.",
    "Stalls",
    "You can use the stall sometimes, but you only have a limit amount of times you can do that. No hogging the "
    "stalls.",
    "Scoring",
    "Score is tracked in three ways: Total score, accuracy, and adjusted score. The total score is an accumulation of "
    "all the score up to this point. Accuracy score is how close to optimal picks you are. Th adjusted score takes "
    "your total score and adjusts it by your accuracy to provide a final number that's easy to compare.",
};