#ifndef __LIB_ARM_
#define __LIB_ARM_

// data types in arm include word(32 bit), half-word(16 bit) and byte.
// for example:
// ldr = Load Word
// ldrh = Load unsigned Half Word
// ldrsh = Load signed Half Word
// ldrb = Load unsigned Byte
// ldrsb = Load signed Bytes

// 64-bit registers in 64-bit arm architecture
#define REG64(_)                                 \
    /* parameter and result registers */         \
    _(x0)                                        \
    _(x1)                                        \
    _(x2)                                        \
    _(x3)                                        \
    _(x4)                                        \
    _(x5)                                        \
    _(x6)                                        \
    _(x7)                                        \
    /* caller saved(indirect result register) */ \
    _(x8)                                        \
    /* caller saved(corruptible) */              \
    _(x9)                                        \
    _(x10)                                       \
    _(x11)                                       \
    _(x12)                                       \
    _(x13)                                       \
    _(x14)                                       \
    _(x15)                                       \
    /* IP0, corruptible */                       \
    _(x16)                                       \
    /* IP1, corruptible */                       \
    _(x17)                                       \
    /* PR */                                     \
    _(x18)                                       \
    /* callee saved(corruptible) */              \
    _(x19)                                       \
    _(x20)                                       \
    _(x21)                                       \
    _(x22)                                       \
    _(x23)                                       \
    _(x24)                                       \
    _(x25)                                       \
    _(x26)                                       \
    _(x27)                                       \
    _(x28)                                       \
    /* Frame pointer */                          \
    _(x29)                                       \
    /* Link pointer */                           \
    _(x30)                                       \
    /* stack pointer */                          \
    _(sp)

// NOTE: 32-bit registers start with w.
// first argument is in x0, second in x1, and so forth.
// return value in x0.

// mnemonic specifies the operation
#define MNEMONIC(_)                       \
    _(mov)                                \
    /* move and negate */                 \
    _(mvn)                                \
    /* add */                             \
    _(add)                                \
    /* sub */                             \
    _(sub)                                \
    /* mul */                             \
    _(mul)                                \
    /* arithmetical shift right */        \
    _(asr)                                \
    /* logical shift right */             \
    _(lsr)                                \
    /* logical shift left */              \
    _(lsl)                                \
    /* rotate right */                    \
    _(ror)                                \
    /* and */                             \
    _(And)                                \
    /* or */                              \
    _(orr)                                \
    /* xor */                             \
    _(eor)                                \
    /* load */                            \
    _(ldr)                                \
    /* store */                           \
    _(str)                                \
    /* push on stack */                   \
    _(push)                               \
    /* pop on stack */                    \
    _(pop)                                \
    /* compare */                         \
    _(cmp)                                \
    /* branch */                          \
    _(b)                                  \
    /* branch with link(call in x86) */   \
    _(bl)                                 \
    /* branch and exchange(ret in x86) */ \
    _(bx)                                 \
    /* branch with link and exchange */   \
    _(blx)

// bl: save (pc + 4) into LR and jump to function;
// bx ld branch and exchange to ld.

// condition codes
#define CONDCODE(_)                                                  \
    /* equal, not equal, greater, less, greater or eq, less or eq */ \
    _(eq)                                                            \
    _(le)                                                            \
    _(gt)                                                            \
    _(lt)                                                            \
    _(ge)                                                            \
    _(le) /* unsigned higher or same; unsigned lower */              \
            _(hs) _(lo)

// a few examples written in (inline) assembly
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// clang-format off
// # immediate number
// example: #3, #0x12
// 
// # instruction
// a typical instruction is made by the following templates:
// MNEMONIC{S}{condition} {Rd}, Operand1, Operand2
// where:
// MNEMONIC     - Short name (mnemonic) of the instruction
// {S}          - If S is specified, the condition flags are updated on the result of the operation 
// {condition}  - Condition that is needed to be met in order for the instruction to be executed          
// {Rd}         - Register (destination) for storing the result of the instruction 
// Operand1     - First operand. Either a register or an immediate value 
// Operand2     - Second(flexible) operand.immediate value or a register with an optional shift
// 
// # memory access
// memory access(ldr for read and str for write) need to compute adress.
// [r0]: address stored in r0
// [r0, r1]: address is (void *)r0 + r1
// [r0, imm]: address is (void *)r0 + imm
// [r0, r1, LSL#2]: address is (void *)r0 + (r1 << 2).
// clang-format on 

#endif // __LIB_ARM_
