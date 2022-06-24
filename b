#!/bin/bash

#set -e

KERNEL_DEFCONFIG=vendor/alioth_defconfig
ANYKERNEL3_DIR=$PWD/AnyKernel3/
FINAL_KERNEL_ZIP=InfiniR_Alioth_v2.21.zip
export ARCH=arm64
export KBUILD_BUILD_HOST="Venoom"
export KBUILD_BUILD_USER="WartegCI"
TC_DIR="/home/space/kernel/cl"
GCC64_DIR="/home/space/kernel/gcc/toolchains/aarch64-linux-android-4.9/prebuilt/linux-x86_64"
GCC32_DIR="/home/space/kernel/gcc/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64"
export PATH="$TC_DIR/bin:$PATH"
export KBUILD_COMPILER_STRING="/home/space/cl/bin/clang --version | head -n 1 | perl -pe 's/\(http.*?\)//gs' | sed -e 's/  */ /g' -e 's/[[:space:]]*$//')"

# Speed up build process
MAKE="./makeparallel"

BUILD_START=$(date +"%s")
blue='\033[1;34m'
yellow='\033[1;33m'
nocol='\033[0m'

# Always do clean build lol
echo -e "$yellow**** Cleaning ****$nocol"
mkdir -p out
make O=out clean

echo -e "$yellow**** Kernel defconfig is set to $KERNEL_DEFCONFIG ****$nocol"
echo -e "$blue***********************************************"
echo "          BUILDING KERNEL          "
echo -e "***********************************************$nocol"
make $KERNEL_DEFCONFIG O=out
#make -j$(nproc --all) O=out \
#                      ARCH=arm64 \
#                      CC=/home/space/kernel/cl/bin/clang \
#                      LD=ld.lld \
#      		       AS=llvm-as \
#                      CLANG_TRIPLE=aarch64-linux-gnu- \
#                      CROSS_COMPILE=/home/space/kernel/gcc64/bin/aarch64-linux-android- \
#                      CROSS_COMPILE_ARM32=/home/space/kernel/gcc32/bin/arm-linux-androideabi-

#make -j$(nproc --all) O=out ARCH=arm64 CC=clang LD=ld.lld AS=llvm-as AR=llvm-ar NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip CROSS_COMPILE=aarch64-linux-gnu- CROSS_COMPILE_ARM32=arm-linux-gnueabi- CLANG_TRIPLE=aarch64-linux-gnu-
make -j$(nproc --all) O=out ARCH=arm64 CC=clang OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip CROSS_COMPILE=aarch64-linux-gnu- CROSS_COMPILE_ARM32=arm-linux-gnueabi- CLANG_TRIPLE=aarch64-linux-gnu-

echo -e "$yellow**** Verify Image.gz-dtb & dtbo.img ****$nocol"
ls $PWD/out/arch/arm64/boot/Image.gz-dtb
ls $PWD/out/arch/arm64/boot/dtbo.img

echo -e "$yellow**** Verifying AnyKernel3 Directory ****$nocol"
ls $ANYKERNEL3_DIR
echo -e "$yellow**** Removing leftovers ****$nocol"
rm -rf $ANYKERNEL3_DIR/Image.gz-dtb
rm -rf $ANYKERNEL3_DIR/dtbo.img
rm -rf $ANYKERNEL3_DIR/$FINAL_KERNEL_ZIP

echo -e "$yellow**** Copying Image.gz-dtb & dtbo.img ****$nocol"
cp $PWD/out/arch/arm64/boot/Image.gz-dtb $ANYKERNEL3_DIR/
cp $PWD/out/arch/arm64/boot/dtbo.img $ANYKERNEL3_DIR/

echo -e "$yellow**** Time to zip up! ****$nocol"
cd $ANYKERNEL3_DIR/
zip -r9 $FINAL_KERNEL_ZIP * -x README $FINAL_KERNEL_ZIP
cp $ANYKERNEL3_DIR/$FINAL_KERNEL_ZIP /home/raystef66/kernel/$FINAL_KERNEL_ZIP

echo -e "$yellow**** Done, here is your checksum ****$nocol"
cd ..
rm -rf $ANYKERNEL3_DIR/$FINAL_KERNEL_ZIP
rm -rf $ANYKERNEL3_DIR/Image.gz-dtb
rm -rf $ANYKERNEL3_DIR/dtbo.img
rm -rf out/

BUILD_END=$(date +"%s")
DIFF=$(($BUILD_END - $BUILD_START))
echo -e "$yellow Build completed in $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds.$nocol"
sha1sum $KERNELDIR/$FINAL_KERNEL_ZIP
