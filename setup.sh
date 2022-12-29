#!/bin/bash

sudo apt-get update && apt-get install -y bison build-essential bc bison curl libssl-dev git zip python python3 flex cpio libncurses5-dev wget

git clone https://github.com/piraterex/AnyKernel3.git -b penguin
# Clone compiler
git clone --depth=1 https://github.com/piraterex/zyc_clang-14 clang
