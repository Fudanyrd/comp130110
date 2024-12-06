/**
 * This is a demo of how to use net module to do networking,
 * but also a test of its correctness.
 * 
 * To run it, first run `make server` on one side, and then 
 * run `make qemu`. Currently the network module is NOT locked,
 * so be careful of concurrency issues!
 */

#include <net/net.h>
#include <net/e1000.h>
#include <net/socket.h>
#include <kernel/mem.h>
#include <kernel/cpu.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <driver/interrupt.h>

extern void net_tx_udp(struct mbuf *m, uint32 dip, uint16 sport, uint16 dport);
extern void e1000_intr(void);

static void rt_entry()
{
    // address: 10.0.2.2
    u32 dst = (10 << 24) | (0 << 16) | (2 << 8) | 2;

    // initialize semaphore, lock, etc.
    sock_init();
    Socket *sock = sock_open(dst, 2000, 23456);

    char *message = "Hello network!\n";
    static char buf[256];
    sock_write(sock, message, 16);

    for (;;) {
        sock_read(sock, buf, sizeof(buf));
        printk("%s\n", (char *)buf);
    }
}

static void chd_entry(u64 arg __attribute__((unused)))
{
    for (;;) {
    }
}

static void timer_irq_handler()
{
    yield();
}

void test_init()
{
    extern Proc root_proc;
    root_proc.kcontext.x0 = (uint64_t)rt_entry;
    Proc *chd = create_proc();
    start_proc(chd, chd_entry, 1234);
    set_interrupt_handler(TIMER_IRQ, timer_irq_handler);

    // enable trap
    set_cpu_on();
    bool ret __attribute__((unused));
    ret = _arch_enable_trap();
}

void run_test()
{
    yield();
    yield();
    yield();
    for (;;) {
    }
}
