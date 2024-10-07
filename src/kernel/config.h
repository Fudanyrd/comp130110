/** Kernel Configuration */
#pragma once
#ifndef __KERNEL_CONFIG_
#define __KERNEL_CONFIG_

/** Round-Robin Core Configuration 
 * This is introduced to provide  the kernel
 * with overhead balance.
 * 
 * For each proc, if it yields, then it will be put 
 * in a different scheduler queue, and hence be run 
 * on a different core later. Upon creation of a proc,
 * it is put to the scheduler queue with id 
 * (p->id % NCPU). An exception to this strategy is 
 * idle threads: they cannot be run on other cores.
 * 
 * To disable this configuration, add #undef RCC and 
 * recompile.
 */
#define RCC

#endif // __KERNEL_CONFIG_
