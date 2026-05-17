#ifndef CORE_LIBK_STRING_H
#define CORE_LIBK_STRING_H

#include <stddef.h>
#include <stdint.h>

size_t kstrlen(const char* text);
int kstrcmp(const char* lhs, const char* rhs);
int kstrncmp(const char* lhs, const char* rhs, size_t n);
char* kstrcpy(char* dst, const char* src);
char* kstrncpy(char* dst, const char* src, size_t n);
char* kstrcat(char* dst, const char* src);
const char* kstrstr(const char* haystack, const char* needle);
int katoi(const char* text);
uint64_t katoi_hex(const char* text);
void* kmemset(void* dst, int value, size_t size);
void* kmemcpy(void* dst, const void* src, size_t size);
int kmemcmp(const void* lhs, const void* rhs, size_t size);

#endif
