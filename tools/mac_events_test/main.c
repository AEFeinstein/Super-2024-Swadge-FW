#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <Carbon/Carbon.h>

extern Boolean ConvertEventRefToEventRecord(EventRef, EventRecord*);

#define LOG_FILE "/Users/dylwhich/events.log"

#ifdef LOG_FILE
#define LOG(...) fprintf(logFile, __VA_ARGS__)
#else
#define LOG printf
#endif

typedef void (*MacOpenFileCb)(const char* path);

typedef struct
{
    EventHandlerUPP globalEventHandler;
    AEEventHandlerUPP appleEventHandler;
    MacOpenFileCb openFileCallback;
    EventHandlerRef globalEventHandlerRef;
} MacOpenFileHandler;

bool installMacOpenFileHandler(MacOpenFileHandler* handlerRef, MacOpenFileCb callback);
void checkForEventsMacOpenFileHandler(MacOpenFileHandler* handlerRef, uint32_t millis);
void uninstallMacOpenFileHandler(MacOpenFileHandler* handlerRef);

static pascal OSErr handleOpenDocumentEvent(const AppleEvent* event, AppleEvent* reply, SRefCon handlerRef);
static OSStatus globalEventHandler(EventHandlerCallRef handler, EventRef event, void* data);
static void doOpenCb(const char* path);

FILE* logFile;

const EventTypeSpec eventTypes[] = {{.eventClass = kEventClassAppleEvent, .eventKind = kEventAppleEvent}};

static void doOpenCb(const char* path)
{
    LOG("Got file: %s\n", path);
}

int main(int argc, char** argv)
{
#ifdef LOG_FILE
    logFile = fopen(LOG_FILE, "a");
#endif

    LOG("\nStarting up\n");

    LOG("Installing event handler...\n");
    MacOpenFileHandler handler;

    bool installed = installMacOpenFileHandler(&handler, doOpenCb);

    if (installed)
    {
        LOG("OK!\n");
        checkForEventsMacOpenFileHandler(&handler, 500);
        uninstallMacOpenFileHandler(&handler);
        LOG("Done processing events\n");
    }
    else
    {
        LOG("FAILED!");
        return 1;
    }

    return 0;
}

bool installMacOpenFileHandler(MacOpenFileHandler* handlerRef, MacOpenFileCb callback)
{
    // Init handler
    handlerRef->appleEventHandler = NULL;
    handlerRef->globalEventHandler = NULL;
    handlerRef->openFileCallback = callback;

    // Install handler
    handlerRef->appleEventHandler = NewAEEventHandlerUPP(handleOpenDocumentEvent);
    OSStatus result = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, handlerRef->appleEventHandler, (SRefCon)handlerRef, false);

    if (result != noErr)
    {
        LOG("Failed to install OpenDocument handler\n");
        uninstallMacOpenFileHandler(handlerRef);
        return false;
    }

    // Install the application-level handler
    const EventTypeSpec eventTypes[] = {{.eventClass = kEventClassAppleEvent, .eventKind = kEventAppleEvent}};

    handlerRef->globalEventHandler = NewEventHandlerUPP(globalEventHandler);
    result = InstallApplicationEventHandler(handlerRef->globalEventHandler, 1, eventTypes, NULL, &handlerRef->globalEventHandlerRef);

    if (result != noErr)
    {
        LOG("Failed to install global event handler\n");
        uninstallMacOpenFileHandler(handlerRef);
        return false;
    }

    // Handler successfully installed
    return true;
}

// Runs the event loop and waits for up to the specified time
// The callback will be called for any events that are recevied
void checkForEventsMacOpenFileHandler(MacOpenFileHandler* handlerRef, uint32_t millis)
{
    EventTimeout timeout = millis / 1000.0;
    while (1)
    {
        EventRef eventRef;

        OSErr result = ReceiveNextEvent(1, eventTypes, timeout, kEventRemoveFromQueue, &eventRef);

        if (result == eventLoopTimedOutErr)
        {
            LOG("No event received after timeout\n");
            break;
        }
        else if (result == noErr)
        {
            LOG("Got an event!\n");
            result = SendEventToEventTarget(eventRef, GetEventDispatcherTarget());
            ReleaseEvent(eventRef);
            if (result != noErr)
            {
                if (result == eventNotHandledErr)
                {
                    LOG("Got eventNotHandledErr from SendEventToEventTarget()\n");
                }
                else
                {
                    LOG("Error in SendEventToEventTarget(): %d %s\n", result, strerror(result));
                    break;
                }
            }
        }
        else
        {
            LOG("Error in ReceiveNextEvent()\n");
            break;
        }
    }
}

// Uninstalls and deletes
void uninstallMacOpenFileHandler(MacOpenFileHandler* handlerRef)
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
    LOG("Hey, handleOpenDocumentEvent() got called!\n");
    MacOpenFileHandler* handlerRef = (MacOpenFileHandler*)handlerRefArg;

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
            LOG("Error getting event doc index %ld\n", i);
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
                    LOG("Successfully handled event?\n");
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
    LOG("globalEventHandler()!!!!!!\n");

    bool inQueue = IsEventInQueue(GetMainEventQueue(), event);

    if (inQueue)
    {
        RetainEvent(event);
        RemoveEventFromQueue(GetMainEventQueue(), event);
    }

    EventRecord record;
    ConvertEventRefToEventRecord(event, &record);
    char messageStr[5] =
    {
        (char)((record.message >> 24) & 0xff),
        (char)((record.message >> 16) & 0xff),
        (char)((record.message >> 8) & 0xff),
        (char)((record.message) & 0xff),
        0,
    };
    LOG("globalEventHandler() what=%hu, message=%s\n", record.what, messageStr);
    OSStatus result = AEProcessAppleEvent(&record);

    if (result == errAEEventNotHandled)
    {
        LOG("errAEEventNotHandled in globalEventHandler()\n");
    }
    else if (result != noErr)
    {
        LOG("globalEventHandler() AEProcessAppleEvent() returned ERROR: %d (%s)\n", result, strerror(result));
    }
    else
    {
        LOG("globalEventHandler() AEProcessAppleEvent() success!\n");
    }

    if (inQueue)
    {
        ReleaseEvent(event);
    }

    return noErr;
}

