#pragma once
#include <kernel/pt.h>
#include <kernel/exec1207.h>

#define MMAP_MIN_ADDR 0x60000000
#define MMAP_MAX_ADDR 0xb0000000

// return value
#define MMAP_FAILED ((u64)-1)

// prots

#define PROT_READ PF_R
#define PROT_WRITE PF_W
#define PROT_EXEC PF_X

// flags

#define MAP_PRIVATE 1
#define MAP_FIXED 2

// mmap syscall executor
u64 mmap(void *addr, u64 length, int prot, int flags, 
           int fd, isize offset);

int munmap(void *addr, u64 length);
