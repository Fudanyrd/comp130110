# mkfs
## Usage 
```
mkfs (file 1) (file 2) ...
```

Create a 8-MB disk image, which will put file 1, 2 and so on 
in the root directory.

## Example

Command: mkfs mkfs.c readfs.c
Then we use `hexdump` to check the disk image:

Super block:
```
00000000  00 40 00 00 bb 3b 00 00  00 04 00 00 40 00 00 00  |.@...;......@...|
00000010  01 00 00 00 41 00 00 00  41 04 00 00 00 00 00 00  |....A...A.......|
00000020  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
```

Inode blocks:
```
00008240  01 00 00 00 00 00 ff ff  40 00 00 00 45 04 00 00  |........@...E...|
00008250  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00008280  02 00 00 00 00 00 01 00  f4 13 00 00 46 04 00 00  |............F...|
00008290  47 04 00 00 48 04 00 00  49 04 00 00 4a 04 00 00  |G...H...I...J...|
000082a0  4b 04 00 00 4c 04 00 00  4d 04 00 00 4e 04 00 00  |K...L...M...N...|
000082b0  4f 04 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |O...............|
000082c0  02 00 00 00 00 00 01 00  b9 0e 00 00 50 04 00 00  |............P...|
000082d0  51 04 00 00 52 04 00 00  53 04 00 00 54 04 00 00  |Q...R...S...T...|
000082e0  55 04 00 00 56 04 00 00  57 04 00 00 00 00 00 00  |U...V...W.......|
000082f0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
```

Bitmap block, the first 1089 blocks is marked as allocated:
```
00088200  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
*
00088280  ff ff ff ff ff ff ff ff  ff ff ff 00 00 00 00 00  |................|
00088290  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
```

Data blocks of root directory:
```
00088a00  01 00 2e 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00088a10  01 00 2e 2e 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00088a20  02 00 6d 6b 66 73 2e 63  00 00 00 00 00 00 00 00  |..mkfs.c........|
00088a30  03 00 72 65 61 64 66 73  2e 63 00 00 00 00 00 00  |..readfs.c......|
00088a40  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
```

Data blocks of mkfs.c:
```
00088c00  23 69 6e 63 6c 75 64 65  20 3c 66 73 2f 64 65 66  |#include <fs/def|
00088c10  69 6e 65 73 2e 68 3e 0a  23 69 6e 63 6c 75 64 65  |ines.h>.#include|
00088c20  20 3c 66 73 2f 69 6e 6f  64 65 2e 68 3e 0a 23 69  | <fs/inode.h>.#i|
00088c30  6e 63 6c 75 64 65 20 3c  66 63 6e 74 6c 2e 68 3e  |nclude <fcntl.h>|
00088c40  0a 23 69 6e 63 6c 75 64  65 20 3c 73 74 72 69 6e  |.#include <strin|
00088c50  67 2e 68 3e 0a 23 69 6e  63 6c 75 64 65 20 3c 73  |g.h>.#include <s|
00088c60  74 64 69 6e 74 2e 68 3e  0a 23 69 6e 63 6c 75 64  |tdint.h>.#includ|
00088c70  65 20 3c 73 74 64 6c 69  62 2e 68 3e 0a 23 69 6e  |e <stdlib.h>.#in|
00088c80  63 6c 75 64 65 20 3c 75  6e 69 73 74 64 2e 68 3e  |clude <unistd.h>|
00088c90  0a 23 69 6e 63 6c 75 64  65 20 3c 73 74 64 69 6f  |.#include <stdio|
00088ca0  2e 68 3e 0a 23 69 6e 63  6c 75 64 65 20 3c 61 73  |.h>.#include <as|
00088cb0  73 65 72 74 2e 68 3e 0a  0a 2f 2f 20 63 6c 61 6e  |sert.h>..// clan|
```

# readfs

## Usage
readfs (file 1) (file 2) or readfs /

can be used to examine a file or directory, currently only support these in 
the root dir.

## Example
The output of `./readfs / | hexdump -C`:

```
00000000  01 00 2e 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000010  01 00 2e 2e 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000020  02 00 6d 6b 66 73 2e 63  00 00 00 00 00 00 00 00  |..mkfs.c........|
00000030  03 00 72 65 61 64 66 73  2e 63 00 00 00 00 00 00  |..readfs.c......|
00000040
```

The fourth line demonstrates a dir entry, we can know that:
<ul>
    <li> Inode number(03 00) is 3. </li>
    <li> File name is "readfs.c". </li>
</ul>
