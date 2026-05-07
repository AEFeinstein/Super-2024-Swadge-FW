#include "amy.h"

void amy_update_tasks() {
}

void amy_platform_init() {
}

void amy_platform_deinit() {
}

size_t amy_i2s_write(const uint8_t *buffer, size_t nbytes) {
    return 0;
}

int16_t *amy_render_audio() {
    return amy_simple_fill_buffer();
}