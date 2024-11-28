#include "file.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <common/defines.h>

/**
 * Specification: each of these method should put the inodes
 * it uses. They should use inodes.share() when accessing
 * inodes.root and proc.cwd
 */

static OpContext *ctx;
static BlockCache *bc;
static struct proc proc;

void file_init(OpContext *_ctx, BlockCache *_bc)
{
    ctx = _ctx;
    bc = _bc;
    proc.cwd = inodes.root;
    inodes.sync(ctx, proc.cwd, false);
    assert(proc.cwd->valid);
}

/** Initialize an inode */
static inline void init_dir(Inode *ino)
{
    inodes.sync(ctx, ino, false);
    memset(&ino->entry, 0, sizeof(ino->entry));
    ino->entry.num_links = 0x1;
}

/** Touch a file here. Will NOT free inodes */
static Inode *touch_here(Inode *start, const char *path)
{
    assert(strlen(path) < FILE_NAME_MAX_LENGTH);
    assert(start->valid);
    assert(*path != '/' && *path != 0);

    // alloc an inode for file
    usize no = inodes.alloc(ctx, INODE_REGULAR);
    Inode *next = inodes.get(no);
    inodes.sync(ctx, next, false);
    assert(next->valid);
    if (inodes.insert(ctx, start, path, no) == (usize)-1) {
        // already exists, this should not happen
        PANIC();
    }

    // init the file.
    init_dir(next);
    next->entry.type = INODE_REGULAR;

    // done.
    inodes.sync(ctx, next, true);
    inodes.sync(ctx, start, true);
    return next;
}

/** Create a directory here. Will NOT free inodes. */
static Inode *mkdir_here(Inode *start, const char *path)
{
    assert(strlen(path) < FILE_NAME_MAX_LENGTH);
    assert(start->valid);
    assert(*path != '/' && *path != 0);

    // alloc an inode for dir
    usize no = inodes.alloc(ctx, INODE_DIRECTORY);
    Inode *next = inodes.get(no);
    inodes.sync(ctx, next, false);
    assert(next->valid);
    if (inodes.insert(ctx, start, path, no) == (usize)-1) {
        // already exists, this should not happen
        PANIC();
    }

    /// init dir
    init_dir(next);
    next->entry.type = INODE_DIRECTORY;
    inodes.insert(ctx, next, ".", no);
    inodes.insert(ctx, next, "..", start->inode_no);

    // done
    inodes.sync(ctx, start, true);
    inodes.sync(ctx, next, true);
    return next;
}

/** Walk on the directory tree.
 * @param[out] buf leave the last level.
 * @param alloc if true, will allocate directories along the path.
 * @return NULL if failed.
 */
static Inode *walk(Inode *start, const char *path, char *buf, bool alloc)
{
    assert(start->valid && start->entry.type == INODE_DIRECTORY);
    assert(*path != '/');

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
        if (alloc) {
            // allocate new inode
            next = mkdir_here(start, buf);
            assert(next->valid);
        } else {
            // fail
            return NULL;
        }
    } else {
        next = inodes.get(nextno);
        inodes.sync(ctx, next, false);
        assert(next->valid);
    }

    // release start.
    inodes.put(ctx, start);
    return walk(next, path, buf, alloc);
}

int sys_create(const char *path, int type)
{
    if (*path == 0) {
        // empty file name
        return -1;
    }

    // create dirs along the path
    static char buf[256];

    // dup proc.cwd cause walk will free it
    Inode *start = *path == '/' ? inodes.share(inodes.root) // absolute
                                  :
                                  inodes.share(proc.cwd); // relative
    inodes.sync(ctx, start, false);
    assert(start->valid);

    while (*path == '/') { path ++; }
    if (*path == 0) {
        inodes.put(ctx, start);
        return -1;
    }

    Inode *ino = walk(start, path, buf, false);
    if (ino == NULL) {
        // start is already freed.
        return -1;
    }
    assert(ino->entry.type == INODE_DIRECTORY && ino->valid);
    if (buf[0] == 0) {
        // ??
        inodes.put(ctx, ino);
        return -1;
    }

    switch (type) {
    case (INODE_DIRECTORY): {
        Inode *ret = mkdir_here(ino, buf);
        inodes.put(ctx, ret);
        break;
    }
    case (INODE_REGULAR): {
        Inode *ret = touch_here(ino, buf);
        inodes.put(ctx, ret);
        break;
    }
    default: {
        // not supported
        PANIC();
    }
    }

    // release, done
    inodes.put(ctx, ino);
    return 0;
}

static struct file *fd2file(int fd)
{
    if (fd < 0 || fd >= 64) {
        return NULL;
    }
    return proc.ofile[fd];
}

