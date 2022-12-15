IMAGE=$(pwd)/out/arch/arm64/boot/Image
DTBO=$(pwd)/out/arch/arm64/boot/dtbo.img
START=$(date +"%s")
BRANCH=$(git rev-parse --abbrev-ref HEAD)
VERSION=Venoom
TANGGAL=$(TZ=Asia/Jakarta date "+%Y%m%d-%H%M")
KBUILD_COMPILER_STRING=$(/root/kernel/16/bin/clang --version | head -n 1 | perl -pe 's/\(http.*?\)//gs' | sed -e 's/  */ /g' -e 's/[[:space:]]*$//')
KBUILD_LINKER_STRING=$(/root/kernel/16/bin/ld.lld --version  | head -n 1 | perl -pe 's/\(http.*?\)//gs' | sed -e 's/  */ /g' -e 's/[[:space:]]*$//' | sed 's/(compatible with [^)]*)//')

# Set Kernel Version
KERNELVER=$(make kernelversion)

# Include argument
ARGS="ARCH=arm64 \
        O=out \
	CC=clang \
	LLVM=1 \
#	CLANG_TRIPLE=aarch64-linux-gnu- \
#	CROSS_COMPILE=aarch64-linux-gnu- \
	CROSS_COMPILE_COMPAT=arm-linux-gnueabi- \
	CROSS_COMPILE=aarch64-linux-gnu- \
	CROSS_COMPILE_ARM32=arm-linux-gnueabi- \
        -j48"

# Build Kernel
export ARCH=arm64
export SUBARCH=arm64
export KBUILD_BUILD_HOST="WartegCI"
export KBUILD_BUILD_USER="Pocongtobat"
export KBUILD_COMPILER_STRING
export KBUILD_LINKER_STRING
#main group
#export chat_id="-1001726996867!"
#channel
#export chat_id2="-1001608547174!"
export chat_id="-1001551521202"
export TOKEN="5452672785:AAFh0ke8wGsfvk1qzEnZKi3fJ_eZvpOx1Rc!"
export DEF="alioth_defconfig"
TC_DIR=/root/kernel/16
GCC64_DIR="/root/kernel/gcc/toolchains/aarch64-linux-android-4.9/prebuilt/linux-x86_64"
GCC32_DIR="/root/kernel/gcc/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64"
export PATH="/root/kernel/16/bin:${PATH}"
export LD_LIBRARY_PATH=/root/kernel/16/lib64:$LD_LIBRARY_PATH

# Post to CI channel
curl -s -X POST https://api.telegram.org/bot${TOKEN}/sendMessage -d text="Buckle up bois Venoom  kernel build has started" -d chat_id=${chat_id} -d parse_mode=HTML
curl -s -X POST https://api.telegram.org/bot${TOKEN}/sendMessage -d text="Branch: <code>$(git rev-parse --abbrev-ref HEAD)</code>
Kernel Version : <code>${KERNELVER}</code>
Compiler Used : <code>${KBUILD_COMPILER_STRING}</code>
Latest Commit: <code>$(git log --pretty=format:'%h : %s' -1)</code>
Starting..." -d chat_id=${chat_id} -d parse_mode=HTML

# make defconfig
    make -j48 ${ARGS} ${DEF}

# Make olddefconfig
cd out || exit
make -j48 ${ARGS} olddefconfig
cd ../ || exit

# compiling
    make -j$(nproc --all) ${ARGS} 2>&1 | tee build.log

END=$(date +"%s")
DIFF=$((END - START))

if [ -f $(pwd)/out/arch/arm64/boot/Image ]
	then
	curl -s -X POST https://api.telegram.org/bot${TOKEN}/sendMessage -d text="<i>Build compiled successfully in $((DIFF / 60)) minute(s) and $((DIFF % 60)) seconds</i>" -d chat_id=${chat_id} -d parse_mode=HTML
        cp ${IMAGE} $(pwd)/AnyKernel3
        cp ${DTBO} $(pwd)/AnyKernel3
        cd AnyKernel3
        zip -r9 Venoom-${TANGGAL}.zip * --exclude *.jar

        curl -F chat_id="${chat_id}"  \
                    -F caption="sha1sum: $(sha1sum Venoom*.zip | awk '{ print $1 }')" \
                    -F document=@"$(pwd)/Venoom-${TANGGAL}.zip" \
                    https://api.telegram.org/bot${TOKEN}/sendDocument

        curl -s -X POST https://api.telegram.org/bot${TOKEN}/sendMessage -d text="hi guys, the latest update is available !" -d chat_id=${chat_id2} -d parse_mode=HTML

cd ..
else
        curl -F chat_id="${chat_id}"  \
                    -F caption="Build ended with an error !!" \
                    -F document=@"build.log" \
                    https://api.telegram.org/bot${TOKEN}/sendDocument

fi
