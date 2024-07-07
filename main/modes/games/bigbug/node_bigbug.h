#include <stdint.h>

typedef struct
{
    int32_t parent_i;
    int32_t parent_j;
    int32_t f;
    int32_t g;
    int32_t h;
    bool foreground;//True: foreground, False: midground
} bb_node_t;