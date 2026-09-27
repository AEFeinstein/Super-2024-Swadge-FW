//==============================================================================
// Includes
//==============================================================================

#include "fairyCollectionData.h"

//==============================================================================
// Consts
//==============================================================================

const int fairyMaxList[FC_OPT_COUNT] = {
    FC_NUM_YEAR_OPTIONS, FC_AT_COUNT,  FC_CARD_COUNT, FC_NONE,       FC_SHAPE_COUNT, FC_CHARM_COUNT,
    FC_FILL_COUNT,       FC_PED_COUNT, FC_COL_COUNT,  FC_WING_COUNT, FC_BALL_COUNT,  FC_AURA_COUNT,
};
const char* const nvsStrs[] = {
    "fairy-col", "user-fairy", "user-card", "spp-saved", "spp-nextIdx",
};
extern const char* const fcOptionTypes[] = {
    "Years Attended:", "Type of Attendee:", "Card Background:",  "Team:",
    "Bottle shape:",   "Bottle Charm:",     "Bottle filling:",   "Bottle Pedestal:",
    "Fairy Colors:",   "Fiary Wing Shape:", "Fairy Ball Shape:", "Fairy Aura:",
};
const char* const fcAttendeeText[] = {
    "Attendee",     "Staff", "Volunteer", "Guest",  "Performer", "Cosplayer", "Maker", "Panalist",
    "Photographer", "Gamer", "Developer", "Artist", "Musician",  "Partier",   "Furry", "Pinball Wizard",
};
const char* const fcCardText[] = {
    "Scrolls", "Pac-Man", "Sheet Music", "Unused1", "Unused2", "Unused3", "Unused4", "Unused5",
    "Red",     "Orange",  "Yellow",      "Green",   "Blue",    "Indigio", "Violet",  "White",
};
const char* const fcTeamText[] = {
    "Blue",
    "Red",
    "Yellow",
    "Unset",
};
const char* const fcShapeText[] = {
    "Mason Jar", "Mason Jar with Handle", "Small-mouth Jar", "Large Flask", "Erlenmeyer", "Heart", "Unused1", "Unused2",
};
const char* const fcCharmText[] = {
    "No Charm", "Music Note", "Heart", "Skull", "Smiley", "Controller", "Guitar", "Gem",
};
const char* const fcFillingText[] = {
    "No filler", "Crystal", "Grass", "Marbles", "Dice", "Seashells", "Paint Supplies", "Toy Cars",
};
const char* const fcPedestalText[] = {
    "No pedestal", "Wood Plate", "Small Rug", "Doily", "Notebook Page", "Stone", "Tea Plate", "Mushrooms",
};
const char* const fcColorsText[] = {
    "Pink", "Red", "Blue", "Green", "Yellow", "Purple", "Gray", "Orange",
};
const char* const fcWingText[] = {
    "No Wings", "Classic", "Round", "Tiny", "Swirly", "Bat", "Curly", "Monarch",
};
const char* const fcBallText[] = {
    "Circular", "Oval", "Two segments", "Tomi", "Teardrop", "UNUSED1", "UNUSED2", "UNUSED3",
};
const char* const fcAuraText[] = {
    "No Aura", "Sparkles", "Musical Notes", "Glow", "Bubbles", "Stink", "UNUSED1", "UNUSED2",
};