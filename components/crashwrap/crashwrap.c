//==============================================================================
// Includes
//==============================================================================

#include "crashwrap.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include "esp_private/panic_internal.h"
#include "esp_debug_helpers.h"
#include "esp_memory_utils.h"
#include "esp_cpu_utils.h"
#include <xtensa_context.h>
#include "hdw-nvs.h"

//==============================================================================
// Function Prototypes
//==============================================================================

void __real_esp_panic_handler(void*);
void IRAM_ATTR __attribute__((noreturn, no_sanitize_undefined)) __real_panic_abort(const char*);

//==============================================================================
// Variables
//==============================================================================

static const char* panicreason   = 0;
static const char crashwrapTag[] = "crashwrap";

static const char crashdesc[]   = "crashdesc";
static const char crashreason[] = "crashreason";
static const char crashframe[]  = "crashframe";
static const char crashpanic[]  = "crashpanic";
static const char crashstack[]  = "crashstack";

#define STACKPAIRS 16

//==============================================================================
// Functions
//==============================================================================

/**
 * This function handles panics and writes the crash info to NVS
 *
 * This function is an IRAM function for speed
 *
 * @param info A panic_info_t* containing things like a name, description, a reason, and a frame
 */
void IRAM_ATTR __wrap_esp_panic_handler(void* info)
{
    esp_rom_printf("Panic has been triggered\n");

    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE_NAME, NVS_READWRITE, &handle) == ESP_OK)
    {
        // Write and hope for the best.
        nvs_set_blob(handle, crashwrapTag, info, 36);
        if (((panic_info_t*)info)->description)
        {
            int len = strlen(((panic_info_t*)info)->description);
            nvs_set_blob(handle, crashdesc, ((panic_info_t*)info)->description, len);
        }
        if (((panic_info_t*)info)->reason)
        {
            int len = strlen(((panic_info_t*)info)->reason);
            nvs_set_blob(handle, crashreason, ((panic_info_t*)info)->reason, len);
        }

        const XtExcFrame* fr = ((panic_info_t*)info)->frame;
        if (fr)
        {
            nvs_set_blob(handle, crashframe, fr, sizeof(XtExcFrame));
        }

        nvs_set_blob(handle, crashpanic, panicreason ? panicreason : "", panicreason ? strlen(panicreason) : 0);

        // First, wipe the stack trace, becuase it might not work.
        uint32_t epc_sp_pairs[STACKPAIRS * 2] = {0};
        nvs_set_blob(handle, crashstack, epc_sp_pairs, sizeof(epc_sp_pairs));
        nvs_close(handle);

        // Try to actually get a stack trace.
        if (nvs_open(NVS_NAMESPACE_NAME, NVS_READWRITE, &handle) == ESP_OK)
        {
            esp_backtrace_frame_t stk_frame = {0};
            esp_backtrace_get_start(&(stk_frame.pc), &(stk_frame.sp), &(stk_frame.next_pc));

            uint32_t epc_sp_pairs[STACKPAIRS * 2] = {0};
            for (int i = 0; i < STACKPAIRS; i++)
            {
                epc_sp_pairs[i * 2 + 0] = esp_cpu_process_stack_pc(stk_frame.pc);
                epc_sp_pairs[i * 2 + 1] = stk_frame.sp;

                // Use frame(i-1)'s BS area located below frame(i)'s sp to get frame(i-1)'s sp and frame(i-2)'s pc
                void* base_save = (void*)stk_frame.sp; // Base save area consists of 4 words under SP
                stk_frame.pc    = stk_frame.next_pc;
                // If next_pc = 0, indicates frame(i-1) is the last frame on the stack
                stk_frame.next_pc = *((uint32_t*)(base_save - 16));
                stk_frame.sp      = *((uint32_t*)(base_save - 12));

                // Return true if both sp and pc of frame(i-1) are sane, false otherwise
                int valid = (esp_stack_ptr_is_sane(stk_frame.sp)
                             && esp_ptr_executable((void*)esp_cpu_process_stack_pc(stk_frame.pc)));

                if (!valid)
                {
                    break;
                }
            }

            nvs_set_blob(handle, crashstack, epc_sp_pairs, sizeof(epc_sp_pairs));

            nvs_close(handle);
        }
    }

    __real_esp_panic_handler(info);
}

