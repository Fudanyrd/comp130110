cmake_minimum_required(VERSION 3.16)

project(rpi-os VERSION 0.1.0 LANGUAGES C ASM CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(aarch64_prefix "")
    set(aarch64_gdb "gdb")
else()
    set(aarch64_prefix "aarch64-linux-gnu-")
    set(aarch64_gdb "gdb-multiarch")
endif()

set(aarch64_gcc "${aarch64_prefix}gcc")
set(aarch64_ld "${aarch64_prefix}ld")
set(aarch64_objdump "${aarch64_prefix}objdump")
set(aarch64_objcopy "${aarch64_prefix}objcopy")

set(aarch64_qemu "qemu-system-aarch64")

add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(users)

get_property(kernel_elf GLOBAL PROPERTY kernel_elf_path)
get_property(kernel_image GLOBAL PROPERTY kernel_image_path)
get_property(sd_image GLOBAL PROPERTY sd_image_path)

set(qemu_flags
    -machine virt,gic-version=3,highmem-ecam=off
    -cpu cortex-a72
    -smp 4
    -m 4096
    -nographic
    -monitor none
    -serial "mon:stdio"
    -global virtio-mmio.force-legacy=false
    -drive file=${sd_image},if=none,format=raw,id=d0
    -device virtio-blk-device,drive=d0,bus=virtio-mmio-bus.0
    -netdev user,id=net0,hostfwd=udp::25999-:2000 -object filter-dump,id=net0,netdev=net0,file=packets.pcap
    -device e1000,netdev=net0,bus=pcie.0
    -kernel "${kernel_elf}")


message(STATUS "Path to Disk Image: ${sd_image}")

# build qemu with 1(unique) CPU for debugging
# race conditions (partially) avoided
set(qemu_unic_flags
    -machine virt,gic-version=3
    -cpu cortex-a72
    -smp 1
    -m 4096
    -nographic
    -monitor none
    -serial "mon:stdio"
    -global virtio-mmio.force-legacy=false
    -kernel "${kernel_elf}")

add_custom_target(qemu
    COMMAND ${aarch64_qemu} ${qemu_flags} -gdb tcp::1234
    DEPENDS image)
add_custom_target(qemu-unic 
    COMMAND ${aarch64_qemu} ${qemu_unic_flags}
    DEPENDS image)
add_custom_target(qemu-debug
    COMMAND ${aarch64_qemu} ${qemu_flags} -gdb tcp::1234 -S
    DEPENDS image)
add_custom_target(qemu-unic-debug
    COMMAND ${aarch64_qemu} ${qemu_unic_flags} -gdb tcp::1234 -S
    DEPENDS image)

add_custom_target(qemu-gdb
    COMMAND ${arrch64_qemu} ${qemu_flags} -gdb tcp::1234 -S
    DEPENDS image)
add_custom_target(debug
    COMMAND ${aarch64_gdb} --nx --quiet
            -ex "set architecture aarch64"
            -ex "file ${kernel_elf}"
            -ex "target remote localhost:1234"
    DEPENDS kernel)

# script for running qemu.
add_custom_target(script
    COMMAND /usr/bin/echo \"\#!/usr/bin/bash\" > debug.sh &&
            /usr/bin/echo \"\#Auto Generated. Don\'t Modify.\" >> debug.sh &&
            make -n qemu | grep \"qemu-system-aarch64\" | sed -r \"s/ -/ \\\\\\\\\\n\\t-/g\" >> debug.sh &&
            chmod +x debug.sh
)

foreach(tcase IN LISTS tcases)
    get_property(elf GLOBAL PROPERTY ${tcase}_elf_path)
    message(STATUS "Adding custom target for ${tcase}")
    add_custom_target("${tcase}-test"
    COMMAND ${aarch64_qemu}
        -machine virt,gic-version=3
        -cpu cortex-a72
        -smp 4
        -m 4096
        -nographic
        -monitor none
        -serial "mon:stdio"
        -global virtio-mmio.force-legacy=false
        -kernel ${elf}
    DEPENDS "${elf}")
    add_custom_target("${tcase}-test-gdb"
    COMMAND ${aarch64_qemu}
        -machine virt,gic-version=3
        -cpu cortex-a72
        -smp 1
        -m 4096
        -nographic
        -monitor none
        -serial "mon:stdio"
        -global virtio-mmio.force-legacy=false
        -kernel ${elf}
        -gdb tcp::1234 -S
   DEPENDS "${elf}")
endforeach()
