#!/bin/bash
#

CDIR=$PWD

    echo "build bbl..." &&
    if [ ! -d $TOP/fpga/bootloader/build ]; then
        mkdir -p $TOP/fpga/bootloader/build
    fi   &&
    cd $TOP/fpga/bootloader/build &&
    ../configure \
        --host=riscv64-unknown-elf &&
    make -j$(nproc) bbl &&
    if [ $? -ne 0 ]; then echo "build bbl failed!"; fi &&
    cp $TOP/fpga/bootloader/build/bbl $CDIR/test$1.bin
