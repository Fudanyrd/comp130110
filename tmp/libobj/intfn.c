/**
 * Collection of integer functions
 */

__attribute__((noinline)) int foo(int v)
{
    return v & (~4095);
}

__attribute__((noinline)) int bar(int n)
{
    return n & 4095;
}

int baz(int n)
{
    return foo(n) + bar(n);
}
