.globl main
.globl _start
_start:
    mov sp, x0
    ldr x0, [sp, #0]
    mov x1, sp 
    add x1, x1, #8
    bl main
_loop:
    b _loop

.globl sys_print
sys_print:
    mov w8, #1
    svc #0
    ret

.globl sys_open
sys_open:
    mov w8, #2
    svc #0
    ret

.globl sys_close
sys_close:
    mov w8, #3
    svc #0
    ret

.globl sys_readdir
sys_readdir:
    mov w8, #4
    svc #0
    ret

.globl sys_read 
sys_read:
    mov w8, #5
    svc #0
    ret
