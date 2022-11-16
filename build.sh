#!/bin/bash


upstream() {
if [ -f $PWD/*.patch ]; then
	clear
	echo " ";
	echo -e "**** Patching kernel ****";
	echo " ";
	patch -N -p1 < *.patch
	rm -f $PWD/*.patch
	#clear
	echo " ";
	echo -e "$blue**** Patching kernel finished ****";
else
	clear
	echo " ";
	echo -e "**** Patch kernel no found ****";
	echo " ";
	echo "Make patch file from diff, and place into Kernel dir ";
	echo -e "with name oxygen.patch";
fi
}


upstream;

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
				

