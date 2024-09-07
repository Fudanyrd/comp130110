#pragma once
#ifndef __FDUTIL_ARITHM_
#define __FDUTIL_ARITHM_

/** Integer arithmetics */

/** Round down */
#define ROUND_DOWN(a, b) ((a) - ((a) % (b)))
/** Round up */
#define ROUND_UP(a, b) ROUND_DOWN(((a) + (b) - 1), b)

#endif // __FDUTIL_ARITHM_
