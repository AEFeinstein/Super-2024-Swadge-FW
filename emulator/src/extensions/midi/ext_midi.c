#include "ext_midi.h"
#include "emu_ext.h"
#include "emu_main.h"
#include "emu_utils.h"

#include "hdw-nvs_emu.h"
#include "emu_cnfs.h"
#include "ext_modes.h"
#include "mode_synth.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef EMU_MACOS
// Used to handle DocumentOpen event that OSX uses instead of Just Putting It In Argv
#include <Carbon/Carbon.h>
#endif

//==============================================================================
// Function Prototypes
//==============================================================================

static bool midiInitCb(emuArgs_t* emuArgs);
static void processEvents(const char** out);

#ifdef EMU_MACOS
static pascal OSErr handleOpenDocumentEvent(const AppleEvent* event, AppleEvent* reply, SRefCon handlerRef);
#endif

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

static char midiPathBuffer[1024];

//==============================================================================
// Functions
//==============================================================================

static bool midiInitCb(emuArgs_t* emuArgs)
{
    const char* midiFile = NULL;
    if (emuArgs->midiFile)
    {
        midiFile = emuArgs->midiFile;
    }

    processEvents(&midiFile);

    if (midiFile)
    {
        printf("Opening MIDI file: %s\n", midiFile);
        if (emuCnfsInjectFile(midiFile, midiFile))
        {
            emuInjectNvs32("storage", "synth_playmode", 1);
            emuInjectNvsBlob("storage", "synth_lastsong", strlen(midiFile), midiFile);
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

#ifdef EMU_MACOS
static pascal OSErr handleOpenDocumentEvent(const AppleEvent* event, AppleEvent* reply, SRefCon handlerRef)
{
    AEDescList docList;
    OSErr result = AEGetParamDesc(event, keyDirectObject, typeAEList, &docList);

    if (result != noErr)
    {
        return result;
    }

    long docCount = 0;
    result = AECountItems(&docList, &docCount);
    if (result != noErr)
    {
        return result;
    }

    if (docCount > 0)
    {
        AEKeyword keyword;
        DescType docType;
        FSRef docRef;
        Size docSize;
        result = AEGetNthPtr(&docList, 0, typeFSRef, &keyword, &docType, &docRef, sizeof(docRef), &docSize);

        if (result != noErr)
        {
            return result;
        }

        CFURLRef docUrlRef = CFURLCreateFromFSRef(NULL, &docRef);
        CFStringRef docStringRef = CFURLCopyFileSystemPath(docUrlRef, kCFURLPOSIXPathStyle);
        CFRelease(docUrlRef);

        if (CFStringGetCString(docUrlRef, midiPathBuffer, sizeof(midiPathBuffer)), kCFStringEncodingASCII)
        {
            printf("Event handled?\n");
        }
        // Necessary?
        //CFRelease(docStringRef);
    }

    return AEDisposeDesc(&docList);
}
#endif

static void processEvents(const char** out)
{
#ifdef EMU_MACOS
    memset(midiPathBuffer, 0, sizeof(midiPathBuffer));

    // Install handler
    OSErr result = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, handleOpenDocumentEvent, 0, false);

    if (result != noErr)
    {
        printf("Failed to install OpenDocument handler\n");
        return;
    }

    // Handler successfully installed, now check for events

    EventRecord event;
    // Half a second timeout
    uint32_t timeout = 30;

    if (WaitNextEvent(highLevelEventMask, &event, timeout, NULL))
    {
        printf("Got an event!\n");
        AEProcessAppleEvent(&event);

        // Our event handler should set up midiPathBuffer here
        if (*midiPathBuffer)
        {
            printf("Event successfully set midiPathBuffer\n");
            *out = midiPathBuffer;
        }
    }

    result = AERemoveEventHandler(kCoreEventClass, kAEOpenDocuments, handleOpenDocumentEvent, false);
    if (result != noErr)
    {
        printf("Failed to uninstall OpenDocument handler\n");
    }

#endif
}