/**
 * This is a wrapper for the system's __real_panic_abort() which also saves the detail in a static variable so
 * that it can be referenced in __wrap_esp_panic_handler()
 */
void IRAM_ATTR __attribute__((noreturn, no_sanitize_undefined)) __wrap_panic_abort(const char* details)
{
    panicreason = details;
    __real_panic_abort(details);
}

/**
 * This function both checks for prior crash info written to NVS.
 * It must be called after initializing NVS and should be called as soon as possible after booting.
 * If there is crash info, it will be printed
 *
 * This function doesn't seem to manually install the crash handler, so I bet the handler is installed by virtue of the
 * function names __wrap_panic_abort() and __wrap_esp_panic_handler()
 */
void checkAndInstallCrashwrap(void)
{
    panic_info_t cd = {0};
    char buffer[768];
    nvs_handle_t handle;
    size_t length = sizeof(cd);

    ESP_LOGI(crashwrapTag, "Crashwrap Install");

    if (nvs_open(NVS_NAMESPACE_NAME, NVS_READONLY, &handle) != ESP_OK)
    {
        ESP_LOGW(crashwrapTag, "Couldnot find 'storage' NVS");
        return;
    }

    if (nvs_get_blob(handle, crashwrapTag, &cd, &length) == ESP_OK && length >= sizeof(cd))
    {
        ESP_LOGW(crashwrapTag, "Crashwrap length: %zu/%zu", length, sizeof(cd));
        ESP_LOGW(crashwrapTag, "Last Crash: ADDR: %08" PRIx32 " FRAME: %08" PRIx32 " EXCEPTION: %d", (uint32_t)cd.addr,
                 (uint32_t)cd.frame, cd.exception);
    }

    length = sizeof(buffer) - 1;
    if (nvs_get_blob(handle, crashreason, buffer, &length) == ESP_OK && length > 0)
    {
        buffer[length] = 0;
        ESP_LOGW(crashwrapTag, "Reason: %s", buffer);
    }

    length = sizeof(buffer) - 1;
    if (nvs_get_blob(handle, crashdesc, buffer, &length) == ESP_OK && length > 0)
    {
        buffer[length] = 0;
        ESP_LOGW(crashwrapTag, "Description: %s", buffer);
    }

    length = sizeof(buffer) - 1;
    if (nvs_get_blob(handle, crashpanic, buffer, &length) == ESP_OK && length > 0)
    {
        buffer[length] = 0;
        ESP_LOGW(crashpanic, "Panic: %s", buffer);
    }

    XtExcFrame fr;
    length = sizeof(fr);
    if (nvs_get_blob(handle, crashframe, &fr, &length) == ESP_OK && length >= sizeof(fr))
    {
        ESP_LOGW(crashwrapTag,
                 "EXIT: 0x%08" PRIx32 " / PC: 0x%08" PRIx32 " / PS: 0x%08" PRIx32 " / A0: 0x%08" PRIx32
                 " / A1: 0x%08" PRIx32 " / SAR: 0x%08" PRIx32 " / EXECCAUSE: 0x%08" PRIx32 " / "
                 "EXECVADDR: 0x%08" PRIx32 "",
                 (uint32_t)fr.exit, (uint32_t)fr.pc, (uint32_t)fr.ps, (uint32_t)fr.a0, (uint32_t)fr.a1,
                 (uint32_t)fr.sar, (uint32_t)fr.exccause, (uint32_t)fr.excvaddr);

        uint32_t epc_sp_pairs[STACKPAIRS * 2] = {0};
        length                                = sizeof(epc_sp_pairs);
        if (nvs_get_blob(handle, crashstack, epc_sp_pairs, &length) == ESP_OK && length > 0)
        {
            ESP_LOGW(crashwrapTag, "Backtrace:");
            int i;
            char fulllog[1024];
            char* flend = fulllog;
            flend += sprintf(flend, "xtensa-esp32s2-elf-addr2line -e build/swadge2024.elf");
            for (i = 0; i < STACKPAIRS; i++)
            {
                if (sizeof(fulllog) + fulllog - flend > 24 && epc_sp_pairs[i * 2 + 0])
                {
                    flend += sprintf(flend, " 0x%08lx:0x%08lx", epc_sp_pairs[i * 2 + 0], epc_sp_pairs[i * 2 + 1]);
                }
            }
            ESP_LOGW(crashwrapTag, "%s", fulllog);
        }
        else
        {
            // Could not get full stack trace, do the best we can.
            ESP_LOGW(crashwrapTag, "xtensa-esp32s2-elf-addr2line -e build/swadge2024.elf 0x%08lx:0x%08lx",
                     (uint32_t)fr.pc, (uint32_t)fr.ps);
        }
    }

    nvs_close(handle);
}

