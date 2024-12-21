#pragma once

#include <common/defines.h>

// Open a UDP socket for networking.
// return the file descriptor.
extern int sys_socket(u32 raddr, u16 lport, u16 rport);
