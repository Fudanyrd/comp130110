#include "file1206.h"

#include <common/string.h>
#include <kernel/proc.h>
#include <kernel/sched.h>
#include <kernel/syscall.h>

/** 
 * WARNING:
 * NAME OF THIS FILE IS RANDOMIZED TO AVOID FUTURE GIT MERGE COLLISION! 
 */

/** Initialize rpi-os's file system */
void fs_init()
{
    init_block_device();
    const SuperBlock *sb = get_super_block();
    init_bcache(sb, &block_device);
    init_inodes(sb, &bcache);
    inodes.sync(NULL, inodes.root, false);
    ASSERT(inodes.root->valid);
    ASSERT(inodes.root->inode_no == ROOT_INODE_NO);
}

/** Initialize an inode */
static inline void init_dir(Inode *ino)
{
    inodes.sync(NULL, ino, false);
    memset(&ino->entry, 0, sizeof(ino->entry));
    ino->entry.num_links = 0x1;
}

/** Walk on the directory tree.
 * @param[out] buf leave the last level.
 * @param alloc if true, will allocate directories along the path.
 * @return NULL if failed.
 */
static Inode *walk(Inode *start, const char *path, char *buf)
{
    ASSERT(start->valid && start->entry.type == INODE_DIRECTORY);
    ASSERT(*path != '/');

    int i;
    for (i = 0; path[i] != 0 && path[i] != '/'; i++) {
        buf[i] = path[i];
    }
    buf[i] = 0;

    // advance path
    path += i;
    while (*path == '/') {
        path++;
    }
    if (*path == 0) {
        // do not walk any deeper
        return start;
    }

    usize nextno = inodes.lookup(start, buf, NULL);
    Inode *next;
    if (nextno == 0) {
        // fail to find dir.
        // release start, and return to caller.
        inodes.put(NULL, start);
        return NULL;
    } else {
        next = inodes.get(nextno);
        inodes.sync(NULL, next, false);
        ASSERT(next->valid);
    }

    // release start.
    inodes.put(NULL, start);
    return walk(next, path, buf);
}

/** Touch a file here. Will free inode start. */
static Inode *touch_here(Inode *start, const char *path)
{
    OpContext *ctx = kalloc(sizeof(OpContext));
    ASSERT(ctx != NULL);
    bcache.begin_op(ctx);

    ASSERT(strlen(path) < FILE_NAME_MAX_LENGTH);
    ASSERT(start->valid);
    ASSERT(*path != '/' && *path != 0);

    // alloc an inode for file
    usize no = inodes.alloc(ctx, INODE_REGULAR);
    Inode *next = inodes.get(no);
    inodes.sync(ctx, next, false);
    ASSERT(next->valid);
    if (inodes.insert(ctx, start, path, no) == (usize)-1) {
        // already exists, this should not happen
        PANIC();
    }

    // increment number of links
    start->entry.num_links++;

    // init the file.
    init_dir(next);
    next->entry.type = INODE_REGULAR;

    // done.
    inodes.sync(ctx, next, true);
    inodes.sync(ctx, start, true);
    inodes.put(ctx, start);
    bcache.end_op(ctx);
    kfree(ctx);
    return next;
}

File *fopen(const char *path, int flags)
{
    Proc *proc = thisproc();
    char *buf = kalloc(256);
    ASSERT(proc->cwd != NULL);
    ASSERT(buf != NULL);

    // check path name
    if (*path == 0) {
        // empty path name
        kfree(buf);
        return NULL;
    }

    /// allocate memory for file
    struct file *fobj = kalloc(sizeof(struct file));
    if (fobj == NULL) {
        // fail
        kfree(buf);
        return NULL;
    }
    fobj->ref = 1;
    fobj->off = 0;
    fobj->type = FD_INODE;

    // walk from start
    Inode *start = *path == '/' ? inodes.share(inodes.root) // absolute
                                  :
                                  inodes.share(proc->cwd); // relative
    while (*path == '/') {
        path++;
    }

    // allows to open root
    if (*path == 0) {
        Inode *ino = inodes.share(inodes.root);
        fobj->ino = ino;
        fobj->readable = (flags & F_READ) != 0;
        fobj->writable = (flags & F_WRITE) != 0;
        inodes.put(NULL, start);
        kfree(buf);
        return fobj;
    }

    // start walking
    Inode *ino = walk(start, path, buf);
    if (ino == NULL) {
        // fail
        kfree(buf);
        return NULL;
    }

    // lookup in the dir
    Inode *fino;
    usize no = inodes.lookup(ino, buf, NULL);
    if (no == 0) {
        if ((flags & F_CREATE) == 0) {
            // fail
            inodes.put(NULL, ino);
            kfree(buf);
            return NULL;
        } 

        // create the file.
        fino = touch_here(ino, buf);
        ASSERT(fino->entry.num_links == 1);
    } else {
        fino = inodes.get(no);
        inodes.sync(NULL, fino, false);
        inodes.put(NULL, ino);
    }
    ASSERT(fino->valid);
    fobj->ino = fino;
    fobj->readable = (flags & F_READ) != 0;
    fobj->writable = (flags & F_WRITE) != 0;

    kfree(buf);
    return fobj;
}

