src = sysutil.c shell.c
sh_objs = shell.o sysutil.o
load_objs = load.o sysutil.o
CC = gcc
CFLAGS = -g -static 

all: sh
sh: $(sh_objs)
	$(CC) $(sh_objs) -o sh
load: $(load_objs)
	$(CC) $(load_objs) -o load

args: args.c
	$(CC) -g args.c -o args
hello: minimal.S
	$(CC) -g -static -c minimal.S && ld minimal.o -o hello

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm *.o 

.PHONY: format
format: fmt.sh
	chmod +x fmt.sh && ./fmt.sh
