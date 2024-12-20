#include <kernel/syscallno.h>
#include <kernel/proc.h>

#define NR_SYSCALL 512

#define IS_KERNEL_ADDR(addr) \
    (((u64)addr & 0xFFFF000000000000) == 0xFFFF000000000000)

/** Maximum number of syscall arguments */
#define EXE_MAX_ARGS 16

/**
 * @param ka kernel virtual address as dst
 * @param va user virtual address, provided in user context
 * @param size num bytes to copy into kernel
 * @return 0 on success.
 */
extern isize copyin(struct pgdir *pd, void *ka, u64 va, u64 size);

/** Copy to user space.
 * @param ka kernel virtual address as dst
 * @param va user virtual address, provided in user context
 * @param size num bytes to copy into kernel
 * @return 0 on success.
 */
extern isize copyout(struct pgdir *pd, void *ka, u64 va, u64 size);

/** Copyin a user null-terminated string
 * @return 0 on success, -1 if failure.
 * @note since the kernel does not alloc memory larger than a page, 
 * this returns -1 if the user's string is larger than PAGE_SIZE.
 */
extern isize copyinstr(struct pgdir *pd, void *ka, u64 va);

void syscall_entry(UserContext *context);
