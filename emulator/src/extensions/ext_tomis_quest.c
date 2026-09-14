#include "ext_tomis_quest.h"
#include "emu_ext.h"
#include "emu_main.h"
#include "emu_utils.h"

#include "hdw-nvs_emu.h"
#include "emu_cnfs.h"
#include "ext_modes.h"
#include "mode_ray.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef EMU_MACOS
    // Used to handle DocumentOpen event that OSX uses instead of Just Putting It In Argv
    #include <Carbon/Carbon.h>
#endif

//==============================================================================
// Types
//==============================================================================

#ifdef EMU_MACOS
typedef void (*MacOpenFileCb)(const char* path);

typedef struct
{
    EventHandlerUPP globalEventHandler;
    AEEventHandlerUPP appleEventHandler;
    EventHandlerRef globalEventHandlerRef;
    MacOpenFileCb openFileCallback;
} MacOpenFileHandler;
#endif

//==============================================================================
// Function Prototypes
//==============================================================================

static bool tomiInitCb(emuArgs_t* emuArgs);
static void tomiPreFrameCb(uint64_t frame);
static bool tomiInjectFile(const char* path);

#ifdef EMU_MACOS
// Exists but isn't declared in the headers
extern Boolean ConvertEventRefToEventRecord(EventRef, EventRecord*);

bool mg_installMacOpenFileHandler(MacOpenFileHandler* handlerRef, MacOpenFileCb callback);
void mg_checkForEventsMacOpenFileHandler(MacOpenFileHandler* handlerRef, uint32_t millis);
void mg_unmg_installMacOpenFileHandler(MacOpenFileHandler* handlerRef);

static pascal OSErr handleOpenDocumentEvent(const AppleEvent* event, AppleEvent* reply, SRefCon handlerRef);
static OSStatus globalEventHandler(EventHandlerCallRef handler, EventRef event, void* data);
static void doFileOpenCb(const char* path);
#endif

//==============================================================================
// Variables
//==============================================================================

emuExtension_t tomiEmuExtension = {
    .name            = "tomi",
    .fnInitCb        = tomiInitCb,
    .fnPreFrameCb    = tomiPreFrameCb,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = NULL,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = NULL,
};

#ifdef EMU_MACOS
static char tomiPathBuffer[1024];
#endif
static const char* tomiFile = NULL;

#ifdef EMU_MACOS
static const EventTypeSpec eventTypes[] = {{.eventClass = kEventClassAppleEvent, .eventKind = kEventAppleEvent}};

static bool handlerInstalled = false;
static bool emulatorStarted  = false;
static MacOpenFileHandler macOpenFileHandler;
#endif

//==============================================================================
// Functions
//==============================================================================

static bool tomiInitCb(emuArgs_t* emuArgs)
{
    if (emuArgs->tomiFile)
    {
        tomiFile = emuArgs->tomiFile;
    }

#ifdef EMU_MACOS
    handlerInstalled = mg_installMacOpenFileHandler(&macOpenFileHandler, doFileOpenCb);
    // Wait up to 100ms for an event at startup
    mg_checkForEventsMacOpenFileHandler(&macOpenFileHandler, 100);
    emulatorStarted = true;
#endif

    if (tomiFile)
    {
        printf("Opening Tomi's Quest level file: %s\n", tomiFile);
        if (!tomiInjectFile(tomiFile))
        {
            printf("Could not read Tomi's Quest level file!\n");
            emulatorQuit();
            return false;
        }

        return true;
    }

    return false;
}

void tomiPreFrameCb(uint64_t frame)
{
#ifdef EMU_MACOS
    if (handlerInstalled)
    {
        // Wait up to 5ms for an event, is that enough?
        mg_checkForEventsMacOpenFileHandler(&macOpenFileHandler, 5);
    }
#endif
}

static bool tomiInjectFile(const char* path)
{
    if (emuCnfsInjectFile(tomiFile, tomiFile))
    {
        emuNvsInjectBlobFile("storage", "user_level", tomiFile);
        emulatorSetSwadgeModeByName(rayMode.modeName);

        return true;
    }
    else
    {
        return false;
    }
}

#ifdef EMU_MACOS
static void doFileOpenCb(const char* path)
{
    strncpy(tomiPathBuffer, path, sizeof(tomiPathBuffer));
    tomiFile = tomiPathBuffer;

    if (emulatorStarted)
    {
        if (!tomiInjectFile(path))
        {
            printf("Error: could not read Tomi's Quest level file %s!\n", path);
        }
    }
}

