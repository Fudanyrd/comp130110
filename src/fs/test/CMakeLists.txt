cmake_minimum_required(VERSION 3.16)

project(fs-test VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include_directories(../..)

set(compiler_warnings "-Wall -Wextra")
set(compiler_flags "${compiler_warnings} \
    -O0 -ftree-pre -g -DSTAND_ALONE \
    -fno-omit-frame-pointer \
    -fsanitize=undefined \
    -fno-sanitize=alignment")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter ${compiler_flags}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${compiler_flags}")

file(GLOB mock_sources CONFIGURE_DEPENDS "mock/*.cpp")
add_library(mock STATIC ${mock_sources})

file(GLOB fs_sources CONFIGURE_DEPENDS "../*.c")
add_library(fs STATIC ${fs_sources} "instrument.c")
target_compile_options(fs PUBLIC "-fno-builtin")

add_executable(inode_test inode_test.cpp)
target_link_libraries(inode_test fs mock pthread)

add_executable(cache_test cache_test.cpp)
target_link_libraries(cache_test fs mock pthread)

# mkfs is a tool to build file system.
add_executable(mkfs mkfs.c)
target_link_libraries(mkfs fs mock)

# readfs: read a file in the root dir.
# It can also read the root dir itself.
# usage: readfs "/" or readfs <fname>
add_executable(readfs readfs.c)
target_link_libraries(readfs fs mock)

add_executable(copyin copyin.c)
target_link_libraries(copyin fs mock )

add_executable(copyout copyout.c)
target_link_libraries(copyout fs mock )
