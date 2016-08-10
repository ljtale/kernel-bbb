#!/bin/bash
# Simple build script for this kernel, the script takes 1 or 0
# arguement, as specified below. The kernel image and device tree blob
# are compiled into separate binaries.
# By Jie Liao

clear

if [ $# == 1 ]
then
    
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

    if [ $1 == menuconfig ]
    then 
        make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- bb.org_defconfig
        make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- menuconfig
        exit
    fi
    
    # the following two lines should only execute once unless the kernel
    # configuration need to be changed
    
    if [ $1 == reconfig ]
    then 
        make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- bb.org_defconfig
        make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- menuconfig
    fi
fi
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -j4
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- uImage dtbs LOADADDR=0X80008000 -j4
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- modules -j4
