#include <kernel/cpu.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/proc.h>
#include <aarch64/mmu.h>

struct cpu cpus[NCPU];

/** Idle process on each cpu */
struct Proc idleproc[NCPU];

void set_cpu_on()
{
    ASSERT(!_arch_disable_trap());
    // disable the lower-half address to prevent stupid errors
    extern PTEntries invalid_pt;
    arch_set_ttbr0(K2P(&invalid_pt));
    extern char exception_vector[];
    arch_set_vbar(exception_vector);
    arch_reset_esr();
    cpus[cpuid()].online = true;
    printk("CPU %lld: hello\n", cpuid());
}

void set_cpu_off()
{
    _arch_disable_trap();
    cpus[cpuid()].online = false;
    printk("CPU %lld: stopped\n", cpuid());
}

void push_off()
{
    struct cpu *cpu = &cpus[cpuid()];
    int old = (int)cpu->online;
    _arch_disable_trap();
    ASSERT(cpu->noff >= 0);
    if (cpu->noff == 0) {
        cpu->intena = old;
    }
    ++cpu->noff;
}

void pop_off()
{
    struct cpu *cpu = &cpus[cpuid()];
    ASSERT(cpu->noff > 0);
    cpu->noff -= 1;
    if (cpu->noff == 0 && cpu->intena) {
        _arch_enable_trap();
    }
}
