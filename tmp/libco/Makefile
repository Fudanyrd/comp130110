a.out: swtch.S thread-os.c
	gcc -g swtch.S thread-os.c
all: a.out

.PHONY: format
format:
	cd .. && ./fmt.sh && cd -
