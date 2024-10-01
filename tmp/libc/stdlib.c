void Exit(int code)
{
    asm volatile("mov w8, #93");
    asm volatile("mov x0, %0" : : "r"(code));
    asm volatile("svc #0");
}