int sys_readdir(int dirfd, DirEntry *dir)
{
    struct file *fobj = fd2file(dirfd);
    if (fobj == NULL) {
        return -1;
    }

    Inode *ino = fobj->ino;
    assert(fobj->nlink > 0);
    assert(fobj->offset % sizeof(DirEntry) == 0);
    assert(ino != NULL && ino->valid);
    assert(ino->entry.num_bytes >= 2 * sizeof(DirEntry));

    if (fobj->offset > ino->entry.num_bytes) {
        // read outside file
        return -1;
    }
    inodes.read(ino, (u8 *)dir, fobj->offset, sizeof(DirEntry));
    fobj->offset += sizeof(DirEntry);

    return 0;
}

static int allocfd()
{
    for (int i = 0; i < 64; i++) {
        if (proc.ofile[i] == NULL) {
            return i;
        }
    }

    PANIC();
}

int sys_open(const char *path, int prot)
{
    static char buf[256];
    Inode *start = *path == '/' ? inodes.share(inodes.root) // absolute
                                  :
                                  inodes.share(proc.cwd); // relative
    while (*path == '/') {
        path++;
    }
    if (*path == 0) {
        // empty path name
        return -1;
    }

    Inode *ino = walk(start, path, buf, false);
    if (ino == NULL) {
        // fail
        return -1;
    }

    usize no = inodes.lookup(ino, buf, NULL);
    Inode *fino;
    if (no == 0) {
        if ((prot & F_CREAT) == 0) {
            return -1;
        }
        // create file
        fino = touch_here(ino, buf);
        assert(fino->entry.num_links == 1);
    } else {
        fino = inodes.get(no);
        inodes.sync(ctx, fino, false);
    }
    assert(fino->valid);

    struct file *fobj = malloc(sizeof(struct file));
    assert(fobj != NULL);
    fobj->ino = fino;
    fobj->nlink = 1;
    fobj->offset = 0;
    fobj->prot = 0;
    fobj->prot |= (prot & F_READ);
    fobj->prot |= (prot & F_WRITE);

    int fd = allocfd();
    proc.ofile[fd] = fobj;
    return fd;
}

int sys_close(int fd)
{
    struct file *fobj = fd2file(fd);
    if (fobj == NULL) {
        return -1;
    }

    // free the inode
    inodes.put(ctx, fobj->ino);

    fobj->nlink--;
    if (fobj->nlink == 0) {
        free(fobj);
        proc.ofile[fd] = NULL;
    }
    return 0;
}

long sys_read(int fd, char *buf, u64 count)
{
    struct file *fobj = fd2file(fd);
    if (fobj == NULL || (fobj->prot & F_READ) == 0) {
        return -1;
    }

    Inode *ino = fobj->ino;
    assert(ino->valid);
    if (ino->entry.type != INODE_REGULAR) {
        return -1;
    }

    if (fobj->offset >= ino->entry.num_bytes) {
        // cannot read.
        return 0;
    }

    usize nread = inodes.read(ino, (u8 *)buf, fobj->offset, count);
    fobj->offset += nread;

    return nread;
}

long sys_write(int fd, char *buf, u64 count)
{
    struct file *fobj = fd2file(fd);
    if (fobj == NULL || (fobj->prot & F_WRITE) == 0) {
        return -1;
    }

    Inode *ino = fobj->ino;
    assert(ino->valid);
    if (ino->entry.type != INODE_REGULAR) {
        return -1;
    }
    if (fobj->offset >= ino->entry.num_bytes) {
        // cannot read.
        return 0;
    }

    usize nwrt = inodes.write(ctx, ino, (u8 *)buf, fobj->offset, count);
    fobj->offset += nwrt;

    return nwrt;
}

long sys_lseek(int fd, u64 offset, int flag)
{
    struct file *fobj = fd2file(fd);
    if (fobj == NULL || fobj->ino) {
        return -1;
    }

    Inode *ino = fobj->ino;
    assert(ino->valid);

    // we allow seeking dir.
    // in this case the count means dir entry.
    if (ino->entry.type == INODE_DEVICE) {
        assert(fobj->offset % sizeof(DirEntry) == 0);
        offset *= sizeof(DirEntry);
    }

    switch (flag) {
    case (S_SET): {
        fobj->offset = offset;
        break;
    }
    case (S_CUR): {
        fobj->offset += offset;
        break;
    }
    case (S_END): {
        fobj->offset = ino->entry.num_bytes + offset;
        break;
    }
    default: {
        // failure
        return -1;
    }
    }

    if (ino->entry.type == INODE_DEVICE) {
        return fobj->offset / sizeof(DirEntry);
    }
    return fobj->offset;
}
