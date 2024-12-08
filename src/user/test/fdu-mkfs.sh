# make a new disk: sd.img that can be loaded by qemu.
# the copyin is provided in src/fs/test/build/

if [ $# -ne 1 ]; then
    echo "Usage ./fdu-mkfs.sh source.c"
    echo "  source.c is the c source file you want to use as init."
    exit 1
fi

c=$1
obj=$( echo $1 | sed "s/\.c/\.o/" )

# build init program to run the test.
echo "Buiding object file" $obj
gcc -static -ffreestanding -O3 -c $1
echo "Buiding object file" start.o
gcc -static -ffreestanding -c start.S -o start.o
echo "Linking init"
ld start.o $obj -o init

cat /dev/zero | head -c 16777216 > sd.img
ftmp=/tmp/mkfs.txt
echo "Creatiing temp file" $ftmp
echo "w /init init" > $ftmp
./copyin < $ftmp
# hexdump -C sd.img | less
rm $ftmp
