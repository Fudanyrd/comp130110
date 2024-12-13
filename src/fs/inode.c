#include <common/string.h>
#include <fs/inode.h>
#include <kernel/mem.h>
#include <kernel/printk.h>

/**
    @brief the private reference to the super block.

    @note we need these two variables because we allow the caller to
            specify the block cache and super block to use.
            Correspondingly, you should NEVER use global instance of
            them.

    @see init_inodes
 */
static const SuperBlock *sblock;

/**
    @brief the reference to the underlying block cache.
 */
static const BlockCache *cache;

/**
    @brief global lock for inode layer, protects list of inodes.

    Use it to protect anything you need.

    e.g. the list of allocated blocks, ref counts, etc.
 */
static SpinLock lock;

/**
    @brief the list of all allocated in-memory inodes.

    We use a linked list to manage all allocated inodes.

    You can implement your own data structure if you want better performance.

    @see Inode
 */
static ListNode head;
static ListNode tail;

/** 8 devices should suffice. */
Device devices[8];

// push an inode into list. Must hold lock.
static INLINE void inode_push_lst(Inode *ino)
{
    ListNode *prev = tail.prev;
    ListNode *nxt = &tail;
    ino->node.prev = prev;
    ino->node.next = nxt;
    prev->next = &ino->node;
    nxt->prev = &ino->node;
}

// remove an inode from list. Must hold lock.
// NOTE: will not free ino.
static INLINE void inode_rm_lst(Inode *ino)
{
    ListNode *prev = ino->node.prev;
    ListNode *nxt = ino->node.next;

    // validate the list.
    ASSERT(prev->next = &ino->node);
    ASSERT(nxt->prev = &ino->node);

    // do removal
    prev->next = nxt;
    nxt->prev = prev;
}

// Find the inode in the list, must hold lock
// Returns NULL if not found.
static Inode *inode_find_lst(usize ino)
{
    Inode *ret;
    ListNode *it = head.next;

    for (; it != &tail; it = it->next) {
        ret = ListEntr(it, Inode, node);
        if (ret->inode_no == ino) {
            return ret;
        }
    }

    return NULL;
}

// return which block `inode_no` lives on.
static INLINE usize to_block_no(usize inode_no)
{
    return sblock->inode_start + (inode_no / (INODE_PER_BLOCK));
}

// return the pointer to on-disk inode.
static INLINE InodeEntry *get_entry(Block *block, usize inode_no)
{
    // do not allow out-of-bound access.
    ASSERT(inode_no < INODE_PER_BLOCK);
    return ((InodeEntry *)block->data) + (inode_no % INODE_PER_BLOCK);
}

// return address array in indirect block.
static INLINE u32 *get_addrs(Block *block)
{
    return ((IndirectBlock *)block->data)->addrs;
}

// initialize inode tree.
void init_inodes(const SuperBlock *_sblock, const BlockCache *_cache)
{
    // FIXME: should use static_assertion.
    ASSERT(sizeof(IndirectBlock) == BLOCK_SIZE);
    ASSERT(INODE_MAX_BYTES >= 1 * 1024 * 1024);

    init_spinlock(&lock);
    sblock = _sblock;
    cache = _cache;

    // init list
    head.prev = NULL;
    tail.next = NULL;
    head.next = &tail;
    tail.prev = &head;

    if (ROOT_INODE_NO < sblock->num_inodes)
        inodes.root = inodes.get(ROOT_INODE_NO);
    else
        printk("(warn) init_inodes: no root inode.\n");
}

// initialize in-memory inode.
static void init_inode(Inode *inode)
{
    init_sleeplock(&inode->lock);
    init_rc(&inode->rc);
    init_list_node(&inode->node);
    inode->inode_no = 0;
    inode->valid = false;
}

// see `inode.h`.
/**
    @brief allocate a new zero-initialized inode on disk.
    @param type the type of the inode to allocate.
    @return the number of newly allocated inode.
    @throw panic if allocation fails (e.g. no more free inode).
 */
