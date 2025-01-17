set(CMAKE_EXPORT_COMPILE_COMMANDS True)

# Use local toolchain
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_ASM_COMPILER gcc)

include_directories(../src)

set(compiler_warnings "-Wall -Wextra")
set(compiler_flags "${compiler_warnings} \
    -O2 -ftree-pre -g -DSTAND_ALONE \
    -fno-omit-frame-pointer \
    -fsanitize=undefined \
    -fno-sanitize=alignment")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter ${compiler_flags}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${compiler_flags}")

file(GLOB mock_sources CONFIGURE_DEPENDS "mock/*.cpp")
add_library(mock STATIC ${mock_sources})

# Use local compiler to compile fs lib again.
file (GLOB fs_sources CONFIGURE_DEPENDS "../src/fs/*.c")
add_library(fs-local STATIC ${fs_sources})

add_executable(copyin copyin.c)
target_link_libraries(copyin fs-local mock )

add_executable(copyout copyout.c)
target_link_libraries(copyout fs-local mock )

add_executable(pipe_test pipe_test.cpp)
target_link_libraries(pipe_test fs-local mock)

add_executable(inode_test inode_test.cpp)
target_link_libraries(inode_test fs-local mock)

add_executable(cache_test cache_test.cpp)
target_link_libraries(cache_test fs-local mock)

# run tests.
add_custom_target(fstest
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/cache_test && 
            ${CMAKE_CURRENT_BINARY_DIR}/inode_test &&
            ${CMAKE_CURRENT_BINARY_DIR}/pipe_test
    DEPENDS cache_test inode_test pipe_test
)
