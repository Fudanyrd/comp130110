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
    -O0 -ftree-pre -g -DSTAND_ALONE \
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