void fclose(File *fobj)
{
    switch (fobj->type) {
    case (FD_NONE): {
        break;
    }
    case (FD_PIPE): {
        if (fobj->readable) {
            pipe_close_read(fobj->pipe);
        } 
        if (fobj->writable) {
            pipe_close_write(fobj->pipe);
        }
        break;
    }
    case (FD_INODE): {
        inodes.put(NULL, fobj->ino);
        break;
    }
    }

    ASSERT(fobj->ref > 0);
    fobj->ref--;
    if (fobj->ref == 0) {
        kfree(fobj);
    }
}

isize fread(File *fobj, char *buf, u64 count)
{
    if (!fobj->readable) {
        // EACCESS
        return -1;
    }

    isize ret = 0;
    switch (fobj->type) {
    case (FD_NONE): {
        // E INVALID FILE
        ret = -1;
        break;
    }
    case (FD_PIPE): {
        ret = pipe_read(fobj->pipe, buf, count);
        break;
    }
    case (FD_INODE): {
        ret = inodes.read(fobj->ino, (u8 *)buf, fobj->off, count);
        break;
    }
    }

    return ret;
}

isize fseek(File *fobj, isize bias, int flag)
{
    if (fobj == NULL || fobj->type != FD_INODE) {
        return -1;
    }
    if (!fobj->ino->valid) {
        // load from disk
        inodes.sync(NULL, fobj->ino, false);
    }

    isize ret;
    switch (flag) {
    case (S_SET): {
        ret = 0 + bias;
        break;
    }
    case (S_CUR): {
        ret  = (isize)fobj->off + bias;
        break;
    }
    case (S_END): {
        ret = (isize)fobj->ino->entry.num_bytes + bias;
        break;
    }
    default: {
        // invalid parameter.
        ret = -1;
    }
    }

    if (ret < 0) {
        return -1;
    } 

    fobj->off = ret;
    return ret;
}

/** Write to an inode-based file. */
static isize fino_write(File *fobj, char *addr, isize n)
{    
    Inode *ino = fobj->ino;
    OpContext *ctx = kalloc(sizeof(OpContext));
    if (ctx == NULL) {
        // bad
        return -1;
    }
    bcache.begin_op(ctx);

    if (!ino->valid) {
        inodes.sync(ctx, ino, false);
    }
    if (ino->entry.type != INODE_REGULAR) {
        // not a regular file
        goto fiw_bad;
    }

    // we allows write beyond the file, 
    // so do not check offset.
    usize incr = inodes.write(ctx, ino, (u8 *)addr, fobj->off, (usize)n);
    // update the offset
    fobj->off += incr;

    bcache.end_op(ctx);
    kfree(ctx);
    return incr;

fiw_bad:
    bcache.end_op(ctx);
    kfree(ctx);
    return -1;
}

isize fwrite(File *fobj, char* addr, isize n)
{
    isize ret;
    switch (fobj->type) {
    case (FD_INODE): {
        ret = fino_write(fobj, addr, n);
        break;
    }
    case (FD_PIPE): {
        ret = pipe_write(fobj->pipe, addr, n);
        break;
    }
    default : {
        // case (FD_NONE):
        ret = -1;
        break;
    }
    }
    return ret;
}

// create directory.
int sys_mkdir(const char *path)
{
    ASSERT(IS_KERNEL_ADDR(path));

    Proc *proc = thisproc();
    char *buf = kalloc(256);
    ASSERT(proc->cwd->valid);
    ASSERT(proc->cwd != NULL);
    if (buf == NULL) {
        kfree(buf);
        return -1;
    }

    // check path name
    if (*path == 0) {
        // empty path name, fail
        kfree(buf);
        return -1;
    }   

    // walk from start
    Inode *start = *path == '/' ? inodes.share(inodes.root) // absolute
                                  :
                                  inodes.share(proc->cwd); // relative
    while (*path == '/') {
        path++;
    }
    
    if (*path == 0) {
        // root dir exists
        kfree(buf);
        return -1;
    }

    // start walking
    Inode *ino = walk(start, path, buf);
    ASSERT(ino->valid);
    ASSERT(*buf != '/');
    if (ino == NULL) {
        // fail
        kfree(buf);
        return -1;
    }

    // lookup in the dir first.
    usize no = inodes.lookup(ino, buf, NULL);
    if (no != 0) {
        // fail: already exists
        inodes.put(NULL, ino);
        kfree(buf);
        return -1;
    }

    // create a ctx and start op
    OpContext *ctx = kalloc(sizeof(ctx));
    bcache.begin_op(ctx);
    if (ctx == NULL) {
        inodes.put(NULL, ino);
        kfree(buf);
        return -1;
    }

    // create a directory.
    u64 dirno = inodes.alloc(ctx, INODE_DIRECTORY);
    Inode *dir = inodes.get(dirno);
    inodes.sync(ctx, dir, false);
    memset((void *)&dir->entry, 0, sizeof(dir->entry));
    dir->entry.num_links = 1;
    dir->entry.type = INODE_DIRECTORY;
    
    // create entry in dir
    inodes.insert(ctx, dir, ".", dirno);
    inodes.insert(ctx, dir, "..", ino->inode_no);

    // create entry in parent dir
    inodes.insert(ctx, ino, buf, dirno);
    ino->entry.num_links ++;

    // sync, release
    inodes.sync(ctx, ino, true);
    inodes.sync(ctx, dir, true);
    inodes.put(ctx, ino);
    inodes.put(ctx, ino);
    bcache.end_op(ctx);

    kfree(buf);
    kfree(ctx);
    return 0;
}

// change directory. requires a kernel virtual address.
int sys_chdir(const char *path)
{
    ASSERT(IS_KERNEL_ADDR(path));

    Proc *proc = thisproc();
    char *buf = kalloc(256);
    ASSERT(proc->cwd != NULL);
    ASSERT(proc->cwd->valid);
    if (buf == NULL) {
        kfree(buf);
        return -1;
    }

    // check path name
    if (*path == 0) {
        // empty path name, fail
        kfree(buf);
        return -1;
    }   

    // walk from start
    Inode *start = *path == '/' ? inodes.share(inodes.root) // absolute
                                  :
                                  inodes.share(proc->cwd); // relative
    while (*path == '/') {
        path++;
    }

    if (*path == 0) {
        // change dir to root: allowd.
        inodes.put(NULL, start);
        inodes.put(NULL, proc->cwd);
        proc->cwd = inodes.share(inodes.root);
        kfree(buf);
        return 0;
    }

    // start walking
    Inode *ino = walk(start, path, buf);
    ASSERT(ino->valid);
    ASSERT(*buf != '/');
    if (ino == NULL) {
        // fail
        kfree(buf);
        return NULL;
    }

    usize dno = inodes.lookup(ino, buf, NULL);
    inodes.put(NULL, ino);
    if (dno == 0) {
        // path not exist
        kfree(buf);
        return -1;
    }

    // to the destination
    Inode *dir = inodes.get(dno);
    ASSERT(dir->inode_no == dno);
    ASSERT(dir->rc.count > 0);
    inodes.sync(NULL, dir, false);
    if (dir->entry.type != INODE_DIRECTORY) {
        // not a directory, fail
        // proc->cwd does not change.
        kfree(buf);
        return -1;
    }

    // set new cwd for this proc.
    inodes.put(NULL, proc->cwd);
    proc->cwd = dir;
    return 0;
}

int sys_readdir(int fd, char *buf)
{
    ASSERT(IS_KERNEL_ADDR(buf));

    if (fd < 0 || fd >= 16) {
        // invalid fd
        return -1;
    }
    // get open file table.
    Proc *proc = thisproc();
    File *fobj = proc->ofile.ofile[fd];

    if (fobj == NULL || fobj->type != FD_INODE) {
        // invalid fd
        return -1;
    }

    Inode *ino = fobj->ino;
    ASSERT(ino != NULL);
    if (!ino->valid) {
        inodes.sync(NULL, ino, false);
    }
    if (ino->entry.type != INODE_DIRECTORY) {
        // invalid inode
        return -1;
    }

    if (inodes.read(ino, (u8 *)buf, fobj->off, 
                    sizeof(DirEntry)) != sizeof(DirEntry)) {
        // fail
        return -1;
    }
    ASSERT(fobj->off % sizeof(DirEntry) == 0);
    fobj->off += sizeof(DirEntry);
    return 0;
}

int sys_open(const char *path, int flags)
{
    ASSERT(IS_KERNEL_ADDR(path));
    // alloc fd.
    Proc *proc = thisproc();
    int fd = -1;
    for (int i = 0; i < 16; i++) {
        if (proc->ofile.ofile[i] == NULL) {
            fd = i;
            break;
        }
    }
    if (fd < 0) {
        return -1;
    }

    File *fobj = fopen(path, flags);
    if (fobj == NULL) {
        // fail
        return -1;
    }

    proc->ofile.ofile[fd] = fobj;
    fobj->type = FD_INODE;
    return fd;
}

int sys_close(int fd)
{
    if (fd < 0 || fd >= 16) {
        // invalid fd, fail
        return -1;
    }

    Proc *proc = thisproc();
    File *fobj = proc->ofile.ofile[fd];
    if (fobj == NULL) {
        // invalid fd, fail
        return -1;
    }

    fclose(fobj);
    // clear in the opent able
    proc->ofile.ofile[fd] = NULL;
    return 0;
}
