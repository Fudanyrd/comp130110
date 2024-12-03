#include <net/net.h>
#include <net/e1000.h>
#include <kernel/mem.h>
#include <kernel/cpu.h>
#include <kernel/printk.h>

void test_init()
{
}

extern void net_tx_udp(struct mbuf *m, uint32 dip, uint16 sport, uint16 dport);
extern void e1000_intr(void);

extern struct mbuf *e1000_poll(void) ;

static void receive_and_print(void) {
    struct mbuf *m;
    while ((m = e1000_poll()) == NULL) {
        // continue;
        __sync_synchronize();
    }

    // receive a packet!
    struct eth *ethhdr;
    ethhdr = mbufpullhdr(m, *ethhdr);
    u16 type = htons(ethhdr->type);

    if (type == ETHTYPE_ARP) {
        printk("Get an arp!\n");
    } else {
        if (type == ETHTYPE_IP) {
            // an ip packet
            mbufpull(m, sizeof(struct ip));
            mbufpull(m, sizeof(struct udp));
            printk("%s\n", m->head);
        }
    }

    // allocated by kalloc_page.
    kfree_page(m);
}

void run_test()
{
    struct mbuf *m = mbufalloc(MBUF_DEFAULT_HEADROOM);
    // address: 10.0.2.2
    u32 dst = (10 << 24) | (0 << 16) | (2 << 8) | 2;

    // enable trap and device intr
    ASSERT(!_arch_disable_trap());
    extern char exception_vector[];
    arch_set_vbar(exception_vector);
    arch_reset_esr();
    bool tmp __attribute__((unused));
    tmp = _arch_enable_trap();
    __sync_synchronize();

    static const char *message = "Hello network!\n";
    char *mb = mbufput(m, 16);
    for (int i = 0; i < 16; i++) {
        mb[i] = message[i];
    }
    net_tx_udp(m, dst, 2000, 23456);

    // printk("package sent.\n");
    for (;;) {
        receive_and_print();
    }

    return;
}