static usize inode_alloc(OpContext *ctx, InodeType type)
{
    ASSERT(type != INODE_INVALID);

    bool success = false;
    usize ret;

    // handles the first block differently,
    // for inode 0 is never used.

    Block *inoblock = cache->acquire(sblock->inode_start);
    InodeEntry *ientr;
    for (usize k = 1; k < INODE_PER_BLOCK; k++) {
        ientr = get_entry(inoblock, k);
        ASSERT(ientr != NULL);

        if (ientr->type == INODE_INVALID) {
            // set the inode to be allocated, and do sync.
            memset((void *)ientr, 0x0, sizeof(InodeEntry));
            ientr->type = type;
            cache->sync(ctx, inoblock);

            success = true;
            ret = k;
            break;
        }
    }
    cache->release(inoblock);
    if (success) {
        return ret;
    }

    for (u32 i = 1; i < sblock->num_inodes && (!success); i++) {
        inoblock = cache->acquire(sblock->inode_start + i);

        // see each inode to see if it is valid.
        for (usize k = 0; k < INODE_PER_BLOCK; k++) {
            ientr = get_entry(inoblock, k);
            ASSERT(ientr != NULL);

            if (ientr->type == INODE_INVALID) {
                // set the inode to be allocated, and do sync.
                memset((void *)ientr, 0x0, sizeof(InodeEntry));
                ientr->type = type;
                cache->sync(ctx, inoblock);

                success = true;
                ret = i * INODE_PER_BLOCK + k;
                break;
            }
        }
        cache->release(inoblock);
    }

    if (!success) {
        PANIC();
    }

    // TODO
    return ret;
}

// see `inode.h`.
/**
    @brief acquire the sleep lock of `inode`.
    - This method should be called before any write operation to `inode` and its
    file content.
    - If the inode has not been loaded, this method should load it from disk.
    @see `unlock` - the counterpart of this method.
 */
static void inode_lock(Inode *inode)
{
    ASSERT(inode->rc.count > 0);
    // TODO
    unalertable_acquire_sleeplock(&inode->lock);
    if (!inode->valid) {
        // load from disk.
        Block *block = cache->acquire(to_block_no(inode->inode_no));
        ASSERT(block->valid);
        memcpy(&inode->entry,
               get_entry(block, inode->inode_no % INODE_PER_BLOCK),
               sizeof(InodeEntry));
        cache->release(block);
        block = NULL;
        inode->valid = true;
    }
}

// see `inode.h`.
/**
    @brief release the sleep lock of `inode`.
    @see `lock` - the counterpart of this method.
 */
static void inode_unlock(Inode *inode)
{
    ASSERT(inode->rc.count > 0);
    // TODO
    release_sleeplock(&inode->lock);
}

// see `inode.h`.
/**
    @brief synchronize the content of `inode` between memory and disk.
    
    Different from block cache, this method can either read or write the inode.

    If `do_write` is true and the inode is valid, write the content of `inode` to disk.

    If `do_write` is false and the inode is invalid, read the content of `inode` from disk.

    If `do_write` is false and the inode is valid, do nothing.

    @note here "write to disk" means "sync with block cache", not "directly
    write to underneath SD card".
    @note caller must hold the lock of `inode`.
    @throw panic if `do_write` is true and `inode` is invalid.
 */
static void inode_sync(OpContext *ctx, Inode *inode, bool do_write)
{
    if (inode->entry.type == INODE_DEVICE) {
        // a device, do nothing and return.
        return;
    }

    // TODO
    ASSERT(inode->inode_no != 0);
    if (do_write) {
        Block *block = cache->acquire(to_block_no(inode->inode_no));
        memcpy((u8 *)get_entry(block, inode->inode_no % INODE_PER_BLOCK),
               &inode->entry, sizeof(InodeEntry));
        cache->sync(ctx, block);
        cache->release(block);
    } else {
        if (inode->valid) {
            // do nothing
        } else {
            // read from disk.
            Block *block = cache->acquire(to_block_no(inode->inode_no));
            ASSERT(block->valid);
            memcpy(&inode->entry,
                   (u8 *)get_entry(block, inode->inode_no % INODE_PER_BLOCK),
                   sizeof(InodeEntry));
            cache->release(block);
            block = NULL;
            inode->valid = true;
        }
    }
}

// see `inode.h`.
/**
    @brief get an inode by its inode number.
    
    This method should increment the reference count of the inode by one.

    @note it does NOT have to load the inode from disk!
    
    @return the `inode` of `inode_no`. `inode->valid` can be false.

    @see `sync` will be responsible to load the content of inode.
    @see `put` - the counterpart of this method.
 */
