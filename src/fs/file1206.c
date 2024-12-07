#include "file1206.h"

#include <common/string.h>
#include <kernel/proc.h>
#include <kernel/sched.h>

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
        return NULL;
    }

    /// allocate memory for file
    struct file *fobj = kalloc(sizeof(struct file));
    if (fobj == NULL) {
        // fail
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
        return fobj;
    }

    // start walking
    Inode *ino = walk(start, path, buf);
    if (ino == NULL) {
        // fail
        return NULL;
    }

    // lookup in the dir
    Inode *fino;
    usize no = inodes.lookup(ino, buf, NULL);
    if (no == 0) {
        if ((flags & F_CREATE) == 0) {
            // fail
            inodes.put(NULL, ino);
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

    kfree(fobj);
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
