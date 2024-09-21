src = sysutil.c shell.c
sh_objs = shell.o sysutil.o
load_objs = load.o sysutil.o
CC = gcc
CFLAGS = -g -static 

# Tools, loader, shell, etc.
all: sh
sh: $(sh_objs)
	$(CC) $(sh_objs) -o sh
load: $(load_objs)
	$(CC) $(load_objs) -o load

args: args.c
	$(CC) -g args.c -o args

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# My own libc
include libc/Makefile

# "User programs"
usr_objs = argls.o echo.o
uprog = hello argls echo
uprog: $(libc_objs) $(usr_objs)

CFLAGS += -ffreestanding -fno-common -nostdlib -fno-pie -no-pie -MD -I./libc 
hello: minimal.S
	$(CC) -g -static -c minimal.S && ld minimal.o -o hello
%.o: libc/%.c
	$(CC) $(CFLAGS) -c $< -o $@
%: %.o $(libc_objs)
	ld -e _start $(libc_objs) $< -o $@

.PHONY: clean
clean:
	rm *.o *.d $(uprog)

.PHONY: format
format: fmt.sh
	chmod +x fmt.sh && ./fmt.sh
