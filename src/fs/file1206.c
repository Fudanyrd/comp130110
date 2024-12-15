#include "file1206.h"

#include <common/string.h>
#include <driver/zero.h>
#include <kernel/proc.h>
#include <kernel/sched.h>
#include <kernel/syscall.h>

extern isize uart_read(u8 *dst, usize count);
extern isize uart_write(u8 *src, usize count);
extern Device devices[8];

// console device, may be used by a lot of progs
File *console;

// create a device on this inode.
static void inode_mknod(Inode *ino, u16 major, u16 minor) 
{
    inodes.lock(ino);
    ino->entry.type = INODE_DEVICE;
    ino->entry.num_links = 4096;
    ino->entry.num_bytes  = -1;
    ino->entry.minor = minor;
    ino->entry.major = major;
    ino->rc.count = 4096;
    inodes.unlock(ino);
}

void init_devices()
{
    // initialize devices
    sys_mkdir("/dev");
    File *devdir = fopen("/dev", F_READ);
    if (devdir == NULL) {
        printk("create /dev FAIL\n");
        PANIC();
    }

    // make a device named uart.
    Device uart;
    uart.read = uart_read;
    uart.write = uart_write;
    devices[0] = uart;

    File *fuart = fopen("/dev/console", F_READ | F_WRITE | F_CREATE);
    if (fuart == NULL) {
        printk("cannot create console\n");
        PANIC();
    }

    // mark it as cannot remove, (pinned)
    Inode *iuart = fuart->ino;
    inode_mknod (iuart, 0, 0);

    // make a zero device.
    Device zero;
    zero.read = zero_read;
    zero.write = null_write;
    devices[1] = zero;

    File *fzero = fopen("/dev/zero", F_READ | F_WRITE | F_CREATE);
    if (fzero == NULL) {
        printk("cannot create zero\n");
        PANIC();
    }
    inode_mknod (fzero->ino, 0, 1);

    // make a null device
    Device null;
    null.read = null_read;
    null.write = null_write;
    devices[2] = null;

    File *fnull = fopen("/dev/null", F_READ | F_WRITE | F_CREATE);
    if (fnull == NULL) {
        printk("cannot create null\n");
        PANIC();
    }
    inode_mknod (fnull->ino, 0, 2);

    // do not sync. It does nothing for a device.

    console = fuart;
    fclose(devdir);
}

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
    // ERROR: must hold the lock.
    inodes.lock(inodes.root);
    inodes.sync(NULL, inodes.root, false);
    inodes.unlock(inodes.root);
    ASSERT(inodes.root->valid);
    ASSERT(inodes.root->inode_no == ROOT_INODE_NO);
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

    inodes.lock(start);
    usize nextno = inodes.lookup(start, buf, NULL);
    inodes.unlock(start);
    Inode *next;
    if (nextno == 0) {
        // fail to find dir.
        // release start, and return to caller.
        inodes.put(NULL, start);
        return NULL;
    } else {
        next = inodes.get(nextno);
        inodes.lock(next);
        inodes.sync(NULL, next, false);
        inodes.unlock(next);
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
    inodes.lock(next);
    inodes.sync(ctx, next, false);
    inodes.unlock(next);
    ASSERT(next->valid);
    inodes.lock(start);
    if (inodes.insert(ctx, start, path, no) == (usize)-1) {
        // already exists, this should not happen
        PANIC();
    }

    // increment number of links
    start->entry.num_links++;
    inodes.sync(ctx, start, true);
    inodes.unlock(start);

    // init the file.
    inodes.lock(next);
    inodes.sync(NULL, next, false);
    memset(&next->entry, 0, sizeof(next->entry));
    next->entry.num_links = 0x1;
    next->entry.type = INODE_REGULAR;
    inodes.sync(ctx, next, true);
    inodes.unlock(next);

    // done.
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
    inodes.lock(ino);
    usize no = inodes.lookup(ino, buf, NULL);
    inodes.unlock(ino);

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
        inodes.lock(fino);
        inodes.sync(NULL, fino, false);
        inodes.unlock(fino);
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

static inline isize file_read(File *fobj, char *buf, u64 count)
{
    Inode *ino = fobj->ino;

    if (ino->entry.type != INODE_REGULAR && ino->entry.type != INODE_DEVICE) {
        // not a valid file
        return -1;
    }
    if (ino->entry.num_bytes <= fobj->off) {
        // read beyond file, fail
        return 0;
    }

    inodes.lock(ino);
    isize ret = inodes.read(ino, (u8 *)buf, fobj->off, count);
    fobj->off += ret;
    inodes.unlock(ino);
    return ret;
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
        ret = file_read(fobj, buf, count);
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
        inodes.lock(fobj->ino);
        inodes.sync(NULL, fobj->ino, false);
        inodes.unlock(fobj->ino);
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

/** Write a small batch of data to avoid large txn.
 * @param n it is suggested that n is no larger than 2048, i.e. 4 blocks.
 */
static INLINE isize file_write_safe(OpContext *ctx, File *fobj, char *addr, 
                                    isize n)
{
    ASSERT(n <= BLOCK_SIZE * 4);
    Inode *ino = fobj->ino;
    bcache.begin_op(ctx);

    inodes.lock(ino);
    // we allows write beyond the file, 
    // so do not check offset.
    usize incr = inodes.write(ctx, ino, (u8 *)addr, fobj->off, (usize)n);
    // update the offset
    fobj->off += incr;
    inodes.unlock(ino);

    // end op
    bcache.end_op(ctx);
    return incr;
}

/** Write to an inode-based file. */
static isize fino_write(File *fobj, char *addr, isize n)
{    
    Inode *ino = fobj->ino;

    // treat device somewhat differently
    if (ino->entry.type == INODE_DEVICE) {
        inodes.lock(ino);
        isize ret = inodes.write(NULL, ino, (u8 *)addr, 0, n);
        inodes.unlock(ino);
        return ret;
    }

    OpContext *ctx = kalloc(sizeof(OpContext));
    if (ctx == NULL) {
        // bad
        return -1;
    }

    if (!ino->valid) {
        inodes.lock(ino);
        inodes.sync(ctx, ino, false);
        inodes.unlock(ino);
    }
    if (ino->entry.type != INODE_REGULAR) {
        // not a regular file
        goto fiw_bad;
    }

    isize ret = 0;
    isize nwrt = BLOCK_SIZE - (fobj->off % BLOCK_SIZE);
    nwrt = n > nwrt ? nwrt : n;

    // write to block boundary first.
    nwrt = file_write_safe(ctx, fobj, addr, nwrt);
    if (nwrt < 0) {
        goto fiw_bad;
    }
    // then advance.
    addr += nwrt;
    ret += nwrt;
    n -= nwrt;

    // write in terms of 4 blocks.
    while (n > 0) {
        nwrt = BLOCK_SIZE * 4;
        nwrt = n > nwrt ? nwrt : n;

        // do write safely.
        nwrt = file_write_safe(ctx, fobj, addr, nwrt);
        if (nwrt < 0) {
            goto fiw_bad;
        }

        // then advance.
        addr += nwrt;
        ret += nwrt;
        n -= nwrt;
    }

    kfree(ctx);
    return ret;

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

    inodes.lock(ino);
    // lookup in the dir first.
    usize no = inodes.lookup(ino, buf, NULL);
    inodes.unlock(ino);

    if (no != 0) {
        // fail: already exists
        inodes.put(NULL, ino);
        kfree(buf);
        return -1;
    }

    // create a ctx and start op
    OpContext *ctx = kalloc(sizeof(OpContext));
    bcache.begin_op(ctx);
    if (ctx == NULL) {
        inodes.put(NULL, ino);
        kfree(buf);
        return -1;
    }

    // create a directory.
    u64 dirno = inodes.alloc(ctx, INODE_DIRECTORY);
    Inode *dir = inodes.get(dirno);

    // initialize the directory's inode.
    inodes.lock(dir);
    inodes.sync(ctx, dir, false);
    memset((void *)&dir->entry, 0, sizeof(dir->entry));
    dir->entry.num_links = 1;
    dir->entry.type = INODE_DIRECTORY;
    
    // create entry in dir
    inodes.insert(ctx, dir, ".", dirno);
    inodes.insert(ctx, dir, "..", ino->inode_no);
    inodes.sync(ctx, dir, true);
    inodes.unlock(dir);
    inodes.put(ctx, dir);
    dir = NULL;

    // create entry in parent dir
    inodes.lock(ino);
    inodes.insert(ctx, ino, buf, dirno);
    ino->entry.num_links ++;
    inodes.sync(ctx, ino, true);
    inodes.unlock(ino);
    inodes.put(ctx, ino);
    ino = NULL;

    // sync, release
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

    inodes.lock(ino);
    usize dno = inodes.lookup(ino, buf, NULL);
    inodes.unlock(ino);
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
    inodes.lock(dir);
    inodes.sync(NULL, dir, false);
    inodes.unlock(dir);
    if (dir->entry.type != INODE_DIRECTORY) {
        // not a directory, fail
        // proc->cwd does not change.
        kfree(buf);
        inodes.put(NULL, dir);
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

    if (fd < 0 || fd >= MAXOFILE) {
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
        inodes.lock(ino);
        inodes.sync(NULL, ino, false);
        inodes.unlock(ino);
    }
    if (ino->entry.type != INODE_DIRECTORY) {
        // invalid inode
        return -1;
    }

    ASSERT(fobj->off % sizeof(DirEntry) == 0);
    ASSERT(ino->entry.num_bytes % sizeof(DirEntry) == 0);
    if (fobj->off >= ino->entry.num_bytes) {
        // read beyond dir.
        return -1;
    }

    bool success = false;
    inodes.lock(ino);
    while (fobj->off < ino->entry.num_bytes) {
        inodes.read(ino, (u8 *)buf, fobj->off, sizeof(DirEntry));
        fobj->off += sizeof(DirEntry);
        DirEntry *entr = (DirEntry *)buf;
        if (entr->inode_no != 0) {
            success = true;
            break;
        }
    }    
    inodes.unlock(ino);

    // seek reaches end.
    return success ? 0 : (-1);
}

int sys_open(const char *path, int flags)
{
    ASSERT(IS_KERNEL_ADDR(path));
    // alloc fd.
    Proc *proc = thisproc();
    int fd = -1;
    for (int i = 0; i < MAXOFILE; i++) {
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
    if (fd < 0 || fd >= MAXOFILE) {
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

isize sys_read(int fd, char *buf, usize count)
{
    ASSERT(IS_KERNEL_ADDR(buf));
    if (fd < 0 || fd >= MAXOFILE) {
        // invalid fd 
        return -1;
    }

    Proc *proc = thisproc();
    File *fobj = proc->ofile.ofile[fd];
    if (fobj == NULL) {
        // invalid fd, fail
        return -1;
    }

    return fread(fobj, buf, count);
}

isize sys_write(int fd, char *buf, usize count)
{
    ASSERT(IS_KERNEL_ADDR(buf));
    if (fd < 0 || fd >= MAXOFILE) {
        // out of bound access
        return -1;
    }

    Proc *proc = thisproc();
    File *fobj = proc->ofile.ofile[fd];
    if (fobj == NULL) {
        return -1;
    }

    return fwrite(fobj, buf, count);
}

/** Removes an entry whose inode no is num. Will sync dir.
 * @throw PANIC() if num is not found.
 */
static void inode_rm_entry(OpContext *ctx, Inode *dir, usize num);

/** Unlink a file or directory */
int sys_unlink(const char *target)
{
    ASSERT(IS_KERNEL_ADDR(target));
    if (*target == NULL) {
        return -1;
    }


    Proc *proc = thisproc();
    // walk from start
    Inode *start = *target == '/' ? inodes.share(inodes.root) // absolute
                                  :
                                  inodes.share(proc->cwd); // relative
    while (*target == '/') {
        target++;
    }

    if (*target == 0) {
        // is root dir, cannot unlink.
        return -1;
    }

    // temporary buffer
    char *buf = kalloc(256);
    if (buf == NULL) {
        return -1;
    }

    // get to destination dir
    Inode *ino = walk(start, target, buf);
    if (ino == NULL) {
        // fail
        kfree(buf);
        return NULL;
    }

    // number of dest inode
    inodes.lock(ino);
    usize no = inodes.lookup(ino, buf, NULL);
    inodes.unlock(ino);
    kfree(buf); buf = NULL;

    if (no == 0) {
        // no such file or directory
        inodes.put(NULL, ino);
        return -1;
    }

    // targeted inode
    Inode *tgt = inodes.get(no);
    inodes.lock(tgt);
    inodes.sync(NULL, tgt, false);
    inodes.unlock(tgt);
    ASSERT(tgt->inode_no == no && tgt->valid);
    ASSERT(tgt->entry.num_links > 0);

    if (tgt->entry.type == INODE_INVALID) {
        // inode type not recognized.
        inodes.put(NULL, ino);
        inodes.put(NULL, tgt);
        return -1;
    }

    // allocate context, and start op
    OpContext *ctx = kalloc(sizeof(OpContext));
    if (ctx == NULL) {
        return -1;
    }
    int ret = 0;
    bcache.begin_op(ctx);

    // in the following steps, put ino
    // and tgt's parent inode.
    if (tgt->entry.type == INODE_DIRECTORY) {
        if (tgt->entry.num_links > 1) {
            // fail
            ret = -1;
        } else {
            tgt->entry.num_links = 0;
            inodes.sync(ctx, tgt, true);

            // should find the parent number
            inodes.lock(tgt);
            usize pno = inodes.lookup(tgt, "..", NULL);
            inodes.unlock(tgt);
            ASSERT(pno != no);

            // parent inode of tgt
            Inode *pr = inodes.get(pno);
            inodes.lock(pr);
            inodes.sync(ctx, pr, false);

            // remove the entry tgt from parent dir.
            ASSERT(pr->entry.num_links > 1);
            pr->entry.num_links--;
            inode_rm_entry(ctx, pr, no);
            inodes.sync(ctx, pr, true);
            inodes.unlock(pr);

            // clean up
            inodes.put(ctx, pr);
        }
        inodes.put(ctx, ino);
    } else {
        inodes.lock(tgt);
        tgt->entry.num_links--;
        inodes.sync(ctx, tgt, true);
        inodes.unlock(tgt);

        // parent dir of tgt is ino!
        inodes.lock(ino);
        ASSERT(ino->entry.num_links > 1);
        ino->entry.num_links --;
        inode_rm_entry(ctx, ino, no);
        inodes.sync(ctx, ino, true);
        inodes.unlock(ino);

        // clean up
        inodes.put(ctx, ino);
    }
    ino = NULL;

    // end op, clean up
    inodes.put(ctx, tgt);
    bcache.end_op(ctx);
    kfree(ctx);

    return ret;
}

// inode write will sync.
// so well this function.
static void inode_rm_entry(OpContext *ctx, Inode *dir, usize num)
{
    ASSERT(dir->valid);
    ASSERT(dir->entry.type == INODE_DIRECTORY && dir->entry.num_links > 0
           && dir->entry.num_bytes > 2 * sizeof(DirEntry));

    DirEntry entry;
    usize offset = 0;
    while (offset < dir->entry.num_bytes) {
        inodes.read(dir, (u8 *)&entry, offset, sizeof(DirEntry));
        if (entry.inode_no == num) {
            // found!
            break;
        }
        offset += sizeof(DirEntry);
    }

    if (offset >= dir->entry.num_bytes) {
        PANIC();
    }

    // erase the entry.
    entry.inode_no = 0;
    entry.name[0] = 0;

    // write back.
    inodes.write(ctx, dir, (u8 *)&entry, offset, sizeof(DirEntry));
}

File *fshare(File *src)
{
    if (src == NULL) {
        return NULL;
    }

    // need to hold lock to avoid concurrent
    // share to src->ref.
    __atomic_fetch_add(&src->ref, 1, __ATOMIC_ACQ_REL);
    switch (src->type) {
    case (FD_INODE): {
        inodes.share(src->ino);
        break;
    }
    case (FD_PIPE): {
        if (src->readable) {
            pipe_reopen_read(src->pipe);
        }
        if (src->writable) {
            pipe_reopen_write(src->pipe);
        }
    }
    default: {
        break;
    }
    }
    return src;
}
