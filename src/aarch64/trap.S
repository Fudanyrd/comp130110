#define pushp(a, b) stp a, b, [sp, #-0x10]!
#define popp(a, b) ldp a, b, [sp], #0x10 

/* `exception_vector.S` send all traps here. */
.global trap_entry
trap_entry:
// TODO: save UserContext
    sub sp, sp, #272
    str x0, [sp,   #0]
    str x1, [sp,   #8]
    str x2, [sp,  #16]
    str x3, [sp,  #24]
    str x4, [sp,  #32]
    str x5, [sp,  #40]
    str x6, [sp,  #48]
    str x7, [sp,  #56]
    str x8, [sp,  #64]
    str x9, [sp,  #72]
    str x10, [sp, #80]
    str x11, [sp, #88]
    str x12, [sp, #96]
    str x13, [sp, #104]
    str x14, [sp, #112]
    str x15, [sp, #120]
    str x16, [sp, #128]
    str x17, [sp, #136]
    str x18, [sp, #144]
    str x19, [sp, #152]
    str x20, [sp, #160]
    str x21, [sp, #168]
    str x22, [sp, #176]
    str x23, [sp, #184]
    str x24, [sp, #192]
    str x25, [sp, #200]
    str x26, [sp, #208]
    str x27, [sp, #216]
    str x28, [sp, #224]
    str lr,  [sp, #232]
    str fp,  [sp, #240]
    mov x0, sp
    str x0,  [sp, #248]
    mrs x0, spsr_el1
    str x0, [sp, #256]
    mrs x0, elr_el1
    str x0, [sp, #264]
    mov x0, sp

    bl trap_global_handler

.global trap_return
trap_return:
// TODO: restore UserContext
    ldr x0, [sp,   #0]
    ldr x1, [sp,   #8]
    ldr x2, [sp,  #16]
    ldr x3, [sp,  #24]
    ldr x4, [sp,  #32]
    ldr x5, [sp,  #40]
    ldr x6, [sp,  #48]
    ldr x7, [sp,  #56]
    ldr x8, [sp,  #64]
    ldr x9, [sp,  #72]
    ldr x10, [sp, #80]
    ldr x11, [sp, #88]
    ldr x12, [sp, #96]
    ldr x13, [sp, #104]
    ldr x14, [sp, #112]
    ldr x15, [sp, #120]
    ldr x16, [sp, #128]
    ldr x17, [sp, #136]
    ldr x18, [sp, #144]
    ldr x19, [sp, #152]
    ldr x20, [sp, #160]
    ldr x21, [sp, #168]
    ldr x22, [sp, #176]
    ldr x23, [sp, #184]
    ldr x24, [sp, #192]
    ldr x25, [sp, #200]
    ldr x26, [sp, #208]
    ldr x27, [sp, #216]
    ldr x28, [sp, #224]
    ldr lr,  [sp, #232]
    ldr fp,  [sp, #240]
    ldr x0,  [sp, #256]
    msr spsr_el1, x0
    ldr x0,  [sp, #264]
    msr elr_el1, x0
    ldr x0,  [sp, #0]
    add sp, sp, #272

    eret
