
static void Puts(const char *s);
extern void sys_print(const char *s, unsigned long len);

int main(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        Puts(argv[i]);
    }
}

static void Puts(const char *s)
{
    unsigned long len = 0;
    for (len = 0; s[len] != 0; len++) {
    }

    sys_print(s, len);
    return;
}
