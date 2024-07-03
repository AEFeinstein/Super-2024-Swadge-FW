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
        pid_t pid = getpid();
        seeded    = true;
        srand(time(NULL) ^ pid);
    }
    return rand();
}
