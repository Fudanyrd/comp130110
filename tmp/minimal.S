.globl _start

#include <syscall.h>
_start:
    mov x0, #1  // write to stdout
    adr x1, st
    mov x2, #0x11
    mov w8, #SYS_write // call write
    svc #0
    mov w8, #SYS_exit  // call exit
    mov x0, #0
    svc #0
st:
  .string "Hello, OS World!\n"
ed:
