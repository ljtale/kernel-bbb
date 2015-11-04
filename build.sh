#!/bin/bash
# Simple build script for this kernel, the script takes 1 or 0
# arguement, as specified below. The kernel image and device tree blob
# are compiled into one binary.
# By Jie Liao
if [ $1 == clean ]
then
    make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- clean
    exit
fi

if [ $1 == mrproper ]
then
    make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- mrproper
    exit
fi

# the following two lines should only execute once unless the kernel
# configuration need to be changed

if [ $1 -eq reconfig ]
then 
    make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- beaglebone_defconfig
    make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- menuconfig
fi
 
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- uImage dtbs
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- uImage-dtb.am335x-boneblack
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- modules
