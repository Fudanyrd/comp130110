#include <fs/defines.h>
#include <fs/inode.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <fs/file1206.h>

extern int sys_create(const char *path, int flags);

// clang-format off
/**
 * Make a 16MB disk, that is 32K blocks, allows multilevel 
 * Inode tree now!
 * 
 * [sb(1) | (log:64) | (inode:1024) | (bitmap:8) | (data:rest) ]
 *        ^1         ^65            ^1089        ^1097         ^32768
 *        x200       x8200           x88200       x89200        x1000000
 */
// clang-format on
static int disk;
static SuperBlock sb;
static OpContext ctx;
static BlockCache bc;
static usize alloc_no;

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
static void add_file();

int main(int argc, char **argv)
{
    // this is an uninitialized block
    const char *dname = "sd.img";
    disk = open(dname, O_RDWR);
    assert(disk >= 0);

    // init superblock
    sb.num_blocks = 32 * 1024;
    sb.log_start = 1;
    sb.num_log_blocks = 64;
    sb.inode_start = 65;
    sb.num_inodes = 1024;
    sb.bitmap_start = 1089;
    sb.num_data_blocks = 31671;

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
    alloc_no = 1097;

    // write superblock
    Block *block;
    begin_op(&ctx);
    block = acquire(0);
    memcpy(block->data, &sb, sizeof(sb));
    release(block);

    // init initial bitmap
    block = acquire(sb.bitmap_start);
    for (usize i = 0; i < alloc_no; i++) {
        // mark non-data block as allocated.
        bitmap_set((void *)block->data, i);
    }
    release(block);

    // init inode tree
    init_inodes(&sb, &bc);

    // init root inode
    Inode *rt = inodes.get(ROOT_INODE_NO);
    // sync at this time will trigger error
    // inodes.sync(&ctx, rt, false);
    rt->valid = true;
    memset((void *)&rt->entry, 0, sizeof(rt->entry));
    rt->entry.num_links = (u16)(0xfff);
    rt->entry.type = INODE_DIRECTORY;

    // insert . and ..
    inodes.insert(&ctx, rt, ".", ROOT_INODE_NO);
    inodes.insert(&ctx, rt, "..", ROOT_INODE_NO);
    inodes.sync(&ctx, rt, true);
    inodes.put(&ctx, rt);

    // init file system
    file_init(&ctx, &bc);
    add_file();

    // done.
    end_op(&ctx);
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
    off_t offset = BLOCK_SIZE * block->block_no;
    assert(lseek(disk, offset, SEEK_SET) == offset);
    assert(write(disk, block->data, BLOCK_SIZE) >= 0);
    free(block);
}

static void begin_op(OpContext *ctx)
{
    return;
}

static void cache_sync(OpContext *ctx, Block *block)
{
    // write to file
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
    usize no = sb.bitmap_start + alloc_no / BIT_PER_BLOCK;
    Block *block = acquire(no);
    bitmap_set((uint64_t *)block->data, alloc_no % BIT_PER_BLOCK);
    release(block);
    return alloc_no++;
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

static void add_file()
{
    static char buf[16];

    char *m = malloc(BLOCK_SIZE);

    char dst[64];
    char src[64];

    while (scanf("%s %s %s", buf, dst, src) == 3) {
        printf("cmd=%s dst=%s src=%s ret=", buf, dst, src);

        switch (buf[0]) {
        case 'm': {
            // mkdir
            // usage: m dest 0
            printf("%d\n", sys_mkdir(dst));
            break;
        }
        case 'a': {
            // add file
            // usage: a dest 0
            printf("%d\n", sys_create(dst, INODE_REGULAR));
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
        case 'l': {
            // create hard links.
            printf("%d\n", sys_link(dst, src));
            break;
        }
        default: {
            printf("0(exit)\n");
            free(m);
            return;
        }
        }
    }

    free(m);
}
