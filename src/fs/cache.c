#include <common/bitmap.h>
#include <common/string.h>
#include <fs/cache.h>
#include <kernel/mem.h>
#include <kernel/printk.h>
#include <kernel/proc.h>
#include <common/sem.h>

#undef acquire_sleeplock
#define acquire_sleeplock unalertable_acquire_sleeplock

// clang-format off
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                       CONDVAR IMPLEMENTATION
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
// clang-format on

struct condvar {
    u32 waitcnt; // number of waiting threads
    Semaphore sema;
};

static inline void cond_init(struct condvar *cv)
{
    init_sem(&cv->sema, 0);
    cv->waitcnt = 0;
}

static void cond_wait(struct condvar *cv, SpinLock *lock)
{
    _lock_sem(&cv->sema);
    // incr # waiting
    cv->waitcnt++;
    // release holding lock, and goto sleep
    release_spinlock(lock);
    unalertable_wait_sem(&cv->sema);
    _unlock_sem(&cv->sema);
    // reacquire lock
    acquire_spinlock(lock);
}

// wake up only one sleeping thread(if any)
static void cond_signal(struct condvar *cv)
{
    _lock_sem(&cv->sema);
    if (cv->waitcnt == 0) {
        _unlock_sem(&cv->sema);
        return;
    }
    cv->waitcnt--;
    _post_sem(&cv->sema);
    _unlock_sem(&cv->sema);
}

// wake up all sleeping thread
static void cond_broadcast(struct condvar *cv)
{
    _lock_sem(&cv->sema);
    for (u32 i = 0; i < cv->waitcnt; i++) {
        _post_sem(&cv->sema);
    }
    cv->waitcnt = 0;
    _unlock_sem(&cv->sema);
}

// clang-format off
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                       GLOBAL VARIABLES
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
// clang-format on

/** Cache's timestamp */
static u64 cache_ts;

/** Number of cache's avaiable slots, i.e. count(!slot->valid); */
static u32 cache_avail;

/** the private reference to the super block.
    Note: we need these two variables because we allow the caller to
          specify the block device and super block to use.
          Correspondingly, you should NEVER use global instance of
          them, e.g. `get_super_block`, `block_device`

    see: init_bcache */
static const SuperBlock *sblock;

/** the reference to the underlying block device. */
static const BlockDevice *device;

/** global lock for block cache.
  Use it to protect anything you need.
  e.g. the list of allocated blocks, etc.  */
static SpinLock cache_lock;

#define CACHE_SIZE 140

/** the list of all allocated in-memory block.
 * 
 *  We use a linked list to manage all allocated cached blocks.
 *  You can implement your own data structure if you like better performance.
 *
 *  see: Block
 */
static Block blocks[CACHE_SIZE];
/** Number of allocated blocks */
static i32 nblock;

// clang-format off
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                       LOGGER IMPLEMENTATION
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
// clang-format on

static LogHeader header; // in-memory copy of log header block.

/** a struct to maintain other logging states, it deals
 * one and only one transactions at a time.
 *  
 * You may wonder where we store some states, e.g.
 * + how many atomic operations are running?
 * + are we checkpointing?
 * + how to notify `end_op` that a checkpoint is done?
 * Put them here!
 *
 * @see cache_begin_op, cache_end_op, cache_sync
 */
struct logger {
    /* These are read-only, so lock does not to be held */
    usize start; // start block of log(to put header)
    usize size; // number of log blocks
            // (=min{LOG_MAX_SIZE, SuperBlock::num_log - 1})
    SpinLock lock; // protects log header
    struct condvar cv; // sleep/wakeup as xv6
    /* Must hold lock when accessing the log header */
    usize outstanding; // number of uncommitted trans
    int committing; // 1 if the logger is busy committing.
    /* your fields here */
} log;

// read the content from disk.
static INLINE void device_read(Block *block)
{
    ASSERT(block != NULL);
    ASSERT(block->block_no < sblock->num_blocks);
    device->read(block->block_no, block->data);
}

// write the content back to disk.
static INLINE void device_write(Block *block)
{
    ASSERT(block != NULL);
    ASSERT(block->block_no < sblock->num_blocks);
    device->write(block->block_no, block->data);
}