bool mg_installMacOpenFileHandler(MacOpenFileHandler* handlerRef, MacOpenFileCb callback)
{
    // Init handler
    handlerRef->appleEventHandler  = NULL;
    handlerRef->globalEventHandler = NULL;
    handlerRef->openFileCallback   = callback;

    // Install handler
    handlerRef->appleEventHandler = NewAEEventHandlerUPP(handleOpenDocumentEvent);
    OSStatus result = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, handlerRef->appleEventHandler,
                                            (SRefCon)handlerRef, false);

    if (result != noErr)
    {
        printf("Failed to install OpenDocument handler\n");
        mg_unmg_installMacOpenFileHandler(handlerRef);
        return false;
    }

    // Install the application-level handler
    handlerRef->globalEventHandler = NewEventHandlerUPP(globalEventHandler);
    result                         = InstallApplicationEventHandler(handlerRef->globalEventHandler, 1, eventTypes, NULL,
                                                                    &handlerRef->globalEventHandlerRef);

    if (result != noErr)
    {
        printf("Failed to install global event handler\n");
        mg_unmg_installMacOpenFileHandler(handlerRef);
        return false;
    }

    // Handler successfully installed
    return true;
}

// Runs the event loop and waits for up to the specified time
// The callback will be called for any events that are recevied
void mg_checkForEventsMacOpenFileHandler(MacOpenFileHandler* handlerRef, uint32_t millis)
{
    EventTimeout timeout = millis / 1000.0;
    while (1)
    {
        EventRef eventRef;

        OSErr result = ReceiveNextEvent(1, eventTypes, timeout, kEventRemoveFromQueue, &eventRef);

        if (result == eventLoopTimedOutErr)
        {
            // printf("No event received after timeout\n");
            break;
        }
        else if (result == noErr)
        {
            result = SendEventToEventTarget(eventRef, GetEventDispatcherTarget());
            ReleaseEvent(eventRef);
            if (result != noErr)
            {
                if (result == eventNotHandledErr)
                {
                    // printf("Got eventNotHandledErr from SendEventToEventTarget()\n");
                }
                else
                {
                    printf("Error in SendEventToEventTarget(): %d %s\n", result, strerror(result));
                    break;
                }
            }
        }
        else
        {
            printf("Error in ReceiveNextEvent()\n");
            break;
        }
    }
}

// Uninstalls and deletes
void mg_unmg_installMacOpenFileHandler(MacOpenFileHandler* handlerRef)
{
    if (handlerRef != NULL)
    {
        if (handlerRef->appleEventHandler != NULL)
        {
            DisposeAEEventHandlerUPP(handlerRef->appleEventHandler);
            handlerRef->appleEventHandler = NULL;
        }

        if (handlerRef->globalEventHandler != NULL)
        {
            DisposeEventHandlerUPP(handlerRef->globalEventHandler);
            handlerRef->globalEventHandler = NULL;
        }
        handlerRef->openFileCallback = NULL;
    }
}

static pascal OSErr handleOpenDocumentEvent(const AppleEvent* event, AppleEvent* reply, SRefCon handlerRefArg)
{
    MacOpenFileHandler* handlerRef = (MacOpenFileHandler*)handlerRefArg;

    AEDescList docList;
    OSErr result = AEGetParamDesc(event, keyDirectObject, typeAEList, &docList);

    if (result != noErr)
    {
        return result;
    }

    long docCount = 0;
    result        = AECountItems(&docList, &docCount);
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
            return result;
        }

        CFURLRef docUrlRef = CFURLCreateWithBytes(NULL, (UInt8*)buffer, docSize, kCFStringEncodingUTF8, NULL);

        if (docUrlRef != NULL)
        {
            CFStringRef docStringRef = CFURLCopyFileSystemPath(docUrlRef, kCFURLPOSIXPathStyle);
            if (docStringRef != NULL)
            {
                char pathBuffer[1024];
                if (CFStringGetFileSystemRepresentation(docStringRef, pathBuffer, sizeof(pathBuffer)))
                {
                    handlerRef->openFileCallback(pathBuffer);
                }
                CFRelease(docStringRef);
            }
            CFRelease(docUrlRef);
        }
    }

    return AEDisposeDesc(&docList);
}

static OSStatus globalEventHandler(EventHandlerCallRef handler, EventRef event, void* data)
{
    bool inQueue = IsEventInQueue(GetMainEventQueue(), event);

    if (inQueue)
    {
        RetainEvent(event);
        RemoveEventFromQueue(GetMainEventQueue(), event);
    }

    EventRecord record;
    ConvertEventRefToEventRecord(event, &record);
    char messageStr[5] = {
        (char)((record.message >> 24) & 0xff),
        (char)((record.message >> 16) & 0xff),
        (char)((record.message >> 8) & 0xff),
        (char)((record.message) & 0xff),
        0,
    };
    printf("globalEventHandler() what=%hu, message=%s\n", record.what, messageStr);
    OSStatus result = AEProcessAppleEvent(&record);

    if (result == errAEEventNotHandled)
    {
        printf("errAEEventNotHandled in globalEventHandler()\n");
    }
    else if (result != noErr)
    {
        printf("globalEventHandler() AEProcessAppleEvent() returned ERROR: %d (%s)\n", result, strerror(result));
    }
    else
    {
        printf("globalEventHandler() AEProcessAppleEvent() success!\n");
    }

    if (inQueue)
    {
        ReleaseEvent(event);
    }

    return noErr;
}

#endif
