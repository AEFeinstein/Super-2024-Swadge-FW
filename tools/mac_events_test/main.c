#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <Carbon/Carbon.h>

#define LOG_FILE "/Users/dylwhich/events.log"

#ifdef LOG_FILE
#define LOG(...) fprintf(logFile, __VA_ARGS__)
#else
#define LOG printf
#endif

static pascal OSErr handleOpenDocumentEvent(const AppleEvent* reply, SRefCon handlerRef);
static void processEvents(const char** out);

static char pathBuffer[1024];
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

static void processEvents(const char** out)
{
    memset(pathBuffer, 0, sizeof(pathBuffer));

    // Install handler
    AEEventHandlerUPP handler = NewAEEventHandlerUPP(handleOpenDocumentEvent);
    OSStatus result = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, handler, 0, false);

    if (result != noErr)
    {
        LOG("Failed to install OpenDocument handler\n");
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
        LOG("No event received after timeout\n");
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
        else
        {
            LOG("Error in SendEventToEventTarget()\n");
        }
    }
    else
    {
        LOG("Error in ReceiveNextEvent()\n");
    }

    result = AERemoveEventHandler(kCoreEventClass, kAEOpenDocuments, handleOpenDocumentEvent, false);
    if (result != noErr)
    {
        LOG("Failed to uninstall OpenDocument handler\n");
    }

}