// read log header from disk.
static INLINE void read_header()
{
    ASSERT(sblock != NULL);
    device->read(sblock->log_start, (u8 *)&header);
}

// write log header back to disk.
static INLINE void write_header()
{
    ASSERT(sblock != NULL);
    device->write(sblock->log_start, (u8 *)&header);
}

static void logger_init(const SuperBlock *sb)
{
    STATIC_ASSERT(sizeof(SuperBlock) <= BLOCK_SIZE);

    log.committing = 0;
    log.outstanding = 0;
    log.start = sb->log_start;
    if (sb->num_log_blocks < 2) {
        PANIC("too few log blocks");
    }
    log.size = sb->num_log_blocks;
    log.size = log.size > LOG_MAX_SIZE ? LOG_MAX_SIZE : log.size;

    // it has to support at least one transaction
    ASSERT(log.size >= OP_MAX_NUM_BLOCKS);
    init_spinlock(&log.lock);
    cond_init(&log.cv);
}

static Block *cache_acquire(usize block_no);
static void cache_release(Block *);
static void cache_pin(Block *b);
static void cache_unpin(Block *b);

// xv6 install_trans(), will unpin block
static void install_trans(bool recovering)
{
    ASSERT(recovering || log.committing);
    if (recovering) {
        // read, write directly from disk.
        // since cache is empty
        static u8 buf[BLOCK_SIZE];
        for (usize i = 0; i < header.num_blocks; i++) {
            device->read(i + log.start + 1, buf);
            device->write(header.block_no[i], buf);
        }
    } else {
        Block *src;
        // the data is (probably) still in the buffer
        // this will avoid one read of log area
        for (usize i = 0; i < header.num_blocks; i++) {
            src = cache_acquire(header.block_no[i]);
            device->write(src->block_no, src->data);
            // it is pinned previously in logger_write()
            cache_unpin(src);
            cache_release(src);
        }
    }

    // clear the log
    header.num_blocks = 0;
    write_header();
}

// xv6 write_log()
static void logger_write_log()
{
    Block *dirty;
    ASSERT(header.num_blocks <= log.size);
    for (usize i = 0; i < header.num_blocks; i++) {
        // acquire() does not return NULL
        dirty = cache_acquire(header.block_no[i]);
        device->write(log.start + 1 + i, dirty->data);
        cache_release(dirty);
    }
}

// xv6 style commit
static void logger_commit()
{
    if (header.num_blocks > 0) {
        // write log, then header
        logger_write_log();
        write_header();
        // install transaction
        install_trans(false);

        // clear the log area
        header.num_blocks = 0;
        write_header();
    }
}

// xv6 style begin_op
static void logger_begin()
{
    acquire_spinlock(&log.lock);
    while (1) {
        if (log.committing) {
            // committing, a time consuming work
            // in xv6, sleep(&log, &log.lock);
            cond_wait(&log.cv, &log.lock);
        } else {
            if ((log.outstanding + 1) * OP_MAX_NUM_BLOCKS + header.num_blocks >
                log.size) {
                // there may not be enough room to hold the transaction,
                // wait till commit
                // sleep(&log, &log.lock)
                cond_wait(&log.cv, &log.lock);
            } else {
                log.outstanding += 1;
                release_spinlock(&log.lock);
                break;
            }
        }
    }
}

// xv6 log_write(), will pin block
static void logger_write(Block *b)
{
    acquire_spinlock(&log.lock);
    if (header.num_blocks > log.size) {
        PANIC("transaction too big");
    }
    if (log.outstanding < 1) {
        PANIC("log outside transaction");
    }

    usize idx = header.num_blocks;
    for (usize i = 0; i < header.num_blocks; i++) {
        if (header.block_no[i] == b->block_no) {
            idx = i;
            break;
        }
    }

    // add a new block?
    if (header.num_blocks == idx) {
        header.block_no[idx] = b->block_no;
        header.num_blocks++;
        cache_pin(b);
    }
    release_spinlock(&log.lock);
}