static Inode *inode_get(usize inode_no)
{
    ASSERT(inode_no > 0);
    ASSERT(inode_no < sblock->num_inodes);
    acquire_spinlock(&lock);
    Inode *ino = inode_find_lst(inode_no);
    release_spinlock(&lock);

    if (ino == NULL) {
        ino = kalloc(sizeof(Inode));
        ASSERT(ino != NULL);
        init_inode(ino);

        // set count to 1.
        increment_rc(&ino->rc);
        // set inode number
        ino->inode_no = inode_no;

        // add to list of inodes.
        acquire_spinlock(&lock);
        inode_push_lst(ino);
        release_spinlock(&lock);
    } else {
        increment_rc(&ino->rc);
    }
    // TODO
    return ino;
}

/**
 * Recursively free an indirect block.
 */
static void inode_rm_indir(OpContext *ctx, u32 *idx)
{
    if (*idx == 0) {
        // not mapped
        return;
    }

    Block *indir_block = cache->acquire(*idx);
    u32 *links = (u32 *)indir_block->data;

    for (usize i = 0; i < INODE_NUM_INDIRECT; i++) {
        // i <= 128
        // free data block
        if (links[i] != 0) {
            cache->free(ctx, links[i]);
            links[i] = 0;
        }
    }
    cache->sync(ctx, indir_block);
    cache->release(indir_block);

    // free the indirect block itself
    cache->free(ctx, *idx);
    // mark as deallocated
    *idx = 0;
    return;
}

/** Recursively free an doubly-indirect block */
static void inode_rm_dindir(OpContext *ctx, u32 *idx)
{
    if (*idx == 0) {
        return;
    }

    Block *dindir_block = cache->acquire(*idx);
    u32 *links = (u32 *)dindir_block->data;
    for (usize i = 0; i < INODE_NUM_INDIRECT; i++) {
        // recursively remove this "indirect block".
        inode_rm_indir(ctx, &links[i]);
    }

    cache->sync(ctx, dindir_block);
    cache->release(dindir_block);

    // free the doubly-indirect block itself
    cache->free(ctx, *idx);
    // mark as deallocated
    *idx = 0;
    return;
}

// see `inode.h`.
/**
    @brief truncate all contents of `inode`.
    
    This method removes (i.e. "frees") all file blocks of `inode`.

    @note do not forget to reset related metadata of `inode`, e.g. `inode->entry.num_bytes`.
    @note caller must hold the lock of `inode`.
 */
static void inode_clear(OpContext *ctx, Inode *inode)
{
    // TODO
    // ASSERT(inode->rc.count == 0);

    // you should call inode_lock to lock it.
    for (usize i = 0; i < INODE_NUM_DIRECT; i++) {
        // free the direct data block.
        if (inode->entry.addrs[i] != 0) {
            cache->free(ctx, inode->entry.addrs[i]);
            inode->entry.addrs[i] = 0;
        }
    }

    // free the indirect data block.
    inode_rm_indir(ctx, &(inode->entry.indirect));
    ASSERT(inode->entry.indirect == 0);
    // free doubly-indirect block
    inode_rm_dindir(ctx, &(inode->entry.dindirect));
    ASSERT(inode->entry.dindirect == 0);
    inode->entry.num_bytes = 0;
    inode_sync(ctx, inode, true);
}

// see `inode.h`.
/**
    @brief duplicate an inode.
    
    Call this if you want to share an inode with others.
    It should increment the reference count of `inode` by one.

    @return the duplicated inode (i.e. may just return `inode`).
 */
static Inode *inode_share(Inode *inode)
{
    // TODO
    ASSERT(inode != NULL);
    increment_rc(&inode->rc);
    return inode;
}

// see `inode.h`.
/**
    @brief notify that you no longer need `inode`.
    
    This method is also responsible to free the inode if no one needs it:

    "No one needs it" means it is useless BOTH in-memory (`inode->rc == 0`) and on-disk
    (`inode->entry.num_links == 0`).

    "Free the inode" means freeing all related file blocks and the inode itself.

    @note do not forget `kfree(inode)` after you have done them all!
    @note caller must NOT hold the lock of `inode`. i.e. caller should have `unlock`ed it.

    @see `get` - the counterpart of this method.
    @see `clear` can be used to free all file blocks of `inode`.
 */
