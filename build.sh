#!/bin/bash
cd ..
MAIN=$PWD
cd kernel_xiaomi_sm8250
KERNEL_DIR=$PWD
ANYKERNEL_DIR=$KERNEL_DIR/AnyKernel3
CLANG_DIR=$MAIN/zyc-clang
DATE_CLOCK=$(date +'%H%M-%d%m%Y')
KERNELVER=$(make kernelversion)
ZIPNAME=$KERNELVER-Oxygen+-$DATE_CLOCK.zip


echo "cleaning..."
mkdir -p out

BUILD_START=$(date +"%s")

make O=out ARCH=arm64 oxygen_defconfig

PATH="$CLANG_DIR/bin:${PATH}" \
make 				O=out \
				ARCH=arm64 \
				CC=clang \
				CROSS_COMPILE=aarch64-linux-gnu- \
				CROSS_COMPILE_ARM32=arm-linux-gnueabi- \
				-j9


mv $KERNEL_DIR/out/arch/arm64/boot/Image.gz-dtb $ANYKERNEL_DIR/
mv $KERNEL_DIR/out/arch/arm64/boot/dtbo.img $ANYKERNEL_DIR/
cd $ANYKERNEL_DIR
echo ""
echo "zipping"
zip -r9 $ZIPNAME META-INF ramdisk tools anykernel.sh banner Image.gz-dtb dtbo.img
mv $ZIPNAME $KERNEL_DIR/
rm -rf $KERNEL_DIR/out/
echo ""

echo "Cleaning AnyKernel3 dir"
echo ""
rm -rf $ANYKERNEL_DIR/*.zip
rm -rf $ANYKERNEL_DIR/Image.gz-dtb
rm -rf $ANYKERNEL_DIR/dtbo.img

echo "****** build done! ******"
echo ""
BUILD_END=$(date +"%s")
DIFF=$(($BUILD_END - $BUILD_START))
echo -e "Build completed in '$(($DIFF / 60))' minute(s) and '$(($DIFF % 60))' seconds"

