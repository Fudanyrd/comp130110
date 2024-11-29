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
static OpContext ctx;
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
static void shell(void);

#define BIT_PER_ELEM (sizeof(uint64_t) * 8)
static inline bool bitmap_get(uint64_t *buf, usize idx)
{
    assert(idx < BIT_PER_BLOCK);
    usize off = idx / BIT_PER_ELEM;
    return (buf[off] & (1UL << (idx % BIT_PER_ELEM))) != 0;
}
static void bitmap_set(uint64_t *buf, usize idx)
{
    assert(idx < BIT_PER_BLOCK);
    usize offset = idx / BIT_PER_ELEM;
    buf[offset] |= (1UL << (idx % BIT_PER_ELEM));
}
static void bitmap_clear(uint64_t *buf, usize idx)
{
    assert(idx < BIT_PER_BLOCK);
    usize offset = idx / BIT_PER_ELEM;
    buf[offset] &= (~(1UL << (idx % BIT_PER_ELEM)));
}

int main(int argc, char **argv)
{
    const char *dname = "sd.img";
    disk = open(dname, O_RDWR);
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
    file_init(&ctx, &bc);

    shell();

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
    off_t offset = BLOCK_SIZE * block->block_no;
    assert(lseek(disk, offset, SEEK_SET) == offset);
    assert(write(disk, block->data, BLOCK_SIZE) >= 0);
    return;
}

static void end_op(OpContext *ctx)
{
    return;
}

static usize cache_alloc(OpContext *ctx)
{
    // allocate bitmap block
    usize ret = -1;
    for (int i = 0; i < 4; i++) {
        Block *bm = acquire(sb.bitmap_start + i);
        for (usize k = 0; k < BIT_PER_BLOCK; k++) {
            if (!bitmap_get((uint64_t *)bm->data, k)) {
                bitmap_set((uint64_t *)bm->data, k);
                cache_sync(ctx, bm);
                ret = k + i * BIT_PER_BLOCK;
            }
        }
        release(bm);
        if (ret != (usize)-1) {
            return ret;
        }
    }

    // cannot allocate
    fprintf(stderr, "disk full\n");
    assert(0);
}

static void cache_free(OpContext *ctx, usize block_no)
{
    Block *bm = acquire(sb.bitmap_start + block_no / BIT_PER_BLOCK);
    bitmap_clear((uint64_t *)bm->data, block_no % BIT_PER_BLOCK);
    cache_sync(ctx, bm);
    release(bm);
}

static void check_file_or_dir(const char *fname)
{
    static char buf[512];

    int fd = sys_open(fname, F_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "%s: No such file or directory.\n", fname);
        return;
    }

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

static void shell(void)
{
    static char buf[512];

    char *m = malloc(sizeof(buf));

    char *dst;
    char *src;

    write(1, "< ", 2);
    while (fgets(buf, sizeof(buf), stdin)) {
        // split input
        dst = (char *)buf + 2;
        src = dst;
        while (*src != ' ') {
            src++;
        }
        *src = 0;
        src++;

        // null terminate
        char *pt = src;
        while (*pt != '\n') {
            pt++;
        }
        *pt = 0;

        switch (buf[0]) {
        case 'm': {
            // mkdir
            // usage: m dest 0
            printf("%d\n", sys_mkdir(dst));
            break;
        }
        case 'u': {
            // unlink file
            // usage: m dest 0
            printf("%d\n", sys_unlink(dst));
            break;
        }
        case 'a': {
            // add file
            // usage: a dest 0
            printf("%d\n", sys_create(dst, INODE_REGULAR));
            break;
        }
        case 'c': {
            // change directory
            // usage: c dest 0
            printf("%d\n", sys_chdir(dst));
            break;
        }
        case 'w': {
            // write file from existing source
            // usage: w dest src
            int fd = open(src, O_RDONLY);
            int dfd = sys_open(dst, F_RDWR | F_CREAT);
            long cnt = 0;
            ssize_t nr = read(fd, m, 512);
            while (nr > 0) {
                cnt += sys_write(dfd, m, nr);
                nr = read(fd, m, 512);
            }
            printf("%ld\n", cnt);

            sys_close(dfd);
            close(fd);
            break;
        }
        case 'r': {
            // read file or dir
            check_file_or_dir(dst);
            break;
        }
        default: {
            free(m);
            return;
        }
        }
        write(1, "< ", 2);
    }

    free(m);
}
