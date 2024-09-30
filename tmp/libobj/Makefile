all: obj fact.so load-dl dl.so intfn.so

load-dl: dl.o load-dl.o
	gcc dl.o load-dl.o -o load-dl

OBJS = dl.o load-dl.o
%.o: %.c
	gcc -g -c -MD $< -o $@
%.so: %.c
	gcc -g -shared $< -o $@

obj: obj.c
	gcc -g obj.c -o obj

#fact.so: fact.c
#	gcc -shared -g fact.c -o fact.so

.PHONY: clean
clean:
	-rm *.o *.so obj
