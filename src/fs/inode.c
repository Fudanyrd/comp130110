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
static const SuperBlock* sblock;

/**
    @brief the reference to the underlying block cache.
 */
static const BlockCache* cache;

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

// push an inode into list. Must hold lock.
static INLINE void inode_push_lst(Inode *ino) {
    ListNode *prev = head.next->prev; 
    ListNode *nxt = head.next;
    ino->node.prev = prev;
    ino->node.next = nxt;
}

// remove an inode from list. Must hold lock.
// NOTE: will not free ino.
static INLINE void inode_rm_lst(Inode *ino) {
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
static Inode *inode_find_lst(usize ino) {
    Inode *ret;
    ListNode *it = head.next;

    for (; it != &tail; it = it->next) {
        ret = ListEntr(it, Inode, node);
        ASSERT(ret->valid);
        if (ret->inode_no == ino) {
            return ret;
        }
    }

    return NULL;
}

// return which block `inode_no` lives on.
static INLINE usize to_block_no(usize inode_no) {
    return sblock->inode_start + (inode_no / (INODE_PER_BLOCK));
}

// return the pointer to on-disk inode.
static INLINE InodeEntry* get_entry(Block* block, usize inode_no) {
    // do not allow out-of-bound access.
    ASSERT(inode_no < INODE_PER_BLOCK);
    return ((InodeEntry*)block->data) + (inode_no % INODE_PER_BLOCK);
}

// return address array in indirect block.
static INLINE u32* get_addrs(Block* block) {
    return ((IndirectBlock*)block->data)->addrs;
}

// initialize inode tree.
void init_inodes(const SuperBlock* _sblock, const BlockCache* _cache) {
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
static void init_inode(Inode* inode) {
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
static usize inode_alloc(OpContext* ctx, InodeType type) {
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
static void inode_lock(Inode* inode) {
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
static void inode_unlock(Inode* inode) {
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
static void inode_sync(OpContext* ctx, Inode* inode, bool do_write) {
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
            Block *block = cache->acquire(inode->inode_no);
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
static Inode* inode_get(usize inode_no) {
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

// see `inode.h`.
/**
    @brief truncate all contents of `inode`.
    
    This method removes (i.e. "frees") all file blocks of `inode`.

    @note do not forget to reset related metadata of `inode`, e.g. `inode->entry.num_bytes`.
    @note caller must hold the lock of `inode`.
 */
static void inode_clear(OpContext* ctx, Inode* inode) {
    // TODO
    ASSERT(inode->rc.count == 0);

    // you should call inode_lock to lock it.
    for (usize i = 0; i < INODE_NUM_DIRECT; i++) {
        // free the direct data block.
        if (inode->entry.addrs[i] != 0) {
            cache->free(ctx, inode->entry.addrs[i]);
            inode->entry.addrs[i] = 0;
        }
    }

    // free the indirect data block.
    usize indirect = inode->entry.indirect;
    if (indirect != 0) {
        Block *indir = cache->acquire(indirect);
        u32 *arr = (u32 *)indir->data;
        for (usize i = 0; i < INODE_NUM_INDIRECT; i++) {
            if (arr[i] != 0) {
                cache->free(ctx, arr[i]);
            }
        }
        cache->release(indirect);
        cache->free(ctx, indirect);
    }
    inode->entry.indirect = 0;
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
static Inode* inode_share(Inode* inode) {
    // TODO
    ASSERT(inode != NULL);
    increment_rc(&inode->rc);
    return inode;
    return 0;
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
static void inode_put(OpContext* ctx, Inode* inode) {
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
        kfree(inode);
        release_spinlock(&lock);
    }
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
static usize inode_map(OpContext* ctx,
                       Inode* inode,
                       usize offset,
                       bool* modified) {
    // TODO
    return 0;
}

// see `inode.h`.
static usize inode_read(Inode* inode, u8* dest, usize offset, usize count) {
    InodeEntry* entry = &inode->entry;
    if (count + offset > entry->num_bytes)
        count = entry->num_bytes - offset;
    usize end = offset + count;
    ASSERT(offset <= entry->num_bytes);
    ASSERT(end <= entry->num_bytes);
    ASSERT(offset <= end);

    // TODO
    return 0;
}

// see `inode.h`.
static usize inode_write(OpContext* ctx,
                         Inode* inode,
                         u8* src,
                         usize offset,
                         usize count) {
    InodeEntry* entry = &inode->entry;
    usize end = offset + count;
    ASSERT(offset <= entry->num_bytes);
    ASSERT(end <= INODE_MAX_BYTES);
    ASSERT(offset <= end);

    // TODO
    return 0;
}

// see `inode.h`.
static usize inode_lookup(Inode* inode, const char* name, usize* index) {
    InodeEntry* entry = &inode->entry;
    ASSERT(entry->type == INODE_DIRECTORY);

    // TODO
    return 0;
}

// see `inode.h`.
static usize inode_insert(OpContext* ctx,
                          Inode* inode,
                          const char* name,
                          usize inode_no) {
    InodeEntry* entry = &inode->entry;
    ASSERT(entry->type == INODE_DIRECTORY);

    // TODO
    return 0;
}

// see `inode.h`.
static void inode_remove(OpContext* ctx, Inode* inode, usize index) {
    // TODO
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