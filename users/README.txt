This files contains my test. These *.c files can be 
compiled and linked to static executable, which will
test the kernel's functionality from user's perspective.

To run a test, you must first create a file named 'mkfs.txt'
in the current directory(ie. ${CMAKE_SOURCE_DIR}/mkfs.txt)
which will be redirected to copyin program. 

Example `mkfs.txt` scripts:

- For checking the file system:
"""
m /bin 0
m /home 0
w /bin/sh xsh
w /bin/echo echo
w /bin/ls ls
w /bin/pwd pwd
w /bin/write write
w /bin/fstest23 fstest23
w /bin/mkdir mkdir
w /bin/link link
w /bin/unlink unlink
w /bin/stat stat
w /bin/cat cat
w /init init
q q q
"""

- For checking process management:
"""
m /bin 0
m /home 0
w /bin/sh xsh
w /bin/echo echo
w /bin/ls ls
w /bin/pwd pwd
w /bin/pipe pipe
w /bin/cat cat
w /bin/fork fork
w /bin/wait wait
w /bin/count count
w /bin/wc wc
w /init init
q q q
"""

- For checking memory management:
"""
m /bin 0
m /home 0
w /bin/sh xsh
w /bin/echo echo
w /bin/ls ls
w /bin/pwd pwd
w /bin/relf relf
w /bin/danger0 danger0
w /bin/danger1 danger1
w /bin/danger2 danger2
w /bin/crash crash
w /bin/crash1 crash1
w /bin/crash2 crash2
w /init init
q q q
"""

- For networking and normal use:
"""
m /bin 0
m /home 0
w /bin/sh xsh
w /bin/echo echo
w /bin/ls ls
w /bin/pwd pwd
w /bin/relf relf
w /bin/hear hear
w /bin/cat cat
w /bin/wc wc
w /bin/stat stat
w /bin/mkdir mkdir
w /bin/link link
w /bin/unlink unlink
w /init init
q q q
"""

Remember to run all these before submitting to Github.