static void inode_put(OpContext *ctx, Inode *inode)
{
    // TODO
    decrement_rc(&inode->rc);
    if (inode->rc.count == 0) {
        if (inode->entry.num_links == 0) {
            // remove the file entirely
            // truncate the file first.
            inode_clear(ctx, inode);
            inode->entry.type = INODE_INVALID;
        }
        inode_sync(ctx, inode, true);

        // free the inode from list.
        acquire_spinlock(&lock);
        inode_rm_lst(inode);
        kfree(inode);
        release_spinlock(&lock);
    }
}

/** Same as inode_map, but will look at indirect block 
 * @param idx pointer to the indirect pointer in inode or
 *   doubly-indirect block
 * @param[out] modified whether the value of idx is modified
 */
static usize inode_map_indirect(OpContext *ctx, u32 *idx, usize offset,
                                bool *modified)
{
    ASSERT(offset < INODE_NUM_INDIRECT * BLOCK_SIZE);
    usize ret = 0;
    if (*idx == 0) {
        // should allocate indirect block
        if (ctx == NULL) {
            // fail
            return 0;
        }

        *modified = true;
        *idx = cache->alloc(ctx);
        ASSERT(*idx != 0);
    }

    Block *indir_block = cache->acquire(*idx);
    u32 *links = (u32 *)indir_block->data;
    if (links[offset / BLOCK_SIZE] == 0 && ctx != NULL) {
        // should allocate a data block
        // do not have to mark modified, for this change does
        // not happen on inode
        links[offset / BLOCK_SIZE] = cache->alloc(ctx);
        // but, should sync this indirect block.
        cache->sync(ctx, indir_block);
    }
    ret = links[offset / BLOCK_SIZE];
    cache->release(indir_block);

    return ret;
}

/** Map at doubly-indirect block 
 * @param idx pointer to doubly-indirect pointer in inode.
 * @param[out] modified whether the value of idx is modified
 */
static usize inode_map_dindir(OpContext *ctx, u32 *idx, usize offset,
                              bool *modified)
{
    const usize BYTE_DINDIR = INODE_NUM_INDIRECT * BLOCK_SIZE;
    ASSERT(offset < INODE_NUM_DINDIRECT * BLOCK_SIZE);
    usize ret = 0;
    if (*idx == 0) {
        // should allocate doubly-indirect block
        if (ctx == NULL) {
            return 0;
        }

        *modified = true;
        *idx = cache->alloc(ctx);
        ASSERT(*idx != 0);
    }

    Block *dindir_block = cache->acquire(*idx);
    u32 *links = (u32 *)dindir_block->data;
    bool tmp __attribute__((unused));

    // since this is not modification to inode,
    // do NOT set modified.
    ret = inode_map_indirect(ctx, &links[offset / BYTE_DINDIR],
                             offset % BYTE_DINDIR, &tmp);
    if (tmp) {
        cache->sync(ctx, dindir_block);
    }
    cache->release(dindir_block);
    return ret;
}

/**
    @brief get which block is the offset of the inode in.

    e.g. `inode_map(ctx, my_inode, 1234, &modified)` will return the block_no
    of the block that contains the 1234th byte of the file
    represented by `my_inode`.

    If a block has not been allocated for that byte, `inode_map` will
    allocate a new block and update `my_inode`, at which time, `modified`
    will be set to true.

    HOWEVER, if `ctx == NULL`, `inode_map` will NOT try to allocate any new block,
    and when it finds that the block has not been allocated, it will return 0.
    
    @param[out] modified true if some new block is allocated and `inode`
    has been changed.

    @return usize the block number of that block, or 0 if `ctx == NULL` and
    the required block has not been allocated.

    @note the caller must hold the lock of `inode`.
 */
