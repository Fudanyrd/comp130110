This files contains my test. These *.c files can be 
compiled and linked to static executable, which will
test the kernel's functionality from user's perspective.

To run a test, you must first create a file named 'mkfs.txt'
in the current directory(ie. ${CMAKE_SOURCE_DIR}/mkfs.txt)
which will be redirected to copyin program. 

Example `mkfs.txt` scripts:

- For checking the file system: fs.txt
- For checking memory management: mem.txt
- For networking and normal use: normal.txt
- For checking process management: proc.txt

Remember to run all these before submitting to Github.
