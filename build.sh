#!/bin/bash

#set -e
KERNEL_DIR=$(pwd)
KERNEL_DEFCONFIG=vendor/alioth_defconfig
DEVICE=aliothin
ANYKERNEL3_DIR=AnyKernel3
TYPE=REL-1.0
DATE=$(date +"%d.%m.%y")
VERSION="Penguin-${DEVICE}-${TYPE}-${DATE}"
# Export Zip name
export ZIPNAME="${VERSION}.zip"
export PATH="/home/piraterex/clang/bin:${PATH}"
export ARCH=arm64
export SUBARCH=arm64
export KBUILD_COMPILER_STRING="$(/home/piraterex/clang/bin/clang --version | head -n 1 | perl -pe 's/\(http.*?\)//gs' | sed -e 's/  */ /g' -e 's/[[:space:]]*$//')"

BUILD_START=$(date +"%s")
blue='\033[1;34m'
yellow='\033[1;33m'
nocol='\033[0m'

# Speed up build process
MAKE="./makeparallel"


#git clone --depth=1 https://github.com/EmanuelCN/zyc_clang-14 clang
#anykernel
git clone https://github.com/dereference23/AnyKernel3.git AnyKernel3
make $KERNEL_DEFCONFIG O=out CC=clang
make -j$(nproc --all) O=out \
                      ARCH=arm64 \
                      CC=clang \
                      CROSS_COMPILE=aarch64-linux-gnu- \
                      NM=llvm-nm \
                      OBJDUMP=llvm-objdump \
                      STRIP=llvm-strip

echo -e "$yellow**** Verify Image.gz-dtb & dtbo.img ****$nocol"
ls out/arch/arm64/boot/Image
ls out/arch/arm64/boot/dtbo.img

echo -e "$yellow**** Verifying AnyKernel3 Directory ****$nocol"
ls $ANYKERNEL3_DIR
echo -e "$yellow**** Removing leftovers ****$nocol"
rm -rf $ANYKERNEL3_DIR/Image
rm -rf $ANYKERNEL3_DIR/dtbo.img
rm -rf $ANYKERNEL3_DIR/$ZIPNAME

echo -e "$yellow**** Copying Image.gz-dtb & dtbo.img ****$nocol"
cp out/arch/arm64/boot/Image $ANYKERNEL3_DIR/
cp out/arch/arm64/boot/dtbo.img $ANYKERNEL3_DIR/

echo -e "$yellow**** Time to zip up! ****$nocol"
cd $ANYKERNEL3_DIR/
zip -r9 $ZIPNAME * -x README $ZIPNAME
cp $ANYKERNEL3_DIR/$ZIPNAME ../

echo -e "$yellow**** Done, here is your checksum ****$nocol"
cd ..
# rm -rf $ANYKERNEL3_DIR/$ZIPNAME
rm -rf $ANYKERNEL3_DIR/Image
rm -rf $ANYKERNEL3_DIR/dtbo.img
#rm -rf out/

BUILD_END=$(date +"%s")
DIFF=$(($BUILD_END - $BUILD_START))
echo -e "$yellow Build completed in $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds.$nocol"
