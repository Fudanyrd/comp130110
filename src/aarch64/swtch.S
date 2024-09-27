// Do kernel-mode context switch
// x0 (first parameter): new context ptr
// x1 (second parameter): addr to save old context ptr

#define pushp(a, b) stp a, b, [sp, #-0x10]!
#define popp(a, b) ldp a, b, [sp], #0x10 

.globl swtch
swtch:
// TODO: save and restore KernelContext
    // store old ones
    // see kernel/proc.c
    str  x8, [x0,   #0]
    str x16, [x0,   #8]
    str x17, [x0,  #16]
    str x18, [x0,  #24]
    str x19, [x0,  #32]
    str x20, [x0,  #40]
    str x21, [x0,  #48]
    str x22, [x0,  #56]
    str x23, [x0,  #64]
    str x24, [x0,  #72]
    str x25, [x0,  #80]
    str x26, [x0,  #88]
    str x27, [x0,  #96]
    str fp,  [x0, #104]
    str lr,  [x0, #112]
    mov x2,  sp    // cannot directly store/load sp
    str x2,  [x0, #120]
    str x0,  [x0, #128]

    // retrieve new ones
    ldr  x8, [x1,   #0]
    ldr x16, [x1,   #8]
    ldr x17, [x1,  #16]
    ldr x18, [x1,  #24]
    ldr x19, [x1,  #32]
    ldr x20, [x1,  #40]
    ldr x21, [x1,  #48]
    ldr x22, [x1,  #56]
    ldr x23, [x1,  #64]
    ldr x24, [x1,  #72]
    ldr x25, [x1,  #80]
    ldr x26, [x1,  #88]
    ldr x27, [x1,  #96]
    ldr fp,  [x1, #104]
    ldr lr,  [x1, #112]
    ldr x2,  [x1, #120]
    ldr x0,  [x1, #128]
    mov sp,  x2
    ret
