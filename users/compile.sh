#!/usr/bin/bash
# compile all user programs and tests, then put in
# the build directory and disk image.

### General configurations. ###
build=../../../mkfs
tooldir=../../fs/test/build/

# create install dir if not exist.
ls $build > /dev/null 2>&1
if [[ $? -ne 0 ]]; then
    mkdir -p $build
fi

ls $build/copyin > /dev/null 2>&1
if [[ $? -ne 0 ]]; then
    cp $tooldir/copyin $build/copyin
    cp $tooldir/copyout $build/copyout
fi

home=$( ./home.sh )
tests="init"

### Toolchain
CROSS=""
CC="$CORSS"gcc
LD="$CROSS"ld
STRIP="$CROSS"strip
CFLAGS="-static -ffreestanding -O3 -fno-plt -fno-pic -mgeneral-regs-only -Wall -Werror"

uprog=$( ./uprog.sh )
if [[ $# -ne 1 ]]; then
	echo "No test cases specified. Use default uprog.sh" 2>&1
else
	uprog=$( $1 )
fi

ls init.c > /dev/null 2>&1
if [[ $? -ne 0 ]]; then
    echo "init.c not found, required."
    exit 1
fi

ls sh.c > /dev/null 2>&1
if [[ $? -ne 0 ]]; then
    echo "sh.c not found, required."
    exit 1
fi

check() {
    echo "Checking $1 usability..."
    $1 -v > /dev/null 2>&1
    if [[ $? -ne 0 ]]; then
        echo "$1 not usable. Abort" 1>&2
        exit 1
    fi
    echo "OK"
}

check $CC
check $LD

echo "Building start.o..."
$CC $CFLAGS -c start.S -o start.o
if [[ $? -ne 0 ]]; then
    echo "Build start.o exited with $?. Abort" 1>&2
    exit 1
fi

for up in $uprog; do
    echo "Buiding user program $up..."
    $CC $CFLAGS -c $up.c -o $up.o && $LD start.o $up.o -o $up
    if [[ $? -ne 0 ]]; then
        echo "Abort" 1>&2
        exit 1
    fi
	$STRIP $up
    mv $up $build/$up
done

for up in $tests; do
    echo "Buiding test program $up..."
    $CC $CFLAGS -c $up.c -o $up.o && $LD start.o $up.o -o $up
    if [[ $? -ne 0 ]]; then
        echo "Abort" 1>&2
        exit 1
    fi
	$STRIP $up # stripped executable
    mv $up $build/$up
done

for file in $home; do
    link $file $build/$file > /dev/null 2>&1
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
