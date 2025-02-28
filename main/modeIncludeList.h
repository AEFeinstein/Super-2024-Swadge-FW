/*!*/

// Mode Includes
/*
 Quickly regenerate with:
   grep -lirE '^extern swadgeMode_t (.*);' main | grep -oE '([^/]+\.h)' \
    | grep -v quickSettings | awk '{printf "#include \"%s\"\n",$1 }' | sort
*/

#include "accelTest.h"
#include "bongoTest.h"
#include "colorchord.h"
#include "dance.h"
#include "factoryTest.h"
#include "gamepad.h"
#include "introMode.h"
#include "jukebox.h"
#include "keebTest.h"
#include "mainMenu.h"
#include "modeTimer.h"
#include "mode_2048.h"
#include "mode_bigbug.h"
#include "mode_cGrove.h"
#include "mode_credits.h"
#include "mode_swadgeHero.h"
#include "mode_synth.h"
#include "pango.h"
#include "sequencerMode.h"
#include "soko.h"
#include "touchTest.h"
#include "tunernome.h"
#include "ultimateTTT.h"

int swadgeModeCount(void);

void modeListSetMenu(menu_t* menu);

extern swadgeMode_t* allSwadgeModes[];