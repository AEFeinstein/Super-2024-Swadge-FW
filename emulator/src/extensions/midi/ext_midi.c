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

static bool midiInitCb(emuArgs_t* emuArgs);
static void midiPreFrameCb(uint64_t frame);
static bool midiInjectFile(const char* path);

// WAV writing functions
static void renderDoneCallback(void);
static bool renderMidiToWav(const char* midiPath, const char* wavPath);

#ifdef EMU_MACOS
// Exists but isn't declared in the headers
extern Boolean ConvertEventRefToEventRecord(EventRef, EventRecord*);

bool installMacOpenFileHandler(MacOpenFileHandler* handlerRef, MacOpenFileCb callback);
void checkForEventsMacOpenFileHandler(MacOpenFileHandler* handlerRef, uint32_t millis);
void uninstallMacOpenFileHandler(MacOpenFileHandler* handlerRef);

static pascal OSErr handleOpenDocumentEvent(const AppleEvent* event, AppleEvent* reply, SRefCon handlerRef);
static OSStatus globalEventHandler(EventHandlerCallRef handler, EventRef event, void* data);
static void doFileOpenCb(const char* path);
#endif

//==============================================================================
// Variables
//==============================================================================

emuExtension_t midiEmuExtension = {
    .name            = "midi",
    .fnInitCb        = midiInitCb,
    .fnPreFrameCb    = midiPreFrameCb,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = NULL,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = NULL,
};

#ifdef EMU_MACOS
static char midiPathBuffer[1024];
#endif
static const char* midiFile = NULL;

#ifdef EMU_MACOS
static const EventTypeSpec eventTypes[] = {{.eventClass = kEventClassAppleEvent, .eventKind = kEventAppleEvent}};

static bool handlerInstalled = false;
static bool emulatorStarted  = false;
static MacOpenFileHandler macOpenFileHandler;
#endif

//==============================================================================
// Functions
//==============================================================================

static bool midiInitCb(emuArgs_t* emuArgs)
{
    if (emuArgs->midiFile)
    {
        midiFile = emuArgs->midiFile;
    }

#ifdef EMU_MACOS
    handlerInstalled = installMacOpenFileHandler(&macOpenFileHandler, doFileOpenCb);
    // Wait up to 100ms for an event at startup
    checkForEventsMacOpenFileHandler(&macOpenFileHandler, 100);
    emulatorStarted = true;
#endif

    if (midiFile)
    {
        printf("Opening MIDI file: %s\n", midiFile);
        if (!midiInjectFile(midiFile))
        {
            printf("Could not read MIDI file!\n");
            emulatorQuit();
            return false;
        }

        return true;
    }

    return false;
}

void midiPreFrameCb(uint64_t frame)
{
#ifdef EMU_MACOS
    if (handlerInstalled)
    {
        // Wait up to 5ms for an event, is that enough?
        checkForEventsMacOpenFileHandler(&macOpenFileHandler, 5);
    }
#endif
}

static bool midiInjectFile(const char* path)
{
    if (emuCnfsInjectFile(path, path))
    {
        emuInjectNvs32("storage", "synth_playmode", 1);
        emuInjectNvsBlob("storage", "synth_lastsong", strlen(path), path);
        emulatorSetSwadgeModeByName(synthMode.modeName);

        // Write <filename>.mid's recording to <filename>.wav
        char outFilePath[128] = {0};
        strcat(outFilePath, path);

        char* dotptr = strrchr(outFilePath, '.');
        snprintf(&dotptr[1], strlen(dotptr), "wav");

        if (renderMidiToWav(path, outFilePath))
        {
            printf("Success! Rendered MIDI to %s\n", outFilePath);
            //emulatorQuit();
        }
        else
        {
            printf("ERROR: Could not render MIDI to %s\n", outFilePath);
        }

        return true;
    }
    else
    {
        return false;
    }
}

static bool renderComplete = false;
static void renderDoneCallback(void)
{
    renderComplete = true;
}

#define NOCLIP 0

