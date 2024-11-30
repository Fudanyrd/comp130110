cd /root/OS-24Fall-FDU/src/fs/test/build
ls ./mkfs.c
if [[ $? -ne 0 ]]; then
    ln ../mkfs.c ./mkfs.c
fi
ls ./readfs.c
if [[ $? -ne 0 ]]; then
    ln ../readfs.c ./readfs.c
fi

make
cat /dev/zero | head -c 16777216 > sd.img
# hexdump -C sd.img | less