// xv6 style end_op
static void logger_end()
{
    int do_commit = 0;

    acquire_spinlock(&log.lock);
    ASSERT(log.outstanding > 0);
    log.outstanding--;

    if (log.committing) {
        // FIXME: think about why panic
        PANIC("commit at end");
    }
    if (log.outstanding == 0) {
        do_commit = 1;
        log.committing = 1;
    } else {
        // since outstanding job decrement, can
        // wakeup(&log.cv);
        cond_signal(&log.cv);
    }
    release_spinlock(&log.lock);

    if (do_commit) {
        // do not hold spinlock while committing
        logger_commit();

        // notify others that commit is done.
        acquire_spinlock(&log.lock);
        log.committing = 0;
        cond_broadcast(&log.cv);
        release_spinlock(&log.lock);
    }
}

// clang-format off
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *                      BLOCK CACHE IMPLEMENTATION
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
// clang-format on

// initialize a block struct.
static void init_block(Block *block)
{
    block->block_no = 0;
    block->acquired = false;
    block->pinned = 0;

    init_sleeplock(&block->lock);
    block->valid = false;
    block->timestamp = 0;
    memset(block->data, 0, sizeof(block->data));
}

// see `cache.h`.
static usize get_num_cached_blocks()
{
    acquire_spinlock(&cache_lock);
    ASSERT(cache_avail <= EVICTION_THRESHOLD);
    usize ret = (usize)cache_avail;
    release_spinlock(&cache_lock);
    // TODO
    return ret;
}

/** Fetch the block blockno, and acquire the sleep lock. */
static Block *cache_acquire(usize block_no)
{
    Block *ret = NULL;
    u64 ts;

    /** See if need to do eviction */
    acquire_spinlock(&cache_lock);
    ts = cache_ts;
    cache_ts++;

    /**
     * Specification: 
     * + if there's block_no in the cache, use it;
     * + else if there's uninialized block, use it;
     * + else select the unused block with least timestamp;
     */
    for (int i = 0; i < nblock; i++) {
        if (blocks[i].block_no == block_no) {
            ret = &blocks[i];
            break;
        }

        if (blocks[i].pinned != 0) {
            // do not use allocated
            continue;
        }

        // reuse the slot with least timestamp
        // these with timestamp = 0 is not initialized,
        // hence will be selected first.
        if (ret == NULL || ret->timestamp > blocks[i].timestamp) {
            // lru replacement
            ret = &blocks[i];
        }
    }

    // increment cache volume
    if (ret == NULL && nblock < CACHE_SIZE) {
        ret = &blocks[nblock];
        nblock++;
    }

    // use a new frame
    if (ret != NULL && ret->block_no != block_no) {
        ret->block_no = block_no;
        ret->valid = false;
        ASSERT(ret->pinned == 0);
    }

    if (ret != NULL) {
        // pin the block
        ret->pinned++;
        // update timestamp
        ret->timestamp = ts;
        ret->acquired = true;
        release_spinlock(&cache_lock);

        ASSERT(ret->block_no == block_no);
        acquire_sleeplock(&ret->lock);

        // if not valid, read the content from disk
        if (!ret->valid) {
            device->read(block_no, (u8 *)ret->data);
            ret->valid = true;
        }
    } else {
        PANIC("buffer full");
    }
    // TODO
    return ret;
}

// see `cache.h`.
static void cache_release(Block *block)
{
    // TODO
    release_sleeplock(&block->lock);

    // unpin the block
    acquire_spinlock(&cache_lock);
    ASSERT(block->pinned > 0);
    block->pinned--;
    block->acquired = false;
    release_spinlock(&cache_lock);
}

// see `cache.h`.
void init_bcache(const SuperBlock *_sblock, const BlockDevice *_device)
{
    sblock = _sblock;
    device = _device;
    ASSERT(sblock != NULL && device != NULL);

    // set initial timestamp
    cache_ts = 1;

    // initialize private members
    for (int i = 0; i < CACHE_SIZE; i++) {
        init_block(&blocks[i]);
    }
    init_spinlock(&cache_lock);
    cache_avail = EVICTION_THRESHOLD;
    nblock = EVICTION_THRESHOLD;

    // read header and initialize logger
    read_header();
    logger_init(_sblock);
    ASSERT(header.num_blocks <= log.size);

    // do crash recovery
    install_trans(true);
}

// see `cache.h`.
static void cache_begin_op(OpContext *ctx)
{
    // TODO
    init_spinlock(&ctx->lock);
    ctx->num_blocks = 0;
    memset(ctx->bwrite, 0, sizeof(ctx->bwrite));

    // notify logger of a trans
    logger_begin();
}

