#ifndef _USER_SYSCALL_
#define _USER_SYSCALL_
// in fs/file1206.h
#define O_READ 0x1
#define O_WRITE 0x2
#define O_CREATE 0x4

#define O_RDWR (O_READ | O_WRITE)
#define O_TRUNC 0
#define O_RDONLY O_READ
#define O_WRONLY O_WRITE

#define NULL ((void *)0x0)

// in common/defines.h
typedef signed char i8;
typedef unsigned char u8;
typedef signed short i16;
typedef unsigned short u16;
typedef signed int i32;
typedef unsigned int u32;
typedef signed long long i64;
typedef unsigned long long u64;

typedef i64 isize;
typedef u64 usize;

// in fs/defines.h

#define BLOCK_SIZE 512

// maximum number of distinct block numbers can be recorded in the log header.
#define LOG_MAX_SIZE ((BLOCK_SIZE - sizeof(usize)) / sizeof(usize))

#define INODE_NUM_DIRECT 11
// 128
#define INODE_NUM_INDIRECT (BLOCK_SIZE / sizeof(u32))
// (128 * 128)
#define INODE_NUM_DINDIRECT (INODE_NUM_INDIRECT * INODE_NUM_INDIRECT)

#define INODE_PER_BLOCK (BLOCK_SIZE / sizeof(InodeEntry))
// (11 + 128 + 128 * 128)
#define INODE_MAX_BLOCKS \
    (INODE_NUM_DIRECT + INODE_NUM_INDIRECT + INODE_NUM_DINDIRECT)
#define INODE_MAX_BYTES (INODE_MAX_BLOCKS * BLOCK_SIZE)

#define DIRENTR_PER_BLOCK (BLOCK_SIZE / sizeof(DirEntry))

// inode types:
#define INODE_INVALID 0
#define INODE_DIRECTORY 1
#define INODE_REGULAR 2 // regular file
#define INODE_DEVICE 3

#define ROOT_INODE_NO 1

typedef u16 InodeType;

#define BIT_PER_BLOCK (BLOCK_SIZE * 8)

// `type == INODE_INVALID` implies this inode is free.
typedef struct dinode {
    InodeType type;
    u16 major; // major device id, for INODE_DEVICE only.
    u16 minor; // minor device id, for INODE_DEVICE only.
    u16 num_links; // number of hard links to this inode in the filesystem.
    u32 num_bytes; // number of bytes in the file, i.e. the size of file.

    /** The following is not used in user space. */
    u32 addrs[INODE_NUM_DIRECT]; // direct addresses/block numbers.
    u32 indirect; // the indirect address block.
    u32 dindirect; // doubly-indirect address block
} InodeEntry;

// directory entry. `inode_no == 0` implies this entry is free.
// the maximum length of file names, including trailing '\0'.
#define FILE_NAME_MAX_LENGTH 14

typedef struct dirent {
    u16 inode_no;
    char name[FILE_NAME_MAX_LENGTH];
} DirEntry;

// in start.S
extern void sys_print(const char *s, unsigned long len);
extern int sys_open(const char *s, int flags);
extern int sys_close(int fd);
extern int sys_readdir(int fd, DirEntry *buf);
extern int sys_chdir(const char *path);
extern isize sys_read(int fd, char *buf, unsigned long size);
extern int sys_chdir(const char *path);
extern int sys_mkdir(const char *dirname);
extern isize sys_write(int fd, const char *buf, unsigned long size);
extern int sys_unlink(const char *path);
extern int sys_fork();
extern int sys_exit(int code);
extern int sys_wait(int *exitcode);
extern int sys_execve(const char *exe, char **argv);
extern int sys_fstat(int fd, InodeEntry *buf);
extern int sys_pipe(int *buf);
extern int sys_dup2(int old, int new);
extern void *sys_sbrk(isize growth);

// exec1207.h
/** The initial stack position.
 * NOTE: the stack grows to low address!
 */
#define STACK_START 0xc0000000

/* Type for a 16-bit quantity.  */
typedef u16 Elf32_Half;
typedef u16 Elf64_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef u32 Elf32_Word;
typedef	i32 Elf32_Sword;
typedef u32 Elf64_Word;
typedef	i32 Elf64_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef u64 Elf32_Xword;
typedef	i64  Elf32_Sxword;
typedef u64 Elf64_Xword;
typedef	i64  Elf64_Sxword;

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
  unsigned char	e_ident[EI_NIDENT];	/* Magic number and other info */
  Elf64_Half	e_type;			/* Object file type */
  Elf64_Half	e_machine;		/* Architecture */
  Elf64_Word	e_version;		/* Object file version */
  Elf64_Addr	e_entry;		/* Entry point virtual address */
  Elf64_Off	e_phoff;		/* Program header table file offset */
  Elf64_Off	e_shoff;		/* Section header table file offset */
  Elf64_Word	e_flags;		/* Processor-specific flags */
  Elf64_Half	e_ehsize;		/* ELF header size in bytes */
  Elf64_Half	e_phentsize;		/* Program header table entry size */
  Elf64_Half	e_phnum;		/* Program header table entry count */
  Elf64_Half	e_shentsize;		/* Section header table entry size */
  Elf64_Half	e_shnum;		/* Section header table entry count */
  Elf64_Half	e_shstrndx;		/* Section header string table index */
} Elf64_Ehdr;

#define	PT_NULL		0		/* Program header table entry unused */
#define PT_LOAD		1		/* Loadable program segment */
#define PT_DYNAMIC	2		/* Dynamic linking information */
#define PT_INTERP	3		/* Program interpreter */
#define PT_NOTE		4		/* Auxiliary information */
#define PT_SHLIB	5		/* Reserved */
#define PT_PHDR		6		/* Entry for header table itself */
#define PT_TLS		7		/* Thread-local storage segment */
#define	PT_NUM		8		/* Number of defined types */

/* Legal values for p_flags (segment flags).  */

#define PF_X		(1 << 0)	/* Segment is executable */
#define PF_W		(1 << 1)	/* Segment is writable */
#define PF_R		(1 << 2)	/* Segment is readable */
#define PF_F    (1 << 3) /* Segment has a file backend */
#define PF_MASKOS	0x0ff00000	/* OS-specific */
#define PF_MASKPROC	0xf0000000	/* Processor-specific */

typedef struct
{
  Elf64_Word	p_type;			/* Segment type */
  Elf64_Word	p_flags;		/* Segment flags */
  Elf64_Off	p_offset;		/* Segment file offset */
  Elf64_Addr	p_vaddr;		/* Segment virtual address */
  Elf64_Addr	p_paddr;		/* Segment physical address */
  Elf64_Xword	p_filesz;		/* Segment size in file */
  Elf64_Xword	p_memsz;		/* Segment size in memory */
  Elf64_Xword	p_align;		/* Segment alignment */
} Elf64_Phdr;

// mmap1217.h
#define MMAP_MIN_ADDR 0x60000000
#define MMAP_MAX_ADDR 0xb0000000

// return value
#define MMAP_FAILED ((void *)-1)

// prots

#define PROT_READ PF_R
#define PROT_WRITE PF_W
#define PROT_EXEC PF_X

// flags

#define MAP_PRIVATE 1
#define MAP_FIXED 2

// mmap syscall executor
void *sys_mmap(void *addr, u64 length, int prot, int flags, 
               int fd, isize offset);


#endif // _USER_SYSCALL_
