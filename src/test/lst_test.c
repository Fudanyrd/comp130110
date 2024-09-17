#include "test.h"
#include "test_util.h"
#include <fdutil/lst.h>
#include <fdutil/stdint.h>

// list elem plus an integer.
struct int_elem {
    int val;
    struct list_elem elem;
};

#define NELEM 32

static struct int_elem elems[NELEM];

// simply test lst.c
static void lst_test(void)
{
    // since lst implementation is borrowed from
    // a famous project and I have played with it
    // a lot, just need to make sure it passes
    // compilation.

    TEST_START;
    // check list_entry.
    ASSERT((struct int_elem *)elems ==
           list_entry(&(elems[0].elem), struct int_elem, elem));

    // make a list
    struct list lst;
    list_init(&lst);
    struct list_elem *e;
    struct int_elem *ie;
    int cnt;

    // put every thing onto list.
    for (int i = 0; i < NELEM; i++) {
        elems[i].val = i;
        list_push_back(&lst, &(elems[i].elem));
    }

    // check list status
    // review: how to traverse in the list?
    // hint: read the comments in fdutil/lst.h
    cnt = 0;
    e = list_begin(&lst);
    for (; e != list_end(&lst); e = list_next(e)) {
        ie = list_entry(e, struct int_elem, elem);
        ASSERT(ie != NULL && ie->val == cnt++);
    }

    // check list size
    ASSERT(list_size(&lst) == NELEM);

    // traverse in the reverse order
    e = list_rbegin(&lst);
    for (; e != list_rend(&lst); e = list_prev(e)) {
        ie = list_entry(e, struct int_elem, elem);
        --cnt;
        ASSERT(ie != NULL && ie->val == cnt);
    }

    // pop everything out of list
    cnt = 0;
    while (!list_empty(&lst)) {
        e = list_pop_front(&lst);
        ie = list_entry(e, struct int_elem, elem);
        ASSERT(ie != NULL && ie->val == cnt++);
    }
    ASSERT(list_empty(&lst));

    // OK, congrats!
    TEST_END;
}

void run_test(void)
{
    // this is a single core test,
    // i.e. not concurrent
    if (cpuid() == 0)
        lst_test();
}

void test_init(void)
{
}