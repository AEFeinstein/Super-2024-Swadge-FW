//==============================================================================
// Includes
//==============================================================================

#include "amy_test.h"
#include "amy.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static void amyEnterMode(void);
static void amyExitMode(void);
static void amyMainLoop(int64_t elapsedUs);
static void amyAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void amyBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void amyDacCallback(uint8_t* samples, int16_t len);
static void amyAddToSwadgePassPacket(struct swadgePassPacket* packet);

static void example_ks(int note, int velocity);
static void example_sine(uint32_t start);
static void example_midi_note(int note, int velocity);

//==============================================================================
// Const Data
//==============================================================================

static const char amyModeName[] = "AMY";

static const trophySettings_t amyTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = amyModeName,
};

static const trophyDataList_t amyTrophyData = {
    .settings = &amyTrophySettings,
    .list     = NULL,
    .length   = 0,
};

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t amyTestMode = {
    .modeName                 = amyModeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = true,
    .overrideSelectBtn        = false,
    .fnEnterMode              = amyEnterMode,
    .fnExitMode               = amyExitMode,
    .fnMainLoop               = amyMainLoop,
    .fnAudioCallback          = amyAudioCallback,
    .fnBackgroundDrawCallback = amyBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = amyDacCallback,
    .fnAddToSwadgePassPacket  = amyAddToSwadgePassPacket,
    .trophyData               = &amyTrophyData,
};

static uint8_t amyQueue[DAC_BUF_SIZE * 8] = {0};
static uint32_t aqHead = 0;
static uint32_t aqTail = 0;

//==============================================================================
// Functions
//==============================================================================

// Play a KS tone
void example_ks(int note, int velocity) {
    amy_event e = amy_default_event();

    e.velocity = velocity;
    e.wave = KS;
    e.feedback = 0.996f;
    e.preset = 15;
    e.osc = 0;
    e.midi_note = note;
    amy_add_event(&e);
}

// make a 440hz sine
void example_sine(uint32_t start) {
    amy_event e = amy_default_event();
    e.time = start;
    e.freq_coefs[0] = 440;
    e.wave = SINE;
    e.velocity = 1;
    amy_add_event(&e);
}

void example_midi_note(int note, int velocity) {
	// Will play MIDI note 50 on patch 130
	amy_event e = amy_default_event();
	e.osc = 0;
	e.patch_number = 130;
	e.velocity = velocity;
	e.midi_note = note;
	e.voices[0] = note-50; // TODO adjust for proper polyphony
	e.volume = 1;
	amy_add_event(&e);
}

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
void amyEnterMode(void)
{
	static const char AMY_TAG[] = "AMY";
	setFrameRateUs(16667);
	switchToSpeaker();

	amy_config_t amy_config = amy_default_config();
    amy_config.audio = AMY_AUDIO_IS_NONE;
    amy_config.features.default_synths = 0;
    amy_start(amy_config);
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
void amyExitMode(void)
{
    amy_stop();
}

/**
 * @brief This function is called from the main loop. It's pretty quick, but the timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
void amyMainLoop(int64_t elapsedUs)
{
    // Check all queued button events
    buttonEvt_t evt;
    while(checkButtonQueueWrapper(&evt))
    {
		int mod = -1;
		switch(evt.button){
			default:
				mod = -1;
				break;
			case PB_DOWN:
				mod = 0;
				break;
			case PB_LEFT:
				mod = 1;
				break;
			case PB_RIGHT:
				mod = 2;
				break;
			case PB_UP:
				mod = 3;
				break;
			case PB_B:
				mod = 4;
				break;
			case PB_A:
				mod = 5;
				break;
		}

		if(evt.down)
        {
			// example_ks(50 + mod, 1);
			example_midi_note(50 + mod, 1);
        }
		else
		{
			// example_ks(50 + mod, 0);
			example_midi_note(50 + mod, 0);
		}
    }
    
    // Check if the touch area is touched, and print values if it is
    int32_t phi, r, intensity;
    if (getTouchJoystick(&phi, &r, &intensity))
    {
        // printf("touch center: %" PRIu32 ", intensity: %" PRIu32 ", intensity %" PRIu32 "\n", phi, r, intensity);
    }
    else
    {
        // printf("no touch\n");
    }

    // Recommended to call amy_update() which calls:
    // - amy_update_tasks()
    // - amy_render_audio() - This gets stuck because libminiaudio-audio isn't being run
    // - amy_global.config.write_samples_fn()
    // Instead, basically the same thing happens in amyDacCallback

	clearPxTft();
	DRAW_FPS_COUNTER((*getSysFont()));
}

/**
 * @brief This function is called whenever audio samples are read from the microphone (ADC) and are ready for
 * processing. Samples are read at 8KHz. If this function is not NULL, then readBattmon() will not work
 *
 * @param samples A pointer to 12 bit audio samples
 * @param sampleCnt The number of samples read
 */
void amyAudioCallback(uint16_t* samples, uint32_t sampleCnt)
{
    ;
}

/**
 * @brief This function is called when the display driver wishes to update a section of the display.
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param upNum update number denominator
 */
void amyBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    ;
}

/**
 * @brief This function is called to fill sample buffers for the DAC. If this is NULL, then
 * globalMidiPlayerFillBuffer() will be used instead to fill sample buffers
 */
static void amyDacCallback(uint8_t* samples, int16_t len)
{
	for (int16_t sIdx = 0; sIdx < len; sIdx++)
	{
		// If the input queue is empty
		if(aqHead == aqTail)
		{
			// Render audio
			amy_update_tasks();
			const int16_t* block = amy_simple_fill_buffer();

			// Fill queue
			for(int16_t bIdx = 0; bIdx < AMY_BLOCK_SIZE * AMY_NCHANS; bIdx += AMY_NCHANS)
			{
				// Add to queue
				amyQueue[aqTail] = (((int32_t)block[bIdx]) + 32768) / 256;
				aqTail++;
				aqTail = (aqTail < sizeof(amyQueue) ? aqTail : 0);
			}
		}

		// Write input queue to output
	    samples[sIdx] = amyQueue[aqHead];
        aqHead++;
        aqHead = (aqHead < sizeof(amyQueue) ? aqHead : 0);
	}
}

/**
 * @brief This function is called to fill in a SwadgePass packet with mode-specific data. The Swadge mode should
 * only fill in it's relevant data and not touch other mode's data.
 *
 * @warning This function will be called when the mode is not initialized or running, so it MUST NOT rely on memory
 * allocated or data loaded in the mode's initializer.
 *
 * @param packet The packet to fill in
 */
void amyAddToSwadgePassPacket(struct swadgePassPacket* packet)
{
    ;
}
