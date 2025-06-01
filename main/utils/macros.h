#ifndef _MACROS_H_
#define _MACROS_H_

/**
 * @brief Clamp a number between an upper and lower bound
 *
 * @param a A number to clamp between an upper and lower bound
 * @param l The lower bound, inclusive
 * @param u The upper bound, inclusive
 * @return The clamped number
 */
#define CLAMP(a, l, u) ((a) < l ? l : ((a) > u ? u : (a)))

/**
 * @brief Find the smaller of two numbers
 *
 * @param a A number to compare
 * @param b Another number to compare
 * @return The smaller of the two numbers
 */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/**
 * @brief Find the larger of two numbers
 *
 * @param a A number to compare
 * @param b Another number to compare
 * @return The larger of the two numbers
 */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/**
 * @brief Find the absolute value of a number
 *
 * @param a A number to find the absolute value of
 * @return The absolute value fo the number
 */
#define ABS(a) (((a) < (0)) ? -(a) : (a))

/**
 * @brief Return the number of elements in a fixed length array. This does not work for pointers.
 *
 * See https://stackoverflow.com/a/19455169
 *
 * @param arr An array to find the number of elements in
 * @return The the number of elements in an array (not the byte size!)
 */
#ifdef __APPLE__
    /* Force a compilation error if condition is true, but also produce a
        result (of value 0 and type size_t), so the expression can be used
        e.g. in a structure initializer (or where-ever else comma expressions
        aren't permitted). */
    #define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int : -!!(e); }))

    /// Helper macro to determine the number of elements in an array. Should not be used directly
    #define __must_be_array(a) BUILD_BUG_ON_ZERO(__builtin_types_compatible_p(typeof(a), typeof(&(a)[0])))

    #define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))
#else
    /// Helper macro to determine the number of elements in an array. Should not be used directly
    #define IS_ARRAY(arr) ((void*)&(arr) == &(arr)[0])

    /// Helper macro to determine the number of elements in an array. Should not be used directly
    #define STATIC_EXP(e) (0 * sizeof(struct { int ARRAY_SIZE_FAILED : (2 * (e)-1); }))

    #define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + STATIC_EXP(IS_ARRAY(arr)))
#endif

/**
 * @brief Returns (a + b) % d, but with negative values converted to equivalent positive values.
 * The resulting value will always be in the range [0, d), assuming d > 0.
 *
 * The first modulo, (b % d) will return e.g. -90 for (-270 % 360)
 *
 * @param a One number to sum
 * @param b Another number to sum
 * @param d The number to mod the sum by
 * @return (a + b) % d
 */
#define POS_MODULO_ADD(a, b, d) ((a + (b % d) + d) % d)

/**
 * @brief Run timer_code every period, using tracking it with timer
 *
 * @param timer The accumulator variable, must persist between calls
 * @param period The period at which timer_code should be run
 * @param elapsed The time elapsed since this was last called
 * @param timer_code The code to execute every period
 */
#define RUN_TIMER_EVERY(timer, period, elapsed, timer_code) \
    do                                                      \
    {                                                       \
        timer += (elapsed);                                 \
        while (timer > (period))                            \
        {                                                   \
            timer -= (period);                              \
            {                                               \
                timer_code                                  \
            }                                               \
        }                                                   \
    } while (0)

#define NUM_FRAME_TIMES 60
#define DRAW_FPS_COUNTER(font)                                                \
    do                                                                        \
    {                                                                         \
        static uint32_t frameTimesIdx               = 0;                      \
        static uint32_t frameTimes[NUM_FRAME_TIMES] = {0};                    \
                                                                              \
        int32_t startIdx  = (frameTimesIdx + 1) % NUM_FRAME_TIMES;            \
        uint32_t tElapsed = frameTimes[frameTimesIdx] - frameTimes[startIdx]; \
        if (0 != tElapsed)                                                    \
        {                                                                     \
            uint32_t fps = (1000000 * NUM_FRAME_TIMES) / tElapsed;            \
            char tmp[16];                                                     \
            snprintf(tmp, sizeof(tmp) - 1, "%" PRIu32, fps);                  \
            fillDisplayArea(34, 1, 51, 3 + font.height, c000);                \
            drawText(&font, c555, tmp, 35, 2);                                \
        }                                                                     \
        frameTimesIdx             = (frameTimesIdx + 1) % NUM_FRAME_TIMES;    \
        frameTimes[frameTimesIdx] = esp_timer_get_time();                     \
    } while (0)

#endif

#if defined(__XTENSA__)
// Memory tagging is not supported on real hardware
#define heap_caps_malloc_tag(s, c, t)     heap_caps_malloc(s, c)
#define heap_caps_calloc_tag(n, s, c, t)  heap_caps_calloc(n, s, c)
#define heap_caps_realloc_tag(p, s, c, t) heap_caps_realloc(p, s, c)
#define heap_caps_free_tag(p)             heap_caps_free(p)
#endif
