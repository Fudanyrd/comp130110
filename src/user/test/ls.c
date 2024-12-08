
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

extern void sys_print(const char *s, unsigned long len);
extern int sys_open(const char *s, int flags);
extern int sys_close(int fd);
static void list(const char *path);

int main(int argc, char **argv) 
{
    for (int i = 1; i < argc; i++) {
        list(argv[i]);
    }
    return 0;
}

static void list(const char *path)
{
    int fd = sys_open(path, O_READ);
    if (fd < 0) {
        // fail
        sys_print("FAIL", 4);
        return;
    }

    if (sys_close(fd) < 0) {
        sys_print("close FAIL", 10);
        return;
    }
    sys_print("ls PASS", 7);
}