static usize inode_map(OpContext *ctx, Inode *inode, usize offset,
                       bool *modified)
{
    // TODO
    if (offset >= INODE_MAX_BYTES) {
        PANIC();
    }
    *modified = false;

    // inode must be valid. or we're reading
    // dirty data.
    ASSERT(inode->valid);

    if (offset < INODE_NUM_DIRECT * BLOCK_SIZE) {
        // search from direct block
        usize idx;
        idx = inode->entry.addrs[offset / BLOCK_SIZE];
        if (idx == 0) {
            if (ctx == NULL) {
                goto bad_ctx;
            }
            *modified = true;
            idx = inode->entry.addrs[offset / BLOCK_SIZE] = cache->alloc(ctx);
        }
        return idx;
    }

    // search from indirect block.
    offset -= INODE_NUM_DIRECT * BLOCK_SIZE;
    usize ret = 0;

    if (offset < INODE_NUM_INDIRECT * BLOCK_SIZE) {
        ret = inode_map_indirect(ctx, &(inode->entry.indirect), offset,
                                 modified);
        if (*modified) {
            ASSERT(ctx != NULL);
        }
        return ret;
    }
    offset -= INODE_NUM_INDIRECT * BLOCK_SIZE;
    ASSERT(offset < INODE_NUM_DINDIRECT * BLOCK_SIZE);
    ret = inode_map_dindir(ctx, &(inode->entry.dindirect), offset, modified);
    if (*modified) {
        ASSERT(ctx != NULL);
    }
    return ret;

bad_ctx:
    return 0;
}

// see `inode.h`.
/**
    @brief read `count` bytes from `inode`, beginning at `offset`, to `dest`.
    @return how many bytes you actually read.
    @note caller must hold the lock of `inode`.
 */
static usize inode_read(Inode *inode, u8 *dest, usize offset, usize count)
{
    InodeEntry *entry = &inode->entry;

    // specially deal with device entry.
    if (entry->type == INODE_DEVICE) {
        // lookup device table.
        ASSERT((void *)devices[entry->minor].read != NULL);
        return devices[entry->minor].read(dest, count);
    }

    if (count + offset > entry->num_bytes)
        count = entry->num_bytes - offset;
    usize end = offset + count;
    ASSERT(offset <= entry->num_bytes);
    ASSERT(end <= entry->num_bytes);
    ASSERT(offset <= end);

    bool modified;

    // what we do here:
    // 1. fill offset to a multiple of BLOCK_SIZE;
    // 2. fill consecutive blocks;
    // 3. fill the rest(if any).

    // number of bytes to read in the next op
    usize nread = BLOCK_SIZE - offset % BLOCK_SIZE;
    //  nread <- min(count, nread)
    nread = nread > count ? count : nread;
    if (nread > 0) {
        usize idx = inode_map(NULL, inode, offset, &modified);
        ASSERT(!modified);

        if (idx == 0) {
            memset(dest, 0x0, nread);
        } else {
            Block *blk_dat = cache->acquire(idx);
            memcpy(dest, (u8 *)blk_dat->data + offset % BLOCK_SIZE, nread);
            cache->release(blk_dat);
        }

        // advance
        offset += nread;
        dest += nread;
        count -= nread;
    }

    while (count >= BLOCK_SIZE) {
        // read operation do NOT write any block,
        // so a context is not needed.
        usize idx = inode_map(NULL, inode, offset, &modified);
        ASSERT(!modified);

        if (idx == 0) {
            // fake that we 'allocated' a zero-block.
            memset(dest, 0x0, BLOCK_SIZE);
        } else {
            Block *blk_dat = cache->acquire(idx);
            memcpy(dest, blk_dat->data, BLOCK_SIZE);
            cache->release(blk_dat);
        }

        // advance
        count -= BLOCK_SIZE;
        dest += BLOCK_SIZE;
        offset += BLOCK_SIZE;
        nread += BLOCK_SIZE;
    }

    if (count) {
        nread += count;
        usize idx = inode_map(NULL, inode, offset, &modified);
        Block *blk_dat = cache->acquire(idx);
        memcpy(dest, blk_dat->data, count);
        cache->release(blk_dat);
        // no need to advance.
    }

    // TODO
    return nread;
}

// see `inode.h`.
/**
    @brief write `count` bytes from `src` to `inode`, beginning at `offset`.
    @return how many bytes you actually write.
    @note caller must hold the lock of `inode`.
 */
