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

static pascal OSErr handleOpenDocumentEvent(const AppleEvent* event, AppleEvent* reply, SRefCon handlerRef);
static OSStatus globalEventHandler(EventHandlerCallRef handler, EventRef event, void* data);
static void processEvents(const char** out);

char pathBuffer[1024];
EventHandlerRef globalEventHandlerRef;
FILE* logFile;

int main(int argc, char** argv)
{
    const char* argumentName = NULL;

#ifdef LOG_FILE
    logFile = fopen(LOG_FILE, "a");
#endif

    LOG("Starting up\n");

    processEvents(&argumentName);

    LOG("Done processing events\n");
    LOG("Argument was: %s\n", argumentName ? argumentName : "NULL");

    return 0;
}

static pascal OSErr handleOpenDocumentEvent(const AppleEvent* event, AppleEvent* reply, SRefCon handlerRef)
{
    LOG("Hey, handleOpenDocumentEvent() got called!\n");
    if (((FourCharCode)handlerRef) != 'odoc')
    {
        LOG("Ignoring event, not 'odoc'\n");
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
            LOG("Error getting event doc index %ld\n", i);
            return result;
        }

        CFURLRef docUrlRef = CFURLCreateWithBytes(NULL, (UInt8*)buffer, docSize, kCFStringEncodingUTF8, NULL);

        if (docUrlRef != NULL)
        {
            CFStringRef docStringRef = CFURLCopyFileSystemPath(docUrlRef, kCFURLPOSIXPathStyle);
            if (docStringRef != NULL)
            {
                if (CFStringGetFileSystemRepresentation(docStringRef, pathBuffer, sizeof(pathBuffer)))
                {
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

static void processEvents(const char** out)
{
    memset(pathBuffer, 0, sizeof(pathBuffer));

    // Install handler
    AEEventHandlerUPP openDocHandler = NewAEEventHandlerUPP(handleOpenDocumentEvent);
    OSStatus result = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, openDocHandler, 0, false);

    if (result != noErr)
    {
        LOG("Failed to install OpenDocument handler\n");
        DisposeAEEventHandlerUPP(openDocHandler);
        return;
    }

    // Install the application-level handler
    const EventTypeSpec eventTypes[] = {{.eventClass = kEventClassAppleEvent, .eventKind = kEventAppleEvent}};

    EventHandlerUPP globalHandler = NewEventHandlerUPP(globalEventHandler);
    result = InstallApplicationEventHandler(globalHandler, 1, eventTypes, NULL, &globalEventHandlerRef);

    if (result != noErr)
    {
        LOG("Failed to install global event handler\n");
        DisposeEventHandlerUPP(globalHandler);
        return;
    }
    // Handler successfully installed, now check for events

    do {
    EventRef eventRef;
    // Half a second timeout
    EventTimeout timeout = 0.5;

    result = ReceiveNextEvent(1, eventTypes, timeout, kEventRemoveFromQueue, &eventRef);

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
        if (result == noErr)
        {
            // Our event handler should set up pathBuffer here
            if (*pathBuffer)
            {
                LOG("Event successfully set pathBuffer\n");
                *out = pathBuffer;
            }
        }
        else if (result == eventNotHandledErr)
        {
            LOG("Got eventNotHandledErr from SendEventToEventTarget()\n");
        }
        else
        {
            LOG("Error in SendEventToEventTarget(): %d %s\n", result, strerror(result));
            break;
        }
    }
    else
    {
        LOG("Error in ReceiveNextEvent()\n");
        break;
    }
    } while (1);//result != eventNotHandledErr);

    DisposeEventHandlerUPP(globalEventHandler);

    result = AERemoveEventHandler(kCoreEventClass, kAEOpenDocuments, handleOpenDocumentEvent, false);
    if (result != noErr)
    {
        LOG("Failed to uninstall OpenDocument handler\n");
    }

}
