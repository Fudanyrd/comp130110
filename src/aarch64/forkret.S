// note:
// void fork_return(uintptr_t sp, uintptr_t entry);
// This function is used when returning 
// to user space after a fork() or execve()
// system call.
.globl fork_return

fork_return:
    // it will setup stack pointer and return address.
    mov sp, x0
    mov lr, x1
    // clear x0, x1
    mov x0, #0
    mov x1, #0
    ret
