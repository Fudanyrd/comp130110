.globl main
.globl _start
_start:
    ldr x0, [sp, #0]
    mov x1, sp 
    add x1, x1, #8
    # exit(main(argc, argv))
    bl main
    bl sys_exit

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

.globl sys_chdir
sys_chdir:
    mov w8, #6
    svc #0
    ret

.globl sys_mkdir
sys_mkdir:
    mov w8, #7
    svc #0
    ret

.globl sys_write
sys_write:
    mov w8, #8
    svc #0
    ret

.globl sys_unlink
sys_unlink:
    mov w8, #9
    svc #0
    ret

.globl sys_fork
sys_fork:
    mov w8, #10
    svc #0
    ret

.globl sys_exit
sys_exit:
    mov w8, #11
    svc #0
    ret

.globl sys_wait
sys_wait:
    mov w8, #12
    svc #0
    ret

.globl sys_execve
sys_execve:
    mov w8, #13
    svc #0
    ret

.globl sys_fstat
sys_fstat:
    mov w8, #14
    svc #0
    ret

.globl sys_pipe
sys_pipe:
    mov w8, #15
    svc #0
    ret

.globl sys_dup2
sys_dup2:
    mov w8, #16
    svc #0
    ret

.globl sys_sbrk
sys_sbrk:
    mov w8, #17
    svc #0
    ret
