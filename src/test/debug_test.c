#include "test.h"
#include <common/debug.h>

static void foo(void);
static void bar(void);
static void baz(void);

void debug_test(void)
{
    // this is a single-core test.
    foo();
}

static void foo(void)
{
    bar();
}

static void bar(void)
{
    // (suppose) a lot of code here...
    baz();
}

static void baz(void)
{
    //
    // insert a lot of empty lines...
    // output should include debug_test, foo, bar, baz.
    //
    backtrace();
}

// now run debug test.
void run_test(void)
{
    debug_test();
}
