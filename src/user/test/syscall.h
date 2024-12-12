#ifndef _USER_SYSCALL_
#define _USER_SYSCALL_
// in fs/file1206.h
#define O_READ 0x1
#define O_WRITE 0x2
#define O_CREATE 0x4

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

#endif // _USER_SYSCALL_
