#pragma once

/** 
 * WARNING:
 * NAME OF THIS FILE IS RANDOMIZED TO AVOID FUTURE GIT MERGE COLLISION! 
 */

#include "pipe.h"
#include "defines.h"
#include "block_device.h"
#include "cache.h"
#include "inode.h"

#include <common/defines.h>

#define F_READ 0x1
#define F_WRITE 0x2
#define F_CREATE 0x4

extern BlockDevice block_device;
extern BlockCache bcache;
extern InodeTree inodes;

extern void fs_init();

// maximum number of open files in the whole system.
#define NFILE 65536  

typedef struct file {
    // type of the file.
    // Note that a device file will be FD_INODE too.
    enum { 
        FD_NONE = 0, 
        FD_PIPE, 
        FD_INODE 
    } type;

    // reference count.
    int ref;
    // whether the file is readable or writable.
    bool readable, writable;

    // corresponding underlying object for the file.
    union {
        Inode* ino;
        struct pipe* pipe;
    };

    // offset of the file in bytes.
    // For a pipe, it is the number of bytes that have been written/read.
    usize off;
} File;

// Opended file by a process
struct oftable {
    struct file *ofile[16];
};

/** Change directory. */
int sys_chdir(const char *path);

/** Returns file struct. Should use fclose to free it. */
File *fopen(const char *path, int flags);
void fclose(File *fobj);

/** Returns the position after the seek. -1 if error */
#define S_SET 0
#define S_CUR 1
#define S_END 2

/** Seek a file.
 * @return the offset after the seek. -1 if failure. 
 */
isize fseek(File *fobj, isize bias, int flag);

/** Returns num bytes read from file */
isize fread(File *fobj, char *buf, u64 count);
