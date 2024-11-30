#include <fs/inode.h>
#include <fs/defines.h>

// allow read
#define F_READ 0x1
// allow write
#define F_WRITE 0x2

#define F_RDONLY F_READ
#define F_WRONLY F_WRITE
#define F_RDWR (F_READ | F_WRITE)
#define F_CREAT 0x4

extern InodeTree inodes;

struct file;

// Mock proc struct
struct proc {
    // current working dir
    Inode *cwd;
    // files opened by this proc
    // do not have stdin, stdout, stderr.
    struct file *ofile[64];

    // other members of proc...
};

// Mock file struct
struct file {
    // inode
    Inode *ino;
    // offset
    u64 offset;
    // protection bits
    int prot;
    // num links
    u32 nlink;
};

/** Unless specifed, return 0 as ok, -1 as failure,
 * Will handle both absolute path and relative path name.
 */

void file_init(OpContext *ctx, BlockCache *bc);

int sys_create(const char *path, int type);
int sys_chdir(const char *path);
int sys_readdir(int dirfd, DirEntry *dir);

// unlink a file
int sys_unlink(const char *path);
// remove a dir
int sys_rmdir(const char *path);

int sys_inode(int fd, InodeEntry *entr);

static inline int sys_mkdir(const char *path)
{
    return sys_create(path, INODE_DIRECTORY);
}

/** Return file descriptors.  */
int sys_open(const char *path, int prot);
int sys_close(int fd);

/** Returns @ bytes read/write. */

long sys_read(int fd, char *buf, u64 count);
long sys_write(int fd, char *buf, u64 count);

#define S_SET 0x1
#define S_CUR 0x2
#define S_END 0x3

/** Returns offset after seek. */
long sys_lseek(int fd, long offset, int flag);
