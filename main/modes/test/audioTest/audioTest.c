#include "audioTest.h"
#include "esp_random.h"

static void audioTestEnterMode(void);
static void audioTestExitMode(void);
static void audioTestMainLoop(int64_t elapsedUs);

typedef struct {
    // a whole two seconds of audio!
    uint8_t audioSample[32768];
    uint32_t audioSampleLength;
} audioTestMode_t;

swadgeMode_t audioTestMode = {
    .modeName = "Audio Test",
    .wifiMode = NO_WIFI,
    .overrideUsb = false,
    .usesAccelerometer = false,
    .usesThermometer = false,
    .overrideSelectBtn = false,
    .fnEnterMode = audioTestEnterMode,
    .fnExitMode = audioTestExitMode,
    .fnMainLoop = audioTestMainLoop,
    .fnAudioCallback = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb = NULL,
    .fnEspNowSendCb = NULL,
    .fnAdvancedUSB = NULL,
    .fnDacCb = NULL,
    .fnAddToSwadgePassPacket = NULL,
    .trophyData = NULL,
};

static audioTestMode_t* at;

static void audioTestEnterMode(void)
{
    at = calloc(1, sizeof(audioTestMode_t));
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    midiPlayerReset(player);
    midiPause(player, false);
    midiGmOn(player);
    midiSetProgram(player, 15, 90); // talking sound effects arbitrarily go on channel 15 and use midi instrument 90.
    midiControlChange(player, 15, MCC_SUSTENUTO_PEDAL, 80);
    midiControlChange(player, 15, MCC_SOUND_RELEASE_TIME, 60);
}

static void audioTestExitMode(void)
{
    free(at);
    at = NULL;
}

static void audioTestMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};

    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && evt.button == PB_A)
        {
            ESP_LOGI("Audio Test", "Playing");
            midiPlayer_t* bgm = globalMidiPlayerGet(MIDI_BGM);
            // Play a random note within an octave at half velocity on channel 1
            int deepBlueseyPitches[] = {31, 34, 36, 37, 38, 41, 43, 54, 55, 50};
            uint8_t pitch            = esp_random() % 10;
            midiNoteOn(bgm, 15, deepBlueseyPitches[pitch], 0x40);
            midiNoteOff(bgm, 15, deepBlueseyPitches[pitch], 0x7F);
        }
    }
}