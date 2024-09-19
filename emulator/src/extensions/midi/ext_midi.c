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
    if (((FourCharCode)handlerRef) != 'odoc')
    {
        printf("Ignoring event, not 'odoc'\n");
        return noErr;
    }

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

    char buffer[2048];

    // Yup, it's zero-indexed. Weird.
    for (long i = 1; i <= docCount; i++)
    {
        AEKeyword keyword;
        DescType docType;
        Size docSize;

        result = AEGetNthPtr(&docList, i, typeFileURL, &keyword, &docType, &buffer, sizeof(buffer), &docSize);

        if (result != noErr)
        {
            printf("Error getting event doc index %ld\n", i);
            return result;
        }

        CFURLRef docUrlRef = CFURLCreateWithBytes(NULL, (UInt8*)buffer, docSize, kCFStringEncodingUTF8, NULL);

        if (docUrlRef != NULL)
        {
            CFStringRef docStringRef = CFURLCopyFileSystemPath(docUrlRef, kCFURLPOSIXPathStyle);
            if (docStringRef != NULL)
            {
                if (CFStringGetFileSystemRepresentation(docStringRef, midiPathBuffer, sizeof(midiPathBuffer))
                {
                    printf("Successfully handled event?\n");
                }
                CFRelease(docStringRef);
            }
            CFRelease(docUrlRef);
        }
    }

    return AEDisposeDesc(&docList);
}

#endif

static void processEvents(const char** out)
{
#ifdef EMU_MACOS
    memset(midiPathBuffer, 0, sizeof(midiPathBuffer));

    // Install handler
    AEEventHandlerUPP handler = NewAEEventHandlerUPP(handleOpenDocumentEvent);
    OSStatus result = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, handler, 0, false);

    if (result != noErr)
    {
        printf("Failed to install OpenDocument handler\n");
        DisposeAEEventHandlerUPP(handler);
        return;
    }
    // Handler successfully installed, now check for events

    EventRef eventRef;
    // Half a second timeout
    EventTimeout timeout = 0.5;
    const EventTypeSpec eventTypes[] = {{.eventClass = kEventClassAppleEvent, .eventKind = kEventAppleEvent}};

    result = ReceiveNextEvent(1, eventTypes, timeout, kEventRemoveFromQueue, &eventRef);

    if (result == eventLoopTimedOutErr)
    {
        printf("No event received after timeout\n");
    }
    else if (result == noErr)
    {
        printf("Got an event!\n");
        result = SendEventToEventTarget(eventRef, GetEventDispatcherTarget());
        ReleaseEvent(eventRef);
        if (result == noErr)
        {
            // Our event handler should set up midiPathBuffer here
            if (*midiPathBuffer)
            {
                printf("Event successfully set midiPathBuffer\n");
                *out = midiPathBuffer;
            }
        }
        else
        {
            printf("Error in SendEventToEventTarget()\n");
        }
    }
    else
    {
        printf("Error in ReceiveNextEvent()\n");
    }

    result = AERemoveEventHandler(kCoreEventClass, kAEOpenDocuments, handleOpenDocumentEvent, false);
    if (result != noErr)
    {
        printf("Failed to uninstall OpenDocument handler\n");
    }

#endif
}
