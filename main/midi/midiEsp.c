#include <inttypes.h>
#include <string.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "midiFileParser.h"
#include "heatshrink_helper.h"
#include "cnfs.h"

bool loadMidiFile(const char* name, midiFile_t* file, bool spiRam)
{
    uint32_t size;
    size_t raw_size;
    uint8_t* data = cnfsReadFile(name, &raw_size, spiRam);

    if (NULL != data)
    {
        if (raw_size < sizeof(midiHeader) || memcmp(data, midiHeader, sizeof(midiHeader)))
        {
            // This is not a MIDI file! Try to decompress
            if (heatshrinkDecompress(NULL, &size, data, (uint32_t)raw_size))
            {
                // Size was read successfully, allocate the non-compressed buffer
                uint8_t* decompressed = heap_caps_malloc(size, spiRam ? MALLOC_CAP_SPIRAM : 0);
                if (decompressed && heatshrinkDecompress(decompressed, &size, data, (uint32_t)raw_size))
                {
                    // Success, free the raw data
                    heap_caps_free(data);
                    data = decompressed;
                }
                else
                {
                    heap_caps_free(decompressed);
                    heap_caps_free(data);
                    return false;
                }
            }
            else
            {
                ESP_LOGE("MIDIFileParser", "Song %s could not be decompressed!", name);
                heap_caps_free(data);
                return false;
            }
        }
        else
        {
            ESP_LOGI("MIDIFileParser", "Song %s is loaded uncompressed", name);
            size = (uint32_t)raw_size;
        }
    }

    ESP_LOGI("MIDIFileParser", "Song %s has %" PRIu32 " bytes", name, size);

    if (data != NULL)
    {
        if (loadMidiData(data, size, file))
        {
            return true;
        }
        else
        {
            // Need to free data
            heap_caps_free(data);
            return false;
        }
    }
    else
    {
        return false;
    }
}
