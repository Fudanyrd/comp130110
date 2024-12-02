#include <net/net.h>
#include <kernel/mem.h>
#include <kernel/printk.h>

void test_init()
{
}

extern void net_tx_udp(struct mbuf *m, uint32 dip, uint16 sport, uint16 dport);

void run_test()
{
    struct mbuf *m = mbufalloc(MBUF_DEFAULT_HEADROOM);
    // address: 10.0.2.2
    u32 dst = (10 << 24) | (0 << 16) | (2 << 8) | 2;

    static const char *message = "Hello network!\n";
    char *mb = mbufput(m, 16);
    for (int i = 0; i < 16; i++) {
        mb[i] = message[i];
    }
    net_tx_udp(m, dst, 2000, 23456);

    printk("package sent.\n");
    return;
}
