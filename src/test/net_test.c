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
#include <kernel/mem.h>
#include <kernel/cpu.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <driver/interrupt.h>

extern void net_tx_udp(struct mbuf *m, uint32 dip, uint16 sport, uint16 dport);
extern void e1000_intr(void);
extern struct mbuf *e1000_poll(void) ;
static void receive_and_print(void);

static void rt_entry() {
    struct mbuf *m = mbufalloc(MBUF_DEFAULT_HEADROOM);
    // address: 10.0.2.2
    u32 dst = (10 << 24) | (0 << 16) | (2 << 8) | 2;

    // initialize semaphore, lock, etc.
    net_txn_init();

    static const char *message = "Hello network!\n";
    char *mb = mbufput(m, 16);
    for (int i = 0; i < 16; i++) {
        mb[i] = message[i];
    }
    net_tx_udp(m, dst, 2000, 23456);

    for (;;) {
        receive_and_print();
    }
}

static void chd_entry(u64 arg __attribute__((unused))) {
    for (;;) {
    }
}

static void timer_irq_handler() {
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

static void receive_and_print(void) {
    struct mbuf *m;
    /*
    while ((m = e1000_poll()) == NULL) {
        // continue;
        __sync_synchronize();
    }

    // receive a packet!
    struct eth *ethhdr;
    ethhdr = mbufpullhdr(m, *ethhdr);
    u16 type = htons(ethhdr->type);

    if (type == ETHTYPE_ARP) {
        printk("A\n");
    } else {
        if (type == ETHTYPE_IP) {
            // an ip packet
            mbufpull(m, sizeof(struct ip));
            mbufpull(m, sizeof(struct udp));
            printk("D\n");
        }
    }*/

    net_txn_begin();
    // wakeup by irq handler, 
    // the data should be ready!
    m = net_txn_end();
    if (m != NULL) {
        printk("%s\n", m->head);
    }

    // allocated by kalloc_page.
    kfree_page(m);
}

void run_test()
{
    yield();
    yield();
    yield();
    for (;;) {}
}
