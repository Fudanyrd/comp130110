all: obj fact.so load-dl dl.so intfn.so fact-recur.so

load-dl: dl.o load-dl.o
	gcc dl.o load-dl.o -o load-dl

OBJS = dl.o load-dl.o
%.o: %.c
	gcc -g -c -MD $< -o $@

# the sign -fPIC allows for generation of position independent code,
# i.e. the text can be loaded to any position.
%.so: %.c
	gcc -g -shared -fPIC $< -o $@

obj: obj.c
	gcc -g obj.c -o obj

#fact.so: fact.c
#	gcc -shared -g fact.c -o fact.so

.PHONY: clean
clean:
	-rm *.o *.so obj *.d
