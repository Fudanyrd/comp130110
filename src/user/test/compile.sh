#!/usr/bin/bash
# compile all user programs and tests, then put in
# the build directory and disk image.

uprog=$( ./uprog.sh )
if [[ $# -ne 1 ]]; then
	echo "No test cases specified. Use default uprog.sh" 2>&1
else
	uprog=$( $1 )
fi

ls init.c
if [[ $? -ne 0 ]]; then
    echo "init.c not found, required."
    exit 1
fi

ls sh.c
if [[ $? -ne 0 ]]; then
    echo "sh.c not found, required."
    exit 1
fi

build=../../../mkfs
home=$( ./home.sh )
tests="init"
cflags="-static -ffreestanding -O3 -fno-plt -fno-pic -mgeneral-regs-only"

echo "Building start.o..."
gcc $cflags -c start.S -o start.o

for up in $uprog; do
    echo "Buiding user program $up..."
    gcc $cflags -c $up.c -o $up.o && ld start.o $up.o -o $up
    if [[ $? -ne 0 ]]; then
        echo "Abort" 1>&2
        exit 1
    fi
	strip $up
    mv $up $build/$up
done

for up in $tests; do
    echo "Buiding test program $up..."
    gcc $cflags -c $up.c -o $up.o && ld start.o $up.o -o $up
    if [[ $? -ne 0 ]]; then
        echo "Abort" 1>&2
        exit 1
    fi
	strip $up # stripped executable
    mv $up $build/$up
done

for file in $home; do
    link $file $build/$file
done

# clean
rm *.o

# change to build dir
cd $build
echo "Now at directory $( pwd )"

# create a brand new disk image
cat /dev/zero | head -c 16777216 > sd.img

# link mkfs tool: copyin
ftmp=/tmp/mkfs.txt
echo "Creating copyin script" $ftmp

# build these files and dirs.
echo "w /init init" > $ftmp
echo "m /home 0" >> $ftmp
echo "m /bin 0" >> $ftmp

# install all user progs
for up in $uprog; do
    echo "w /bin/$up $up" >> $ftmp
done
for file in $home; do
    echo "w /home/$file $file" >> $ftmp
done

cat $ftmp
./copyin < $ftmp
if [[ $? -ne 0 ]]; then
    echo "Abort" 1>&2
    exit 1
fi

# hexdump -C sd.img | less
rm $ftmp

for file in $home; do
    rm $file
done
