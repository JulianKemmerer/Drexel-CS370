#!/bin/sh

insmod tarfs.ko

lsmod

mount -t tarfs ../test.tar /mnt/testtar/

echo "FILES WITHIN /mnt/testtar/"
ls /mnt/testtar/

umount /mnt/testtar/

rmmod tarfs

lsmod


