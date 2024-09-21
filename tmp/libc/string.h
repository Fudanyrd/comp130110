#ifndef __STRING_H_
#define __STRING_H_ 1

#include <stdint.h>
#include <stddef.h>
extern uintptr_t Strlen(const char *s);
extern void *memset(uint8_t *b, int v, size_t sz);

#endif // __STRING_H_
