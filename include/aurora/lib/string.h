#ifndef AURORA_LIB_STRING_H
#define AURORA_LIB_STRING_H

#include <aurora/core/types.h>

void *aurora_memset(void *destination, int value, size_t size);
void *aurora_memcpy(void *destination, const void *source, size_t size);
size_t aurora_strlen(const char *text);
size_t aurora_strnlen(const char *text, size_t max_length);
size_t aurora_strlcpy(char *destination, const char *source, size_t destination_size);

#endif
