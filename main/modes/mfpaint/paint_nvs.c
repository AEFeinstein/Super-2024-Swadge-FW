#include "paint_nvs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#include "settingsManager.h"
#include "esp_heap_caps.h"
#include "hdw-nvs.h"
#include "macros.h"

#include "paint_common.h"
#include "paint_draw.h"
#include "paint_ui.h"
#include "paint_util.h"
#include "paint_canvas.h"

#define PAINT_PARAM(m, x, k, d) \
    static const settingParam_t paint##k##Param = {.min = m, .max = x, .def = d, .key = "mfp." #k}

#define PAINT_BOOL_PARAM(k, d) PAINT_PARAM((0), (1), k, d)

static const char KEY_PAINT_LAST_SLOT[] = "paint_last";

PAINT_BOOL_PARAM(EnableLeds, true);

static int32_t paintReadParam(const settingParam_t* param);
static bool paintWriteParam(const settingParam_t* param, int32_t val);

const settingParam_t* paintGetEnableLedsBounds(void)
{
    return &paintEnableLedsParam;
}

bool paintGetEnableLeds(void)
{
    return paintReadParam(&paintEnableLedsParam);
}

void paintSetEnableLeds(bool enableLeds)
{
    paintWriteParam(&paintEnableLedsParam, enableLeds);
}

static int32_t paintReadParam(const settingParam_t* param)
{
    int32_t out;
    // Wrap this just in case we need to do stuff later
    if (!readNvs32(param->key, &out))
    {
        // Set the default value if the read failed
        out = param->def;
    }

    return out;
}

static bool paintWriteParam(const settingParam_t* param, int32_t val)
{
    return writeNvs32(param->key, CLAMP(val, param->min, param->max));
}

bool paintGetAnySlotInUse(void)
{
    return nvsNamespaceInUse(PAINT_NS_DATA);
}

bool paintSaveNamed(const char* name, const paintCanvas_t* canvas)
{
    wsg_t tmpWsg;
    tmpWsg.px = heap_caps_malloc(sizeof(paletteColor_t) * canvas->w * canvas->h, MALLOC_CAP_SPIRAM);
    tmpWsg.w  = canvas->w;
    tmpWsg.h  = canvas->h;

    if (NULL == tmpWsg.px)
    {
        PAINT_LOGE("Not able to allocate pixels for saving");
        return false;
    }

    // Read pixels from the screen into the temp WSG
    for (uint16_t r = 0; r < canvas->h; ++r)
    {
        for (uint16_t c = 0; c < canvas->w; ++c)
        {
            paletteColor_t col;
            if (canvas->buffered && canvas->buffer)
            {
                if (((r * canvas->w + c) % 2) == 0)
                {
                    col = canvas->palette[(canvas->buffer[(r * canvas->w + c) / 2] >> 4) & 0x0F];
                }
                else
                {
                    col = canvas->palette[(canvas->buffer[(r * canvas->w + c) / 2] >> 0) & 0x0F];
                }
            }
            else
            {
                col = getPxTft(canvas->x + c * canvas->xScale, canvas->y + r * canvas->yScale);
            }

            tmpWsg.px[r * canvas->w + c] = col;
        }
    }

    bool result = saveWsgNvs(PAINT_NS_DATA, name, &tmpWsg);
    free(tmpWsg.px);

    return result && writeNamespaceNvsBlob(PAINT_NS_PALETTE, name, canvas->palette, sizeof(canvas->palette));
}

bool paintLoadNamed(const char* name, paintCanvas_t* canvas)
{
    wsg_t tmpWsg;
    bool result = loadWsgNvs(PAINT_NS_DATA, name, &tmpWsg, true);

    if (result)
    {
        canvas->w = tmpWsg.w;
        canvas->h = tmpWsg.h;

        // Canvas is now kind of optional at this point
        size_t length = PAINT_MAX_COLORS;
        if (!readNamespaceNvsBlob(PAINT_NS_PALETTE, name, canvas->palette, &length))
        {
            paintRebuildPalette(canvas->palette, tmpWsg.px, canvas->w, canvas->h);
            PAINT_LOGW("No palette found for image %s, that's weird right?", name);
        }

        if (canvas->buffered)
        {
            if (canvas->buffer)
            {
                free(canvas->buffer);
            }
            canvas->buffer = malloc(paintGetStoredSize(canvas));
            paintSerializeWsgPalette(canvas->buffer, &tmpWsg, canvas->palette);
        }
        else
        {
            for (uint16_t y = 0; y < canvas->h; ++y)
            {
                for (uint16_t x = 0; x < canvas->w; ++x)
                {
                    setPxScaled(x, y,
                                cTransparent == tmpWsg.px[y * canvas->w + x] ? c555 : tmpWsg.px[y * canvas->w + x],
                                canvas->x, canvas->y, canvas->xScale, canvas->yScale);
                }
            }
        }
        freeWsg(&tmpWsg);
    }

    return result;
}

void paintDeleteNamed(const char* name)
{
    eraseNamespaceNvsKey(PAINT_NS_DATA, name);
    eraseNamespaceNvsKey(PAINT_NS_PALETTE, name);
}

bool paintSlotExists(const char* name)
{
    size_t blobSize;
    return readNamespaceNvsBlob(PAINT_NS_DATA, name, NULL, &blobSize);
}

bool paintGetLastSlot(char* out)
{
    size_t len;
    if (readNvsBlob(KEY_PAINT_LAST_SLOT, NULL, &len))
    {
        if (len <= 17)
        {
            return readNvsBlob(KEY_PAINT_LAST_SLOT, out, &len);
        }
    }

    return false;
}

void paintSetLastSlot(const char* name)
{
    writeNvsBlob(KEY_PAINT_LAST_SLOT, name, strlen(name) + 1);
}
