#pragma once
#ifndef _FS_PIPE_
#define _FS_PIPE_

#include <fs/condvar.h>
#include <common/spinlock.h>
#include <common/defines.h>
#include <common/list.h>

#define PIPE_SIZE 512

#include <kernel/mem.h>

typedef struct pipe {
    ListNode node; // for management
    SpinLock lock; // protects nreader, nwriter
    struct condvar cv; // sleep/wakeup
    u32 nreader; // number of reader
    u32 nwriter; // number of writer
    u32 bread; // number of bytes read
    u32 bwrite; // number of bytes write
    u8 buf[PIPE_SIZE]; // buffer
} Pipe;

/** Initialization */
extern void pipe_init(void);

/** Create and initialize a pipe,
 * Set number of reader/writer to 1.
 */
extern Pipe *pipe_open(void);

/** Reopen a read port. */
static inline void pipe_reopen_read(Pipe *p)
{
    acquire_spinlock(&p->lock);
    p->nreader++;
    release_spinlock(&p->lock);
}

/** Reopen a write port */
static inline void pipe_reopen_write(Pipe *p)
{
    acquire_spinlock(&p->lock);
    p->nwriter++;
    release_spinlock(&p->lock);
}

/** Decrement reader count */
extern void pipe_close_read(Pipe *p);

/** Decrement writer count */
extern void pipe_close_write(Pipe *p);

/** Read from pipe, suspend if pipe is empty.
 * @return -1 if pipe is broken.
 */
extern isize pipe_read(Pipe *p, char *buf, usize count);

/** Write to pipe, suspend if pipe is full.
 * @return -1 if pipe is broken.
 */
extern isize pipe_write(Pipe *p, char *buf, usize count);

#endif // _FS_PIPE_
