#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <esp_random.h>

static bool seeded = false;

uint32_t esp_random(void)
{
    if (!seeded)
    {
#ifdef __wasm__
        int pid = 0x1337;
#else
        pid_t pid = getpid();
#endif
        seeded    = true;
        srand(time(NULL) ^ pid);
    }
    return rand();
}
