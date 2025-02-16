#pragma once

#include <stddef.h>

void deleteNvsCookie(void);
size_t getNvsCookieSize(void);
size_t readNvsCookie(void* buf, size_t size);
