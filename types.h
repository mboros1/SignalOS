#ifndef SIGNALOS_TYPES_H
#define SIGNALOS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
// #include <inttypes.h>
#include <stddef.h>


typedef int pid_t;

typedef __builtin_va_list va_list;
#define va_start(val, last) __builtin_va_start(val, last)
#define va_arg(val, type) __builtin_va_arg(val, type)
#define va_end(val) __builtin_va_end(val)

#define __section(x) __attribute__((section(x)))
#define __no_asan    __attribute__((no_sanitize_address))
#define __noinline   __attribute__((noinline))
#define __always_inline static inline __attribute__((always_inline))

#endif /* !SIGNALOS_TYPES_H */
