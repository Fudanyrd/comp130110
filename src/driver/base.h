#pragma once

#define KERNEL_BASE 0xffff000000000000
#define MMIO_BASE (KERNEL_BASE + 0x3F000000)
#define LOCAL_BASE (KERNEL_BASE + 0x40000000)

#define V2P(v) ((u64)(v)-VA_START)
#define P2V(p) ((u64)(p) + VA_START)

#define VA_START 0xffff000000000000
#define PUARTBASE 0x9000000
#define UARTBASE P2V(PUARTBASE)