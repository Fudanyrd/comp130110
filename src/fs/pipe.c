#include "pipe.h"

/** Guards the global pipe list. */
static SpinLock plock;

static ListNode head;
static ListNode tail;

// push a pipe into list. Must hold lock.
static INLINE void pipe_push_lst(Pipe *ino)
{
    ListNode *prev = tail.prev;
    ListNode *nxt = &tail;
    ino->node.prev = prev;
    ino->node.next = nxt;
    prev->next = &ino->node;
    nxt->prev = &ino->node;
}

// remove an inode from list. Must hold lock.
// NOTE: will not free ino.
static INLINE void pipe_rm_lst(Pipe *ino)
{
    ListNode *prev = ino->node.prev;
    ListNode *nxt = ino->node.next;

    // validate the list.
    ASSERT(prev->next = &ino->node);
    ASSERT(nxt->prev = &ino->node);

    // do removal
    prev->next = nxt;
    nxt->prev = prev;
}

void pipe_init(void)
{
    head.prev = NULL;
    head.next = &tail;
    tail.prev = &head;
    tail.next = NULL;
    init_spinlock(&plock);
}

Pipe *pipe_open(void)
{
    Pipe *pip = kalloc(sizeof(Pipe));
    if (pip == NULL) {
        return NULL;
    }

    // register in the list.
    acquire_spinlock(&plock);
    pipe_push_lst(pip);
    release_spinlock(&plock);

    /// init lock and condvar
    cond_init(&pip->cv);
    init_spinlock(&pip->lock);

    // mark nreader and nwriter
    pip->nreader = pip->nwriter = 1;

    // mark nread and bwrite
    pip->bread = pip->bwrite = 0;
    return pip;
}

void pipe_close_read(Pipe *p)
{
    acquire_spinlock(&p->lock);
    ASSERT(p->nreader != 0);
    p->nreader--;
    if (p->nreader == 0) {
        if (p->nwriter == 0) {
            // safe to free the pipe.
            release_spinlock(&p->lock);
            acquire_spinlock(&plock);
            pipe_rm_lst(p);
            release_spinlock(&plock);
            kfree(p);
        } else {
            // wakeup the writer so that it can close it.
            cond_signal(&p->cv);
            release_spinlock(&p->lock);
        }
    }
}

void pipe_close_write(Pipe *p)
{
    acquire_spinlock(&p->lock);
    ASSERT(p->nwriter != 0);
    p->nwriter--;
    if (p->nwriter == 0) {
        if (p->nreader == 0) {
            // safe to free the pipe.
            release_spinlock(&p->lock);
            acquire_spinlock(&plock);
            pipe_rm_lst(p);
            release_spinlock(&plock);
            kfree(p);
        } else {
            // should wakeup reader
            cond_signal(&p->cv);
            release_spinlock(&p->lock);
        }
    }
}

isize pipe_read(Pipe *p, char *buf, usize count)
{
    // if buffer is not empty, copyout.
    acquire_spinlock(&p->lock);
    ASSERT(p->nreader > 0);

    while (p->bwrite == p->bread && p->nwriter > 0) {
        // goto sleep.
        cond_wait(&p->cv, &p->lock);
    }

    // check for broken pipe first.
    if (p->bwrite == p->bread && p->nwriter == 0) {
        // the reader should now close the pipe!
        release_spinlock(&p->lock);
        return -1;
    }

    // copy this number of bytes out.
    // nb = min(count, (p->bwrt - p->brd));
    usize nb = count > (p->bwrite - p->bread) ? (p->bwrite - p->bread) : count;
    for (usize i = 0; i < nb; i++) {
        *buf = p->buf[p->bread % PIPE_SIZE];
        // advance
        p->bread++;
        buf++;
    }

    // wakeup blocked writer.
    cond_signal(&p->cv);
    release_spinlock(&p->lock);
    return (isize)nb;
}

isize pipe_write(Pipe *p, char *buf, usize count)
{
    acquire_spinlock(&p->lock);
    ASSERT(p->nwriter > 0);

    while (p->bwrite == (p->bread + PIPE_SIZE) && p->nreader > 0) {
        // goto sleep
        cond_wait(&p->cv, &p->lock);
    }

    // no one will read the data from the pipe!
    // do not try to write to the pipe.
    if (p->nreader == 0) {
        release_spinlock(&p->lock);
        return -1;
    }

    // copy this number of bytes in.
    // nb = min(count, (p->bwrt - p->brd));
    ASSERT(p->bwrite <= p->bread + PIPE_SIZE);
    usize nb = count > (PIPE_SIZE + p->bread - p->bwrite) ?
                       (PIPE_SIZE + p->bread - p->bwrite) :
                       count;
    for (usize i = 0; i < nb; i++) {
        p->buf[p->bwrite % PIPE_SIZE] = *buf;
        buf++;
        p->bwrite++;
    }

    // signal the reader
    cond_signal(&p->cv);
    release_spinlock(&p->lock);
    return (isize)nb;
}