// see `cache.h`.
static void cache_sync(OpContext *ctx, Block *block)
{
    if (ctx == NULL) {
        // by api doc, should write back.
        device_write(block);
        return;
    }

    // update timestamp
    acquire_spinlock(&cache_lock);
    block->timestamp = cache_ts++;
    release_spinlock(&cache_lock);

    // check overflow(see test_overflow())
    acquire_spinlock(&ctx->lock);
    bool found = false;
    for (usize i = 0; i < ctx->num_blocks; i++) {
        Block *cur = ctx->bwrite[i];
        ASSERT(cur != NULL);
        if (block->block_no == cur->block_no) {
            ASSERT(block == cur);
            found = true;
            break;
        }
    }
    if (!found) {
        // overflow the array
        if (ctx->num_blocks == OP_MAX_NUM_BLOCKS) {
            PANIC("ctx overflow: trans too large");
        }
        // record in the ctx
        ctx->bwrite[ctx->num_blocks] = block;
        ctx->num_blocks++;
    }
    release_spinlock(&ctx->lock);

    // notify logger
    logger_write(block);

    // TODO
}

// see `cache.h`.
static void cache_end_op(OpContext *ctx)
{
    // TODO
    logger_end();
    ctx->num_blocks = 0;
}

static void cache_pin(Block *block)
{
    acquire_spinlock(&cache_lock);
    block->pinned++;
    release_spinlock(&cache_lock);
}
static void cache_unpin(Block *block)
{
    acquire_spinlock(&cache_lock);
    ASSERT(block->pinned);
    block->pinned--;
    release_spinlock(&cache_lock);
}

/** Number of bits per block */
#define BITS_PER_BLOCK (BLOCK_SIZE * 8)

// see `cache.h`.
static usize cache_alloc(OpContext *ctx)
{
    // TODO
    ASSERT(sblock != NULL);

    /* A bitmap block */
    Block *bm = NULL;
    usize ret = 0;
    /* size of bitmap area(see block_device.ipp) */
    u32 bm_size =
            (sblock->num_data_blocks + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;
    bool found = false;

    // read each bitmap block to find a sector
    for (u32 i = 0; i < bm_size; i++) {
        bm = cache_acquire(sblock->bitmap_start + i);
        BitmapCell *cell = (BitmapCell *)(bm->data);

        for (usize k = 0; k < BITS_PER_BLOCK; k++) {
            if (!bitmap_get(cell, k)) {
                ret += k;
                found = true;
                break;
            }
        }

        // if found the bit
        if (found) {
            if (ret < sblock->num_blocks) {
                bitmap_set(cell, ret % BIT_PER_BLOCK);
                cache_sync(ctx, bm);
                cache_release(bm);

                // init with zero
                Block *zero = cache_acquire(ret);
                memset(zero->data, 0, BLOCK_SIZE);
                cache_sync(ctx, zero);
                cache_release(zero);
            } else {
                PANIC("disk full");
            }
            break;
        } else {
            // release the block
            cache_release(bm);
        }
        ret += BITS_PER_BLOCK;
    }

    if (!found) {
        // FIXME: when no available blocks can be found,
        // wait instead of just panic.
        PANIC("disk full");
    }
    return ret;
}

// see `cache.h`.
static void cache_free(OpContext *ctx, usize block_no)
{
    // TODO
    ASSERT(sblock != NULL);
    ASSERT(block_no < sblock->num_blocks);

    u32 bm_off = block_no / BITS_PER_BLOCK;
    usize idx = block_no % BITS_PER_BLOCK;
    Block *bm = cache_acquire(bm_off + sblock->bitmap_start);
    BitmapCell *cell = (BitmapCell *)(bm->data);
    ASSERT(bitmap_get(cell, idx));
    bitmap_clear(cell, idx);
    cache_sync(ctx, bm);
    cache_release(bm);

    return;
}

BlockCache bcache = {
    .get_num_cached_blocks = get_num_cached_blocks,
    .acquire = cache_acquire,
    .release = cache_release,
    .begin_op = cache_begin_op,
    .sync = cache_sync,
    .end_op = cache_end_op,
    .alloc = cache_alloc,
    .free = cache_free,
};