#ifndef __ASSERT_H_
#define __ASSERT_H_ 1

#include "stdio.h"
#include "stdlib.h"
#define Assert(cond)                                                   \
    do {                                                               \
        if (!cond) {                                                   \
            printf("[%s:%d] assertion failure\n", __FILE__, __LINE__); \
            Exit(1);                                                   \
        }                                                              \
    } while (0)

#define Static_assert(cond) \
    do {                    \
        switch (0) {        \
        case (0):           \
        case (cond):        \
            break           \
        }                   \
    } while (0)

#endif // __ASSERT_H_ 1
