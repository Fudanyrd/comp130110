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
 * 
 * We do NOT allow deleting an unempty directory.
 * 
 * In my design, the number of links of a directory equals to 
 * its number of entries(except . and ..) plus 1. Then, when 
 * the number of links drop to 0, we know that it has no entry
 * in it and can remove it.
 */

static OpContext *ctx;
static BlockCache *bc;
static struct proc proc;

void file_init(OpContext *_ctx, BlockCache *_bc)
{
    ctx = _ctx;
    bc = _bc;
    proc.cwd = inodes.share(inodes.root);
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

    // increment number of links
    start->entry.num_links++;

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
    assert(start->entry.type == INODE_DIRECTORY);
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

    // increment number of links
    start->entry.num_links++;

    /// init dir
    init_dir(next);
    next->entry.type = INODE_DIRECTORY;
    // must successs
    assert(inodes.insert(ctx, next, ".", no) != (usize)-1);
    assert(inodes.insert(ctx, next, "..", start->inode_no) != (usize)-1);

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

int sys_unlink(const char *path)
{
    if (*path == 0) {
        return -1;
    }

    static char buf[256];
    Inode *start = *path == '/' ? inodes.share(inodes.root) :
                                  inodes.share(proc.cwd);

    // skip leading /
    while (*path == '/') {
        path++;
    }
    if (*path == 0) {
        // cannot unlink root
        inodes.put(ctx, start);
        return -1;
    }

    // last but two inode
    // NOTE: do not need to put start later,
    // cause it's done by walk.
    Inode *ino = walk(start, path, buf, false);
    if (ino == NULL) {
        return -1;
    }
    assert(ino->valid && ino->entry.type == INODE_DIRECTORY);

    usize idx;
    usize dno = inodes.lookup(ino, buf, &idx);
    if (dno == 0) {
        // no such file
        inodes.put(ctx, ino);
        return -1;
    }

    // destination
    Inode *dest = inodes.get(dno);
    inodes.sync(ctx, dest, false);
    assert(dest != NULL && dest->valid);
    assert(dest->entry.num_links > 0);
    if (dest->entry.type != INODE_REGULAR) {
        // not a file
        inodes.put(ctx, ino);
        inodes.put(ctx, dest);
        return -1;
    }
    dest->entry.num_links--;

    // by specification, also decrease # links
    // of the dir.
    assert(ino->entry.num_links > 1);
    ino->entry.num_links--;

    // remove the entry in the dir
    inodes.remove(ctx, ino, idx);

    // sync, release
    inodes.sync(ctx, dest, true);
    inodes.sync(ctx, ino, true);
    inodes.put(ctx, ino);
    inodes.put(ctx, dest);
    return 0;
}

int sys_rmdir(const char *path)
{
    if (*path == 0) {
        return -1;
    }

    static char buf[256];
    Inode *start = *path == '/' ? inodes.share(inodes.root) :
                                  inodes.share(proc.cwd);

    // skip leading /
    while (*path == '/') {
        path++;
    }
    if (*path == 0) {
        // cannot unlink root
        inodes.put(ctx, start);
        return -1;
    }

    // last but two inode
    // NOTE: do not need to put start later,
    // cause it's done by walk.
    Inode *ino = walk(start, path, buf, false);
    if (ino == NULL) {
        return -1;
    }
    assert(ino->valid && ino->entry.type == INODE_DIRECTORY);

    usize idx;
    usize dno = inodes.lookup(ino, buf, &idx);
    // free ino for it's not used later.
    // inodes.sync(ctx, ino, true);
    inodes.put(ctx, ino);
    ino = NULL;
    if (dno == 0) {
        // no such file
        return -1;
    }

    // destination
    Inode *dest = inodes.get(dno);
    inodes.sync(ctx, dest, false);
    assert(dest != NULL && dest->valid);
    assert(dest->entry.num_links > 0);
    if (dest->entry.type != INODE_DIRECTORY) {
        // not a file
        inodes.put(ctx, dest);
        return -1;
    }
    if (dest->entry.num_links != 1) {
        // this is not an empty dir.
        // do NOT delete, fail
        inodes.put(ctx, dest);
        return -1;
    }
    dest->entry.num_links--;

    // by specification, also decrease # links
    // of the dir.
    // NOTE: ino is NOT necessarily the parent of
    // dest. Eg. the path may be "./"
    // Need to find the parent and decrement
    // nlinks.
    usize pno = inodes.lookup(dest, "..", &idx);
    assert(pno != 0);
    Inode *parent = inodes.get(pno);
    assert(parent != NULL);
    assert(parent->entry.num_bytes > 2 * sizeof(DirEntry));
    assert(parent->entry.num_bytes % sizeof(DirEntry) == 0);
    assert(parent->entry.num_links > 1);
    parent->entry.num_links--;
    // remove the node in parent dir.

    // create a temporary buffer instead of use static buffer
    // to avoid race
    DirEntry *de = malloc(sizeof(DirEntry));
    de->inode_no = 0;
    assert(de != NULL);
    usize offset = 0;
    while (offset < parent->entry.num_bytes) {
        inodes.read(parent, (u8 *)de, offset, sizeof(DirEntry));
        if (de->inode_no == dno) {
            break;
        }
        offset += sizeof(DirEntry);
    }
    assert(de->inode_no == dno);
    // clear the entry, and write back.
    de->inode_no = 0;
    de->name[0] = 0;
    inodes.write(ctx, parent, (u8 *)de, offset, sizeof(DirEntry));
    free(de);
    inodes.sync(ctx, parent, true);
    inodes.put(ctx, parent);

    // sync, release
    inodes.sync(ctx, dest, true);
    inodes.put(ctx, dest);
    return 0;
}

int sys_chdir(const char *path)
{
    if (*path == 0) {
        return -1;
    }

    static char buf[256];
    Inode *start = *path == '/' ? inodes.share(inodes.root) :
                                  inodes.share(proc.cwd);

    // skip leading /
    while (*path == '/') {
        path++;
    }
    if (*path == 0) {
        // change to root dir
        inodes.put(ctx, start);
        inodes.put(ctx, proc.cwd);
        proc.cwd = inodes.share(inodes.root);
        return 0;
    }

    // last but two inode
    Inode *ino = walk(start, path, buf, false);
    if (ino == NULL) {
        return -1;
    }
    // destination

    usize dno = inodes.lookup(ino, buf, NULL);
    // put ino here, for it is not used later.
    inodes.put(ctx, ino);
    if (dno == 0) {
        // path not exist
        return -1;
    }
    Inode *dest = inodes.get(dno);
    inodes.sync(ctx, dest, false);
    assert(dest != NULL && dest->valid);
    if (dest->entry.type != INODE_DIRECTORY) {
        // not a dir, fail
        inodes.put(ctx, dest);
        return -1;
    }
    // release prev cwd
    // , and update cwd to dest
    inodes.put(ctx, proc.cwd);
    proc.cwd = dest;

    return 0;
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

    while (*path == '/') {
        path++;
    }
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
        assert(ret->valid);
        assert(ret->entry.type == INODE_DIRECTORY);
        inodes.put(ctx, ret);
        break;
    }
    case (INODE_REGULAR): {
        Inode *ret = touch_here(ino, buf);
        assert(ret->valid);
        assert(ret->entry.type == INODE_REGULAR);
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
        // invalid fd
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

    if (fobj->offset >= ino->entry.num_bytes) {
        // read outside file
        return -1;
    }

    while (fobj->offset < ino->entry.num_bytes) {
        inodes.read(ino, (u8 *)dir, fobj->offset, sizeof(DirEntry));
        fobj->offset += sizeof(DirEntry);
        if (dir->inode_no != 0) {
            // a valid entry.
            return 0;
        }
    }

    // no entry found since fob->offset.
    return -1;
}

static int allocfd()
{
    for (int i = 0; i < 64; i++) {
        if (proc.ofile[i] == NULL) {
            return i;
        }
    }

    // cannot alloc: buffer full!
    PANIC();
}

int sys_open(const char *path, int prot)
{
    if (*path == 0) {
        // empty path name
        return -1;
    }
    static char buf[256];
    Inode *start = *path == '/' ? inodes.share(inodes.root) // absolute
                                  :
                                  inodes.share(proc.cwd); // relative
    while (*path == '/') {
        path++;
    }

    // different from sys_create, it is allowed to
    // open root directory.
    if (*path == 0) {
        Inode *ino = inodes.share(inodes.root);
        struct file *fobj = malloc(sizeof(struct file));
        fobj->ino = ino;
        fobj->nlink = 1;
        fobj->offset = 0;
        fobj->prot = 0;
        fobj->prot |= (prot & F_READ);
        fobj->prot |= (prot & F_WRITE);

        int fd = allocfd();
        proc.ofile[fd] = fobj;
        inodes.put(ctx, start);
        return fd;
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
            inodes.put(ctx, ino);
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

    // do not put fino, for it is used in `file` struct.
    // but, the dir inode `ino` should be put.
    inodes.put(ctx, ino);
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
    assert(fobj->nlink != 0);

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
    usize nwrt = inodes.write(ctx, ino, (u8 *)buf, fobj->offset, count);
    fobj->offset += nwrt;

    return nwrt;
}

long sys_lseek(int fd, long offset, int flag)
{
    struct file *fobj = fd2file(fd);
    if (fobj == NULL || fobj->ino) {
        return -1;
    }

    Inode *ino = fobj->ino;
    assert(ino->valid);

    // we allow seeking dir.
    // in this case the count means dir entry.
    if (ino->entry.type == INODE_DIRECTORY) {
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

    if (ino->entry.type == INODE_DIRECTORY) {
        // return the dir entry index
        // instead of actual offset
        return fobj->offset / sizeof(DirEntry);
    }
    return fobj->offset;
}

int sys_inode(int fd, InodeEntry *entr)
{
    struct file *fobj = fd2file(fd);
    if (fobj == NULL) {
        return -1;
    }

    // copy dinode out
    memcpy(entr, &(fobj->ino->entry), sizeof(InodeEntry));
    return 0;
}