#if 0
/*
	Neat stuff:
		* pm_noise_check_process
*/
int __real_ieee80211_ioctl(uint32_t a, uint32_t b, uint32_t c );
int __wrap_ieee80211_ioctl( uint32_t a, uint32_t b, uint32_t c )
{
	uint32_t * l = (uint32_t*)a;
	ESP_LOGE(crashwrapTag, "8ii: %08x { %08x %08x %08x %08x } %08x %08x", a, l[0], l[1], l[2], l[3], b, c );
	return __real_ieee80211_ioctl( a, b, c );
}


int __real_esp_wifi_internal_ioctl(uint32_t a, uint32_t b, uint32_t c );
int __wrap_esp_wifi_internal_ioctl( uint32_t a, uint32_t b, uint32_t c )
{
	ESP_LOGE(crashwrapTag, "wii: %08x %08x %08x", a, b, c );
	return __real_esp_wifi_internal_ioctl( a, b, c );
}



int __real_nvs_set_i8(uint32_t a, uint32_t b, uint32_t c );
int __wrap_nvs_set_i8(uint32_t a, uint32_t b, uint32_t c )
{
	ESP_LOGE(crashwrapTag, "nvs_set_i8: %08x %08x %08x", a, b, c );
	uint32_t * t = (uint32_t*)b;
	ESP_LOGE(crashwrapTag, "nvs_set_i8: %08x %08x (%08x %08x %08x)  %08x", a, b, t[0], t[1], t[2], c );
	
	return __real_nvs_set_i8( a, b, c );
}




int __real_nvs_set_u8(uint32_t a, uint32_t b, uint32_t c );
int __wrap_nvs_set_u8(uint32_t a, uint32_t b, uint32_t c )
{
	ESP_LOGE(crashwrapTag, "nvs_set_u8: %08x %08x %08x", a, b, c );
	uint32_t * t = (uint32_t*)b;
	ESP_LOGE(crashwrapTag, "nvs_set_u8: %08x %08x (%08x %08x %08x)  %08x", a, b, t[0], t[1], t[2], c );
	
	return __real_nvs_set_u8( a, b, c );
}


int __real_nvs_get_u8(uint32_t a, uint32_t b, uint32_t c );
int __wrap_nvs_get_u8(uint32_t a, uint32_t b, uint32_t c )
{
	ESP_LOGE(crashwrapTag, "nvs_get_u8: %08x %s %08x", a, (char*)b, c );
	return __real_nvs_get_u8( a, b, c );
}

esp_err_t __real_nvs_open(const char* name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle);
esp_err_t __wrap_nvs_open(const char* name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle)
{
	ESP_LOGE(crashwrapTag, "__real_nvs_open: %s %d %p", name, open_mode, out_handle );
	if( name == (char*)7 )
	{
		void esp_phy_erase_cal_data_in_nvs();
		esp_phy_erase_cal_data_in_nvs();
	}
	return __real_nvs_open(name, open_mode, out_handle);
}

#endif
