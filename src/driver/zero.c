#include <driver/zero.h>
#include <common/string.h>

isize null_read(u8 *dst, usize count)
{
    // Immediate EOF
    return 0;
}

isize zero_read(u8 *dst, usize count)
{
    // set to zero.
    memset(dst, 0, count);
    return count;
}

isize null_write(u8 *dst, usize count)
{
    // discard write.
    return count;
}
