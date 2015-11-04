#!/bin/bash
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
