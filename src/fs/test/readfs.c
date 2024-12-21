#include <fs/defines.h>
#include <fs/inode.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

// clang-format off
/**
 * Make a 8MB disk, that is 16K blocks!
 * [sb(1) | (log:64) | (inode:1024) | (bitmap:4) | (data:rest) ]
 *        ^1         ^65            ^1089        ^1093         ^8388608
 */
// clang-format on

int disk;
static SuperBlock sb;
static BlockCache bc;
static usize alloc_no;
extern InodeTree inodes;

/** Create mock cache */
static usize get_num_cached_blocks(void);
static Block *acquire(usize block_no);
static void release(Block *block);
static void begin_op(OpContext *ctx);
static void cache_sync(OpContext *ctx, Block *block);
static void end_op(OpContext *ctx);
static usize cache_alloc(OpContext *ctx);
static void cache_free(OpContext *ctx, usize block_no);
static void bitmap_set(uint64_t *buf, usize idx);
static void check_file(const char *fname);

int main(int argc, char **argv)
{
    const char *dname = "sd.img";
    disk = open(dname, O_RDONLY);
    assert(disk >= 0);

    // init superblock
    assert(lseek(disk, 0, SEEK_SET) == 0);
    assert(read(disk, &sb, sizeof(sb)) == sizeof(sb));

    // init block cache
    bc.acquire = acquire;
    bc.alloc = cache_alloc;
    bc.begin_op = begin_op;
    bc.end_op = end_op;
    bc.free = cache_free;
    bc.get_num_cached_blocks = get_num_cached_blocks;
    bc.release = release;
    bc.sync = cache_sync;

    // init alloc_no
    alloc_no = 1089;

    // init inode tree
    init_inodes(&sb, &bc);

    for (int i = 1; i < argc; i++) {
        check_file(argv[i]);
    }

    end_op(NULL);
    close(disk);
    return 0;
}

static usize get_num_cached_blocks(void)
{
    return 0;
}

static Block *acquire(usize block_no)
{
    Block *block = malloc(sizeof(Block));
    assert(block != NULL);
    off_t offset = BLOCK_SIZE * block_no;
    assert(lseek(disk, offset, SEEK_SET) == offset);

    // init
    assert(read(disk, block->data, BLOCK_SIZE) >= 0);
    block->valid = true;
    block->block_no = block_no;
    return block;
}

static void release(Block *block)
{
    // write to file
    free(block);
}

static void begin_op(OpContext *ctx)
{
    return;
}

static void cache_sync(OpContext *ctx, Block *block)
{
    return;
}

static void end_op(OpContext *ctx)
{
    return;
}

static usize cache_alloc(OpContext *ctx)
{
    fprintf(stderr, "readfs does not do write\n");
    assert(0);
}

static void cache_free(OpContext *ctx, usize block_no)
{
    fprintf(stderr, "Fatal: should not free block when making disk image!\n");
    exit(1);
}

static void bitmap_set(uint64_t *buf, usize idx)
{
    assert(idx < BIT_PER_BLOCK);
#define BIT_PER_ELEM (sizeof(uint64_t) * 8)
    usize offset = idx / BIT_PER_ELEM;
    buf[offset] |= (1UL << (idx % BIT_PER_ELEM));
}

static Inode *file2inode(const char *fname)
{
    Inode *rt = inodes.root;
    inodes.sync(NULL, rt, false);
    assert(rt->valid);
    assert(rt->entry.type == INODE_DIRECTORY);
    if (strcmp(fname, "/") == 0) {
        return rt;
    }

    usize tmp __attribute__((unused));
    usize idx = inodes.lookup(rt, fname, &tmp);

    if (idx == 0) {
        fprintf(stderr, "%s: No such file or directory.\n", fname);
        exit(1);
    }
    Inode *ret = inodes.get(idx);
    inodes.sync(NULL, ret, false);
    return ret;
}

static void check_file(const char *fname)
{
    assert(strlen(fname) < FILE_NAME_MAX_LENGTH);
    static uint8_t buf[512];

    Inode *ino = file2inode(fname);

    usize offset = 0;
    usize nread = inodes.read(ino, buf, offset, sizeof(buf));
    while (nread > 0) {
        write(1, buf, nread);
        offset += nread;
        nread = inodes.read(ino, buf, offset, sizeof(buf));
    }
}
