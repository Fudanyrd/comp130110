#!/usr/bin/bash
# A simple runner that automatically build disk image
# and run qemu.

tests=$( ls *.c )
PWD=$( pwd )
BUILD_DIR=../../../build

echo "Collecting test cases..." $tests
echo "Getting current working directory..." $PWD
echo "Getting build directory..." $BUILD_DIR

for test in $tests; do 
./install.sh $test > /dev/null
qemu-system-aarch64 \
    -machine virt,gic-version=3,highmem-ecam=off -cpu cortex-a72 \
    -smp 1 -m 4096 -nographic -monitor none -serial mon:stdio -global virtio-mmio.force-legacy=false \
    -drive file=/root/OS-24Fall-FDU/build/sd.img,if=none,format=raw,id=d0 \
    -device virtio-blk-device,drive=d0,bus=virtio-mmio-bus.0 \
    -kernel /root/OS-24Fall-FDU/build/src/kernel8.elf \
    -netdev user,id=net0,hostfwd=udp::26999-:2000 \
    -device e1000,netdev=net0,bus=pcie.0,id=e0,id=e1 \
    -gdb tcp::1234
done

    # -object filter-dump,id=net0,netdev=net0,file=packets.pcap
