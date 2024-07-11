#ifndef SIGNALOS_TYPES_H
#define SIGNALOS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
// #include <inttypes.h>
#include <stddef.h>


typedef int pid_t;


#define __section(x) __attribute__((section(x)))
#define __no_asan    __attribute__((no_sanitize_address))
#define __noinline   __attribute__((noinline))
#define __always_inline static inline __attribute__((always_inline))

#endif /* !SIGNALOS_TYPES_H */
