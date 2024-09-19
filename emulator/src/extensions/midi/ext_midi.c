#include "ext_midi.h"
#include "emu_ext.h"
#include "emu_main.h"

#include "hdw-nvs_emu.h"
#include "emu_cnfs.h"
#include "ext_modes.h"
#include "mode_synth.h"

#include <stdbool.h>

//==============================================================================
// Function Prototypes
//==============================================================================

static bool midiInitCb(emuArgs_t* emuArgs);

//==============================================================================
// Variables
//==============================================================================

emuExtension_t midiEmuExtension = {
    .name            = "midi",
    .fnInitCb        = midiInitCb,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = NULL,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = NULL,
};

//==============================================================================
// Functions
//==============================================================================

static bool midiInitCb(emuArgs_t* emuArgs)
{
    if (emuArgs->midiFile)
    {
        printf("Opening MIDI file: %s\n", emuArgs->midiFile);
        if (emuCnfsInjectFile(emuArgs->midiFile, emuArgs->midiFile))
        {
            emuInjectNvs32("storage", "synth_playmode", 1);
            emuInjectNvsBlob("storage", "synth_lastsong", strlen(emuArgs->midiFile), emuArgs->midiFile);
            emulatorSetSwadgeModeByName(synthMode.modeName);
        }
        else
        {
            printf("Could not read MIDI file!\n");
            emulatorQuit();
            return false;
        }

        return true;
    }

    return false;
}