#include <fs/defines.h>
#include <fs/inode.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include "mock/file.h"

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
static void check_file_or_dir(const char *fname);

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
    // init file system
    file_init(NULL, &bc);

    for (int i = 1; i < argc; i++) {
        check_file_or_dir(argv[i]);
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

static void check_file_or_dir(const char *fname)
{
    static char buf[512];

    int fd = sys_open(fname, F_RDONLY);

    // print inode info
    static InodeEntry entry;
    assert(sys_inode(fd, &entry) == 0);
    printf("{type: %u, nlink: %u, size: %u}\n", (u32)entry.type,
           (u32)entry.num_links, entry.num_bytes);

    if (entry.type == INODE_DIRECTORY) {
        static DirEntry dentr;
        while (sys_readdir(fd, &dentr) == 0) {
            printf("{inode: %d, name: %s}\n", dentr.inode_no, dentr.name);
        }
    } else {
        long nr = 0;
        while ((nr = sys_read(fd, buf, sizeof(buf))) > 0) {
            write(1, buf, nr);
        }
    }

    sys_close(fd);
}
