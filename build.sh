#!/bin/bash



if [[-d "out"]]
then
   cd out && make clean && make mrproper && cd ..
else
    mkdir -p out
fi

make O=out ARCH=arm64 oxygen_defconfig

PATH="/home/danda/Desktop/Kernel/zyc-clang/bin:${PATH}" \
make 				O=out \
				ARCH=arm64 \
				CC=clang \
				CROSS_COMPILE=aarch64-linux-gnu- \
				CROSS_COMPILE_ARM32=arm-linux-gnueabi- \
				-j9
				

