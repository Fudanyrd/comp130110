// Implementation of zero device and null device.
#pragma once
#ifndef _DRIVER_ZERO_
#define _DRIVER_ZERO_

#include <common/defines.h>

/** Read zero. */
extern isize zero_read(u8 *dst, usize count);

/** Read null.  */
extern isize null_read(u8 *dst, usize count);

/** Write null/zero. */
extern isize null_write(u8 *dst, usize count);

#endif // _DRIVER_ZERO_
