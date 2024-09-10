src = sysutil.c shell.c
sh_objs = shell.o sysutil.o
CC = gcc
CFLAGS = -g

all: sh
sh: $(sh_objs)
	$(CC) $(sh_objs) -o sh

args: args.c
	$(CC) -g args.c -o args

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm *.o 

.PHONY: format
format: fmt.sh
	chmod +x fmt.sh && ./fmt.sh
