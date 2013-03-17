#!/bin/sh

insmod tarfs.ko

lsmod

mount -t tarfs ../test.tar /mnt/testtar/

echo "FILES WITHIN /mnt/testtar/"
ls /mnt/testtar/

echo "Contents of filea:"
cat /mnt/testtar/filea

echo "Overwriting filea..."
echo "The new contents in file a" > /mnt/testtar/filea

echo "New contents of filea:"
cat /mnt/testtar/filea

umount /mnt/testtar/

rmmod tarfs

lsmod


