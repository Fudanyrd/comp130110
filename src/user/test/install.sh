# install user program in the build directory, for testing.
# usage: install foo.c
# where foo.c is the test you want to run.

# if this runs without any error, run `make qemu` to see test result.

PWD=$( pwd )
BUILD_DIR=../../../build
ls $BUILD_DIR > /dev/null 2> /dev/null

if [ $? -ne 0 ]; then 
    echo "Build directory $BUILD_DIR does not exist." 1>&2
    echo "Abort" 1>&2
    exit 1
fi

ls $BUILD_DIR/copyin > /dev/null 2> /dev/null
if [ $? -ne 0 ]; then
    echo "Disk image builder copyin is not found. " 1>&2
    echo "Abort" 1>&2
    exit 1
fi 

files=$( ls ./ )
for file in $files; do 
    link $file "$BUILD_DIR/$file"
done

cd $BUILD_DIR
bash fdu-mkfs.sh $1

if [ $? -ne 0 ]; then
    echo "Build script failed. Abort" 1>&2
    exit 1
fi

for file in $files; do
    rm -f $file
done

# remove object file, leave init there.
rm *.o
cd $PWD
