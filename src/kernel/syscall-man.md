# print

## NAME
print - print a short message to console. Will not be affected by redirection or pipe.

## SYNOPSIS
```c
void print(const char *fmt, uint64_t len);
```

## Description

This syscall sends a message to kernel, the kernel format and then prints
it to the console. This is helpful for debugging.

## Return Value 
None.

# open

## NAME
open - open a regular file or directory or device. 

## SYNOPSIS
```c
int open(const char *path, int flags);
```

## Description

This syscall opens the inode at `path` and record in the program's open file table.
This support both absolute(eg. "/home/foo/bar", starting from root) and 
relative path name(eg. "./foo/bar"). 

The `flags` can be the combination of `O_READ`, `O_WRITE`, `O_CREATE`, and `O_TRUNC`.

**O_READ**: the returned file descriptor is readable.
**O_WRITE**: the returned file descriptor is writable.
**O_CREATE**: if the regular file does not exist, create one.
**O_TRUNC**: if the file already exists, clear its content.

For each initialized program, the first 3 file descriptors(0,1,2) are used for `stdin`,
`stdout` and `stderr`, respectively.

## Return Value
The file descriptor, -1 on error.

## See Also
<a href="#close">close</a>

# close

## NAME
close - close a file descriptor opened by syscall <a href="#open">open</a>.

## SYNOPSIS
```c
int close(int fd);
```

## Description

The syscall closes the file descriptor and frees the underlying in-memory file
from the process's open file table.

## Return Value
0 on success. -1 on error.

## See Also
<a href="#open"> open </a>

# readdir

## NAME
readdir - read a directory entry and copy it to user space, 
and increment the file's offset.

## SYNOPSIS

```c
typedef struct dirent {
    uint16_t inode_no;
    char name[14];
} DirEntry;


int readdir(int fd, DirEntry *buf);
```

## Return Value
0 on success. -1 if `fd` reaches its EOF, or is not a directory, 
or `fd` is not valid.

## See Also
<a href="#close"> close </a>
<a href="#open"> open </a>

# read

## NAME
read - read bytes from a file or device or pipe or web socket. 

## SYNOPSIS

```c
long read(int fd, char *buf, long len);
```

## Return Value
The actual number of bytes read to buffer. -1 on error.

## See Also
<a href="#close"> close </a>
<a href="#open"> open </a>
<a href="#pipe"> pipe </a>

# chdir

## NAME
chdir - Change the program's current working directory. 

## SYNOPSIS

```c
int chdir(const char *path);
```
