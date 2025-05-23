#include <stdio.h>

#include <esp_log.h>

#include <esp_timer.h>

static const char levelChars[] = {
    'N', // ESP_LOG_NONE
    'E', // ESP_LOG_ERROR
    'W', // ESP_LOG_WARN
    'I', // ESP_LOG_INFO
    'D', // ESP_LOG_DEBUG
    'V'  // ESP_LOG_VERBOSE
};

void esp_log_write(esp_log_level_t level, const char* file, int line, const char* func, const char* tag,
                   const char* format, ...)
{
    if (level <= LOG_LOCAL_LEVEL)
    {
        char dbgStr[256];
        va_list args;
        va_start(args, format);
        vsnprintf(dbgStr, sizeof(dbgStr), format, args);
        va_end(args);

        printf("%c (%" PRId64 ") %s | %s:%d - %s() | %s\n", levelChars[level], esp_timer_get_time() / 1000, tag, file,
               line, func, dbgStr);
    }
}
