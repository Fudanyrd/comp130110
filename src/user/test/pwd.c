// pwd: print current working directory.

#include "syscall.h"

static char buf[256];
static char *names[16];
static InodeEntry entry;
static char *next;
static i32 lv = 0;

static int Fstat(const char *name);
static void pwd();
static void Chdir(const char *path);
static char *strcpy(char *dst, const char *src);
static u64 strlen(const char *name);

int main(int argc, char **argv)
{
    next = (char *)buf;
    pwd();
    return 0;
}

static int Fstat(const char *name)
{
    int fd = sys_open(name, O_RDONLY);
    if (fd < 0) {
        sys_write(2, "open fail\n", 11);
        sys_exit(1);
    }
    if (sys_fstat(fd, &entry) != 0) {
        sys_close(fd);
        sys_write(2, "stat fail\n", 11);
        sys_exit(1);
    }

    return fd;
}

static void pwd()
{
    int fd = Fstat(".");
    static DirEntry de;
    while (entry.inode_no != ROOT_INODE_NO) {
        u32 oldno = entry.inode_no;

        // change to parent dir, and read each entry
        // to match inode number.
        sys_close(fd);
        Chdir("../");
        fd = Fstat(".");
        while (sys_readdir(fd, &de) == 0) {
            if (oldno == de.inode_no) {
                names[lv++] = next;
                next = strcpy(next, de.name);
                next++;
                break;
            }
        }
    }

    sys_close(fd);
    for (int i = lv - 1; i >= 0; i--) {
        sys_write(1, "/", 1);
        sys_write(1, names[i], strlen(names[i]));
    }
    sys_write(1, "/\n", 2);
    return;
}

static char *strcpy(char *dst, const char *src)
{
    for (; *src != 0; src++, dst++) {
        *dst = *src;
    }
    *dst = 0;
    return dst;
}

static void Chdir(const char *path)
{
    if (sys_chdir(path) != 0) {
        sys_write(2, "chdir fail\n", 12);
        sys_exit(1);
    }
}

static u64 strlen(const char *name)
{
    char *old = name;
    for (; *name != 0; *name++) {
    }

    return (name - old);
}