static usize inode_write(OpContext *ctx, Inode *inode, u8 *src, usize offset,
                         usize count)
{
    InodeEntry *entry = &inode->entry;

    // specially deal with device entry.
    if (entry->type == INODE_DEVICE) {
        // lookup device table.
        ASSERT((void *)devices[entry->minor].write != NULL);
        return devices[entry->minor].write(src, count);
    }

    usize end = offset + count;
    ASSERT(offset <= entry->num_bytes);
    ASSERT(end <= INODE_MAX_BYTES);
    ASSERT(offset <= end);

    // a dirty inode continue to be dirty on write.
    bool dirty = false;
    bool modified;
    usize nwrite = BLOCK_SIZE - offset % BLOCK_SIZE;
    // nwrite <- min(nwrite, count)
    nwrite = nwrite > count ? count : nwrite;

    // pad to 512
    if (nwrite > 0) {
        usize idx = inode_map(ctx, inode, offset, &dirty);
        Block *blk_dat = cache->acquire(idx);
        memcpy((u8 *)blk_dat->data + offset % BLOCK_SIZE, src, nwrite);
        cache->sync(ctx, blk_dat);
        cache->release(blk_dat);

        // advance
        count -= nwrite;
        offset += nwrite;
        src += nwrite;
    }

    // read in blocks
    while (count >= BLOCK_SIZE) {
        usize idx = inode_map(ctx, inode, offset, &modified);
        Block *blk_dat = cache->acquire(idx);
        memcpy((u8 *)blk_dat->data, src, BLOCK_SIZE);
        cache->sync(ctx, blk_dat);
        cache->release(blk_dat);

        // advance
        if (modified) {
            dirty = true;
        }
        count -= BLOCK_SIZE;
        offset += BLOCK_SIZE;
        src += BLOCK_SIZE;
        nwrite += BLOCK_SIZE;
    }

    // finally
    if (count) {
        nwrite += count;
        usize idx = inode_map(ctx, inode, offset, &modified);
        Block *blk_dat = cache->acquire(idx);
        memcpy((u8 *)blk_dat->data, src, count);
        cache->sync(ctx, blk_dat);
        cache->release(blk_dat);
        // BUG: should update offset for it's used later.
        offset += count;
    }

    // reset file offset
    if (offset > entry->num_bytes) {
        entry->num_bytes = offset;
        dirty = true;
    }

    // now, if the inode is written, sync.
    if (dirty) {
        inode_sync(ctx, inode, true);
    }

    // TODO
    return nwrite;
}

// see `inode.h`.
/**
    @brief look up an entry named `name` in directory `inode`.

    @param[out] index the index of found entry in this directory.

    @return the inode number of the corresponding inode, or 0 if not found.
    
    @note caller must hold the lock of `inode`.

    @throw panic if `inode` is not a directory.
 */
static usize inode_lookup(Inode *inode, const char *name, usize *index)
{
    InodeEntry *entry = &inode->entry;
    ASSERT(entry->type == INODE_DIRECTORY);

    usize offset = 0;
    usize rest = inode->entry.num_bytes;
    ASSERT(inode->entry.num_bytes % sizeof(DirEntry) == 0);
    while (rest >= BLOCK_SIZE) {
        // can read 512 / 16 = 32 inodes a time!
        // we read directly in the cache to avoid
        // memory copy.

        bool modified;
        usize idx = inode_map(NULL, inode, offset, &modified);
        ASSERT(!modified);
        Block *block = cache->acquire(idx);
        DirEntry *de = (DirEntry *)block->data;
        for (usize i = 0; i < BLOCK_SIZE / sizeof(DirEntry); i++) {
            if (strncmp(name, de[i].name, FILE_NAME_MAX_LENGTH) == 0) {
                // found!
                cache->release(block);
                if (index != NULL) {
                    *index = i + offset / sizeof(DirEntry);
                }
                return de[i].inode_no;
            }
        }
        cache->release(block);

        // advance
        offset += BLOCK_SIZE;
        rest -= BLOCK_SIZE;
    }

    // optimization: check rest to avoid a cache acquire.
    if (rest == 0) {
        return 0;
    }

    // read the rest.
    bool modified;
    usize idx = inode_map(NULL, inode, offset, &modified);
    ASSERT(!modified);
    Block *block = cache->acquire(idx);
    DirEntry *de = (DirEntry *)block->data;
    for (usize i = 0; i < rest / sizeof(DirEntry); i++) {
        if (strncmp(name, de[i].name, FILE_NAME_MAX_LENGTH) == 0) {
            cache->release(block);
            if (index != NULL) {
                *index = i + offset / sizeof(DirEntry);
            }
            return de[i].inode_no;
        }
    }
    cache->release(block);

    // TODO
    return 0;
}

