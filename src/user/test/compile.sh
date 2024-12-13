#!/usr/bin/bash
# compile all user programs and tests, then put in
# the build directory and disk image.

if [[ $# -ne 1 ]]; then 
    echo "What test program you want to run?" 1>&2
    exit 1
fi

build=../../../build
uprog=echo
tests="cat chdir exec fork fstest23 ls mkdir wait write"
cflags="-static -ffreestanding -O3 -fno-plt -fno-pic -mgeneral-regs-only"

echo "Building start.o..."
gcc $cflags -c start.S -o start.o

for up in $uprog; do
    echo "Buiding user program $up..."
    gcc $cflags -c $up.c -o $up.o && ld start.o $up.o -o $up
    if [[ $? -ne 0 ]]; then
        echo "Abort" 1>&2
    fi
    mv $up $build/$up
done

for up in $tests; do
    echo "Buiding test program $up..."
    gcc $cflags -c $up.c -o $up.o && ld start.o $up.o -o $up
    if [[ $? -ne 0 ]]; then
        echo "Abort" 1>&2
    fi
    mv $up $build/$up
done

# clean
rm *.o

# change to build dir
cd $build

# create a brand new disk image
cat /dev/zero | head -c 16777216 > sd.img

# link mkfs tool: copyin
ftmp=/tmp/mkfs.txt
echo "Creatiing temp file" $ftmp

# build these files and dirs.
echo "w /init $1" > $ftmp
echo "m /home 0" >> $ftmp
echo "m /bin 0" >> $ftmp
echo "w /home/README.txt README.txt" >> $ftmp

# install all user progs
for up in $uprog; do
    echo "w /bin/$uprog $uprog" >> $ftmp
done

cat $ftmp
./copyin < $ftmp
if [[ $? -ne 0 ]]; then
    echo "Abort" 1>&2
fi

# hexdump -C sd.img | less
rm $ftmp
