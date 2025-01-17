#pragma once
#ifndef _KERNEL_EXEC_
#define _KERNEL_EXEC_

#include <common/defines.h>
#include <kernel/proc.h>
#include <kernel/sched.h>

// clang-format off
/** My program's address space layout
 * +----------+ <- 0xc0000000
 * |  stack   |
 * +----------+ <- 0xc0000000 - 32KB
 * |          |
 * |  empty   |
 * |          |
 * +----------+
 * |   .bss   |
 * +----------+ < determined by elf
 * |  .data   |
 * +----------+
 * |  .text   |
 * +----------+ < determined by elf
 */
// clang-format on

/** The initial stack position.
 * NOTE: the stack grows to low address!
 */
#define STACK_START 0xc0000000

/* Type for a 16-bit quantity.  */
typedef u16 Elf32_Half;
typedef u16 Elf64_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef u32 Elf32_Word;
typedef i32 Elf32_Sword;
typedef u32 Elf64_Word;
typedef i32 Elf64_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef u64 Elf32_Xword;
typedef i64 Elf32_Sxword;
typedef u64 Elf64_Xword;
typedef i64 Elf64_Sxword;

/* Type of addresses.  */
typedef u32 Elf32_Addr;
typedef u64 Elf64_Addr;

/* Type of file offsets.  */
typedef u32 Elf32_Off;
typedef u64 Elf64_Off;

/* Type for section indices, which are 16-bit quantities.  */
typedef u16 Elf32_Section;
typedef u16 Elf64_Section;

/* Type for version symbol information.  */
typedef Elf32_Half Elf32_Versym;
typedef Elf64_Half Elf64_Versym;

#define EI_NIDENT (16)
typedef struct {
    unsigned char e_ident[EI_NIDENT]; /* Magic number and other info */
    Elf64_Half e_type; /* Object file type */
    Elf64_Half e_machine; /* Architecture */
    Elf64_Word e_version; /* Object file version */
    Elf64_Addr e_entry; /* Entry point virtual address */
    Elf64_Off e_phoff; /* Program header table file offset */
    Elf64_Off e_shoff; /* Section header table file offset */
    Elf64_Word e_flags; /* Processor-specific flags */
    Elf64_Half e_ehsize; /* ELF header size in bytes */
    Elf64_Half e_phentsize; /* Program header table entry size */
    Elf64_Half e_phnum; /* Program header table entry count */
    Elf64_Half e_shentsize; /* Section header table entry size */
    Elf64_Half e_shnum; /* Section header table entry count */
    Elf64_Half e_shstrndx; /* Section header string table index */
} Elf64_Ehdr;

#define PT_NULL 0 /* Program header table entry unused */
#define PT_LOAD 1 /* Loadable program segment */
#define PT_DYNAMIC 2 /* Dynamic linking information */
#define PT_INTERP 3 /* Program interpreter */
#define PT_NOTE 4 /* Auxiliary information */
#define PT_SHLIB 5 /* Reserved */
#define PT_PHDR 6 /* Entry for header table itself */
#define PT_TLS 7 /* Thread-local storage segment */
#define PT_NUM 8 /* Number of defined types */

/* Legal values for p_flags (segment flags).  */

#define PF_X (1 << 0) /* Segment is executable */
#define PF_W (1 << 1) /* Segment is writable */
#define PF_R (1 << 2) /* Segment is readable */
#define PF_F (1 << 3) /* Segment has a file backend */
#define PF_S (1 << 4) /* Segment is shared. */
#define PF_MASKOS 0x0ff00000 /* OS-specific */
#define PF_MASKPROC 0xf0000000 /* Processor-specific */

typedef struct {
    Elf64_Word p_type; /* Segment type */
    Elf64_Word p_flags; /* Segment flags */
    Elf64_Off p_offset; /* Segment file offset */
    Elf64_Addr p_vaddr; /* Segment virtual address */
    Elf64_Addr p_paddr; /* Segment physical address */
    Elf64_Xword p_filesz; /* Segment size in file */
    Elf64_Xword p_memsz; /* Segment size in memory */
    Elf64_Xword p_align; /* Segment alignment */
} Elf64_Phdr;

// Only valid value for e_machine
#define EM_AARCH64 183 /* ARM AARCH64 */

extern int exec(const char *path, char **argv);
extern int fork();

// Make stack size: 8 pages.
#define STACK_PAGE 8

#endif // _KERNEL_EXEC_