// see `inode.h`.
/**
    @brief insert a new directory entry in directory `inode`.
    Add a new directory entry in `inode` called `name`, which points to inode 
    with `inode_no`.

    @return the index of new directory entry, or -1 if `name` already exists.

    @note if the directory inode is full, you should grow the size of directory inode.
    @note you do NOT need to change `inode->entry.num_links`. Another function
    to be finished in our final lab will do this.
    @note caller must hold the lock of `inode`.

    @throw panic if `inode` is not a directory.
 */
static usize inode_insert(OpContext *ctx, Inode *inode, const char *name,
                          usize inode_no)
{
    InodeEntry *entry = &inode->entry;
    ASSERT(entry->type == INODE_DIRECTORY);
    ASSERT(inode_no != 0);

    ASSERT(entry->num_bytes % sizeof(DirEntry) == 0);
    // check the existence of name.
    usize index;
    if (inode_lookup(inode, name, &index) != 0) {
        // name exists already.
        return -1;
    }
    index = 0;

    // now only have to find a slot to fit in the entry.

    usize offset = 0;
    usize rest = inode->entry.num_bytes;
    bool modified = false;
    ASSERT(inode->entry.num_bytes % sizeof(DirEntry) == 0);

    // should write at most 1 data block.
    while (rest >= BLOCK_SIZE) {
        usize idx = inode_map(ctx, inode, offset, &modified);
        Block *block = cache->acquire(idx);
        DirEntry *de = (DirEntry *)block->data;

        for (usize i = 0; i < BLOCK_SIZE / sizeof(DirEntry); i++) {
            if (de[i].inode_no == 0) {
                // found one
                strncpy(de[i].name, name, FILE_NAME_MAX_LENGTH);
                de[i].inode_no = inode_no;
                cache->sync(ctx, block);
                cache->release(block);
                return index + i;
            }
        }
        cache->release(block);

        // advance.
        index += BLOCK_SIZE / sizeof(DirEntry);
        offset += BLOCK_SIZE;
        rest -= BLOCK_SIZE;
    }

    if (rest) {
        usize idx = inode_map(ctx, inode, offset, &modified);
        Block *block = cache->acquire(idx);
        DirEntry *de = (DirEntry *)block->data;

        for (usize i = 0; i < rest / sizeof(DirEntry); i++) {
            if (de[i].inode_no == 0) {
                // found one
                strncpy(de[i].name, name, FILE_NAME_MAX_LENGTH);
                de[i].inode_no = inode_no;
                cache->sync(ctx, block);
                cache->release(block);
                return index + i;
            }
        }
        cache->release(block);
        offset += rest;
        index += rest / sizeof(DirEntry);
    }

    // grow the dir by one direntry.
    // at the end of the dir.
    ASSERT(offset == entry->num_bytes);
    DirEntry *de = kalloc(sizeof(DirEntry));
    strncpy(de->name, name, FILE_NAME_MAX_LENGTH);
    de->inode_no = inode_no;
    inode_write(ctx, inode, (u8 *)de, offset, sizeof(DirEntry));
    kfree(de);

    // TODO
    return index;
}

// see `inode.h`.
/**
    @brief remove the directory entry at `index`.
    If the corresponding entry is not used before, `remove` does nothing.

    @note if the last entry is removed, you can shrink the size of directory inode.
    If you like, you can also move entries to fill the hole.

    @note caller must hold the lock of `inode`.

    @throw panic if `inode` is not a directory.
 */
static void inode_remove(OpContext *ctx, Inode *inode, usize index)
{
    // TODO
    InodeEntry *entry = &inode->entry;
    ASSERT(entry->type == INODE_DIRECTORY);

    // compute the offset of index i.
    usize offset = index * sizeof(DirEntry);
    DirEntry *de = kalloc(sizeof(DirEntry));
    inode_read(inode, (u8 *)de, offset, sizeof(DirEntry));
    de->name[0] = 0;
    de->inode_no = 0;
    inode_write(ctx, inode, (u8 *)de, offset, sizeof(DirEntry));
    kfree(de);
}

InodeTree inodes = {
    .alloc = inode_alloc,
    .lock = inode_lock,
    .unlock = inode_unlock,
    .sync = inode_sync,
    .get = inode_get,
    .clear = inode_clear,
    .share = inode_share,
    .put = inode_put,
    .read = inode_read,
    .write = inode_write,
    .lookup = inode_lookup,
    .insert = inode_insert,
    .remove = inode_remove,
};