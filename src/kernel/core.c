#include <aarch64/intrinsic.h>
#include <common/buf.h>
#include <driver/virtio.h>
#include <kernel/core.h>
#include <kernel/cpu.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <test/test.h>
#include <fdutil/stddef.h>
#include <fs/file1206.h>
#include <kernel/exec1207.h>

volatile bool panic_flag;

NO_RETURN void idle_entry()
{
    set_cpu_on();
    while (1) {
        yield();
        if (panic_flag)
            break;
        arch_with_trap
        {
            arch_wfi();
        }
    }
    set_cpu_off();
    arch_stop_cpu();
}

NO_RETURN void kernel_entry()
{
    printk("Hello world! (Core %lld)\n", cpuid());
    // user_proc_test();

    // before doing anything else, 
    // initialize the file system first.
    // the fs is initialized here because it need 
    // some other proc to take over.
    fs_init();

    ASSERT(inodes.root->valid);

    // init proc runs at root dir.
    thisproc()->cwd = inodes.share(inodes.root);
    // setup a usercontext on stack.
    UserContext ctx;
    thisproc()->ucontext = &ctx;

    char *argv[] = {
        "/init", ".", "/", "/init", NULL
    };
    exec(argv[0], argv);
    
    // the user space is ready, now jump to the 
    // user space via trap_ret. This should be 
    // the only case where trap_ret happens without a 
    // trap_entry.
    asm volatile(
        "mov sp, %0\n\t"
        "bl trap_return"
      :
      : "r"(&ctx)
    );

    while (1)
        yield();
}

NO_INLINE NO_RETURN void _panic(const char *file, int line)
{
    printk("=====%s:%d PANIC%lld!=====\n", file, line, cpuid());
    panic_flag = true;
    // set_cpu_off();
    for (int i = 0; i < NCPU; i++) {
        if (cpus[i].online)
            i--;
    }
    printk("Kernel PANIC invoked at %s:%d. Stopped.\n", file, line);
    arch_stop_cpu();
}

static Buf mbr_buf;

void init_mbr()
{
    /* MBR is at the first 512 Bytes. */
    STATIC_ASSERT(sizeof(struct mbr) == 512);
    mbr_buf.flags = 0;
    mbr_buf.block_no = 0;

    virtio_blk_rw(&mbr_buf);

    /* Verify the content of mbr: check magic */
    struct mbr *mbr = (struct mbr *)(mbr_buf.data);
    ASSERT(mbr->magic_1 == 0x55);
    ASSERT(mbr->magic_2 == 0xaa);

    /* Log the LBA of first absolute sector */
    uint32_t *pt = (uint32_t *)((char *)mbr->partition_entr_2 + 0x8);
    printk("LBA of partition 2: %u\n", *pt);
    /* Log the number of sectors in the partition. */
    pt++;
    printk("# sectors of partition 2: %u\n", *pt);
}

struct mbr *this_mbr()
{
    struct mbr *mbr = (struct mbr *)(mbr_buf.data);
    return mbr->magic_1 == 0x55 ? mbr : NULL;
}