static bool renderMidiToWav(const char* midiPath, const char* wavPath)
{
    renderComplete = false;

    // Inject the MIDI file so we can lazily just use the existing stuff for it
    if (emuCnfsInjectFile("__wav_in.mid", midiPath))
    {
        // The parser is just for getting metadata
        midiFile_t tmpMidi = {0};
        midiFileReader_t parser = {0};
        if (!loadMidiFile("__wav_in.mid", &tmpMidi, false) || !initMidiParser(&parser, &tmpMidi))
        {
            printf("Error: invalid MIDI file %s!\n", midiPath);
            return false;
        }

        // We want meta event specifically, for text
        parser.handleMetaEvents = true;

        const char* artistName = NULL;
        const char* trackName = NULL;
        uint32_t songTicks = 0;

        // -1. Get the metadata for artist info, etc.
        midiEvent_t event = {0};
        while (midiNextEvent(&parser, &event))
        {
            if (event.type == META_EVENT && event.meta.type == COPYRIGHT)
            {
                // Artist name?
                printf("Artist name: '%s'\n", event.meta.text);
                if (artistName)
                {
                    free(artistName);
                    artistName = NULL;
                }
                artistName = strdup(event.meta.text);
            }
            else if (event.type == META_EVENT && event.meta.type == SEQUENCE_OR_TRACK_NAME)
            {
                // Track name?
                printf("Track name: '%s'\n", event.meta.text);
                if (trackName)
                {
                    free(trackName);
                    trackName = NULL;
                }
                trackName = strdup(event.meta.text);
            }
            else if (event.meta.type == END_OF_TRACK)
            {
                // Not technically correct, need to take variable tempo into account
                songTicks = event.absTime;
            }
        }
        deinitMidiParser(&parser);

        printf("Song length: %" PRIu32 "\n", songTicks);

        // 0. Open the file in read/write mode, truncating or creating the file
        FILE* outFile = fopen(wavPath, "wb+");

        if (outFile == NULL)
        {
            printf("Error: Cannot open WAV file %s for writing!\n", wavPath);
            unloadMidiFile(&tmpMidi);

            if (artistName)
            {
                free(artistName);
            }

            if (trackName)
            {
                free(trackName);
            }
            return false;
        }

        // 1. Write the RIFF header
        //    a. Magic bytes ('RIFF')
        fputs("RIFF", outFile);

        //    b. File size (We do not know what it will be yet, so write all zeroes)
        fputc(0, outFile);
        fputc(0, outFile);
        fputc(0, outFile);
        fputc(0, outFile);

        //    c. Format ID
        fputs("WAVE", outFile);

        // 2. Write format block
        //    a. Format Block ID
        fputs("fmt ", outFile);

        //    b. Format chunk size minus 8 - 4 bytes
        fputc(16, outFile);
        fputc(0, outFile);
        fputc(0, outFile);
        fputc(0, outFile);

        //    b. Audio format (1 for PCM, 3 for float) - 2 bytes
        fputc(1, outFile);
        fputc(0, outFile);

        //    c. Channel count - 2 bytes
        int16_t channelCount = 1;
        fputc((channelCount & 0xFF), outFile);
        fputc((channelCount >> 8) & 0xFF, outFile);

        //    d. Sample rate - 4 bytes
        fputc((DAC_SAMPLE_RATE_HZ) & 0xFF, outFile);
        fputc((DAC_SAMPLE_RATE_HZ >> 8) & 0xFF, outFile);
        fputc((DAC_SAMPLE_RATE_HZ >> 16) &  0xFF, outFile);
        fputc((DAC_SAMPLE_RATE_HZ >> 24) & 0xFF, outFile);

        int16_t bitsPerSample = 8;
        int16_t bytesPerBlock = channelCount * bitsPerSample / 8;
        int32_t bytesPerSecond = DAC_SAMPLE_RATE_HZ * bytesPerBlock;

        //    e. Bytes per second - 4 bytes
        fputc((bytesPerSecond) & 0xFF, outFile);
        fputc((bytesPerSecond >> 8) & 0xFF, outFile);
        fputc((bytesPerSecond >> 16) &  0xFF, outFile);
        fputc((bytesPerSecond >> 24) & 0xFF, outFile);

        //    f. Bytes per block - 2 bytes
        fputc((bytesPerBlock & 0xFF), outFile);
        fputc((bytesPerBlock >> 8) & 0xFF, outFile);

        //    g. Bits per sample - 2 bytes
        fputc((bitsPerSample & 0xFF), outFile);
        fputc((bitsPerSample >> 8) & 0xFF, outFile);

        // 3. Write data chunk
        //    a. Data chunk ID - 4 bytes
        fputs("data", outFile);

        //    b. Size (also 0 for now) - 4 bytes
        // Save the position so it's easy to come back and overwrite this
        int dataSizeOffset = ftell(outFile);
        fputc(0, outFile);
        fputc(0, outFile);
        fputc(0, outFile);
        fputc(0, outFile);

        int dataOffset = ftell(outFile);
        int32_t dataBytes;

        midiPlayer_t player = {0};
        midiPlayerInit(&player);
        int32_t headroom = player.headroom;
        int tries = 1;

        while (true)
        {
            // Now, use the player to actually play and record the song
            midiPlayerReset(&player);
            midiSetFile(&player, &tmpMidi);
            midiGmOn(&player);
            player.loop = false;
            player.songFinishedCallback = renderDoneCallback;

            midiPause(&player, false);

            int32_t maxSample = 0;
            int32_t sample;

            renderComplete = false;
            dataBytes = 0;

            int clipped = 0;

            uint8_t buffer[4096];
            while (!renderComplete)
            {
                int i;
                for (i = 0; i < sizeof(buffer) && !renderComplete; i++)
                {
                    sample = midiPlayerStep(&player);
                    if (-sample > maxSample)
                    {
                        maxSample = -sample;
                    }
                    else if (sample > maxSample)
                    {
                        maxSample = sample;
                    }

                    sample *= headroom;
                    sample >>= 16;

                    if (sample < -128)
                    {
                        clipped++;
                        buffer[i] = 0;
                    }
                    else if (sample > 127)
                    {
                        clipped++;
                        buffer[i] = 255;
                    }
                    else
                    {
                        buffer[i] = sample + 128;
                    }
                }

                fwrite(buffer, 1, i, outFile);

                dataBytes += i;
            }

            if (clipped)
            {
                printf("Clipped %d samples at volume %d\n", clipped, headroom);

                // This is what noclip is, right?
                if (clipped > 10 && NOCLIP)
                {
                    headroom = UINT16_MAX * 128 / maxSample;
                    tries++;
                    fseek(outFile, dataOffset, SEEK_SET);
                    continue;
                }
            }

            break;
        }

        if (tries > 1)
        {
            printf("Took %d tries to get a reasonable number of clips, mix volume was %d%%\n", tries, headroom * 100 / UINT16_MAX);
        }

        if ((dataBytes % 2) != 0)
        {
            // Odd number of data bytes, write one to pad
            fputc(0, outFile);
            dataBytes++;
        }

        // Get the total file size, minus the header bytes
        int32_t waveSize = ftell(outFile) - 8;

        // Seek to the header file size field
        fseek(outFile, 4, SEEK_SET);

        // Overwrite the placeholder with the actual file size
        fputc((waveSize & 0xFF), outFile);
        fputc((waveSize >> 8) & 0xFF, outFile);
        fputc((waveSize >> 16) & 0xFF, outFile);
        fputc((waveSize >> 24) & 0xFF, outFile);

        // Seek to the data chunk size field offset
        fseek(outFile, dataSizeOffset, SEEK_SET);

        // Overwrite the placeholder with the actual data size
        fputc((dataBytes & 0xFF), outFile);
        fputc((dataBytes >> 8) & 0xFF, outFile);
        fputc((dataBytes >> 16) & 0xFF, outFile);
        fputc((dataBytes >> 24) & 0xFF, outFile);

        fclose(outFile);

        midiPlayerReset(&player);
        unloadMidiFile(&tmpMidi);

        if (artistName)
        {
            free(artistName);
        }

        if (trackName)
        {
            free(trackName);
        }

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
    strncpy(midiPathBuffer, path, sizeof(midiPathBuffer));
    midiFile = midiPathBuffer;

    if (emulatorStarted)
    {
        if (!midiInjectFile(path))
        {
            printf("Error: could not read MIDI file %s!\n", path);
        }
    }
}

bool installMacOpenFileHandler(MacOpenFileHandler* handlerRef, MacOpenFileCb callback)
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
        uninstallMacOpenFileHandler(handlerRef);
        return false;
    }

    // Install the application-level handler
    handlerRef->globalEventHandler = NewEventHandlerUPP(globalEventHandler);
    result                         = InstallApplicationEventHandler(handlerRef->globalEventHandler, 1, eventTypes, NULL,
                                                                    &handlerRef->globalEventHandlerRef);

    if (result != noErr)
    {
        printf("Failed to install global event handler\n");
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
