set(CMAKE_EXPORT_COMPILE_COMMANDS True)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_COMPILER ${aarch64_gcc})
set(CMAKE_ASM_COMPILER ${aarch64_gcc})

set(compiler_warnings "-Wall -Werror")
set(compiler_flags "${compiler_warnings} \
    -static -ffreestanding -nostdlib -O3 -fno-plt -fno-pic -mgeneral-regs-only -Wall -Werror")

set(CMAKE_EXE_LINKER_FLAGS "-nostdlib")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${compiler_flags}")

add_custom_target(ping
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/ping.py 22307
)
add_custom_target(server
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/server.py 22307
)

add_library(start STATIC start.S)

add_custom_target(mkfs-script
    COMMAND /usr/bin/ls ${CMAKE_CURRENT_SOURCE_DIR}/mkfs.txt
    DEPENDS  cat chdir count crash 
            crash1 crash2 danger0 danger1 danger2 echo exec fork fstest23 
            head hear init link ls main mkdir mmaptest pipe pwd relf sh stat 
            unlink wait wc write xsh
)

add_custom_command(
    OUTPUT sd.img
    COMMAND /usr/bin/cat /dev/zero | /usr/bin/head -c 16777216 > sd.img && ../tools/copyin < ${CMAKE_CURRENT_SOURCE_DIR}/mkfs.txt
    DEPENDS mkfs-script copyin ${CMAKE_CURRENT_SOURCE_DIR}/mkfs.txt
)

add_custom_target(image ALL DEPENDS sd.img kernel)
set_property(GLOBAL PROPERTY sd_image_path ${CMAKE_CURRENT_BINARY_DIR}/sd.img)

# Hint: auto-generated CMakeLists.txt!
#for file in $( ls *.c | sed "s/\.c//" ); do
#    echo "# user program $file"
#    echo "add_executable($file $file.c)"
#    echo "target_link_libraries($file start)"
#    echo ""
#done
# user program cat
add_executable(cat cat.c)
target_link_libraries(cat start)

# user program chdir
add_executable(chdir chdir.c)
target_link_libraries(chdir start)

# user program count
add_executable(count count.c)
target_link_libraries(count start)

# user program crash
add_executable(crash crash.c)
target_link_libraries(crash start)

# user program crash1
add_executable(crash1 crash1.c)
target_link_libraries(crash1 start)

# user program crash2
add_executable(crash2 crash2.c)
target_link_libraries(crash2 start)

# user program danger0
add_executable(danger0 danger0.c)
target_link_libraries(danger0 start)

# user program danger1
add_executable(danger1 danger1.c)
target_link_libraries(danger1 start)

# user program danger2
add_executable(danger2 danger2.c)
target_link_libraries(danger2 start)

# user program echo
add_executable(echo echo.c)
target_link_libraries(echo start)

# user program exec
add_executable(exec exec.c)
target_link_libraries(exec start)

# user program fork
add_executable(fork fork.c)
target_link_libraries(fork start)

# user program fstest23
add_executable(fstest23 fstest23.c)
target_link_libraries(fstest23 start)

# user program head
add_executable(head head.c)
target_link_libraries(head start)

# user program hear
add_executable(hear hear.c)
target_link_libraries(hear start)

# user program init
add_executable(init init.c)
target_link_libraries(init start)

# user program link
add_executable(link link.c)
target_link_libraries(link start)

# user program ls
add_executable(ls ls.c)
target_link_libraries(ls start)

# user program main
add_executable(main main.c)
target_link_libraries(main start)

# user program mkdir
add_executable(mkdir mkdir.c)
target_link_libraries(mkdir start)

# user program mmaptest
add_executable(mmaptest mmaptest.c)
target_link_libraries(mmaptest start)

# user program necho
add_executable(necho necho.c)
target_link_libraries(necho start)

# user program pipe
add_executable(pipe pipe.c)
target_link_libraries(pipe start)

# user program pwd
add_executable(pwd pwd.c)
target_link_libraries(pwd start)

# user program relf
add_executable(relf relf.c)
target_link_libraries(relf start)

# user program sh
add_executable(sh sh.c)
target_link_libraries(sh start)

# user program stat
add_executable(stat stat.c)
target_link_libraries(stat start)

# user program unlink
add_executable(unlink unlink.c)
target_link_libraries(unlink start)

# user program wait
add_executable(wait wait.c)
target_link_libraries(wait start)

# user program wc
add_executable(wc wc.c)
target_link_libraries(wc start)

# user program write
add_executable(write write.c)
target_link_libraries(write start)

# user program xsh
add_executable(xsh xsh.c)
target_link_libraries(xsh start)

