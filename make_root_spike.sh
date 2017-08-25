#!/bin/bash
#
# help:
# BUSYBOX=path/to/busybox LINUX=path/to/linux LOWRISC=path/to/lowrisc make_root.sh

if [ -z "$BUSYBOX" ]; then BUSYBOX=$TOP/riscv-tools/busybox-1.21.1; fi
BUSYBOX_CFG=$TOP/riscv-tools/busybox_config.spike

ROOT_INITTAB=$TOP/riscv-tools/inittab

if [ -z "$LINUX" ]; then LINUX=$TOP/riscv-tools/linux-4.6.2; fi
LINUX_CFG=$TOP/riscv-tools/vmlinux_config.spike


CDIR=$PWD

if [ -d "$BUSYBOX" ] && [ -d "$LINUX" ]; then
    echo "build busybox..."
    cp  $BUSYBOX_CFG "$BUSYBOX"/.config &&
    make -j$(nproc) -C "$BUSYBOX" 2>&1 1>/dev/null &&
    if [ -d ramfs ]; then rm -fr ramfs; fi &&
    mkdir ramfs && cd ramfs &&
    mkdir -p bin etc dev lib mnt proc sbin sys tmp usr usr/bin usr/lib usr/sbin &&
    cp "$BUSYBOX"/busybox bin/ &&
    ln -s bin/busybox ./init &&
    cp $ROOT_INITTAB etc/inittab &&
    printf "#!/bin/sh\necho 0 > /proc/sys/kernel/randomize_va_space\n" > disable_aslr.sh &&
    chmod +x disable_aslr.sh &&
    cp ../zram_swap.sh . &&
    cp ../ramdisk_swap.sh . &&
    riscv64-unknown-linux-gnu-gcc -O1 -static ../ptr_elf_tagged.c -o ptr_elf_tagged &&
    ../elf_tagger ptr_elf_tagged ../ptr_elf_tagged_tags ../ptr_elf_tagged_trace &&
    riscv64-unknown-linux-gnu-objcopy --add-section .tags=../ptr_elf_tagged_tags ptr_elf_tagged &&
    riscv64-unknown-linux-gnu-gcc -O1 -static ../ptr_vuln.c -o ptr_vuln &&
    riscv64-unknown-linux-gnu-gcc -O1 -static ../stack_elf_tagged.c -o stack_elf_tagged &&
    ../elf_tagger stack_elf_tagged ../stack_elf_tagged_tags ../stack_elf_tagged_trace &&
    riscv64-unknown-linux-gnu-objcopy --add-section .tags=../stack_elf_tagged_tags stack_elf_tagged &&
    riscv64-unknown-linux-gnu-gcc -O1 -static ../dummy_tag_test.c -o dummy_tag_test &&
    ../elf_tagger dummy_tag_test ../dummy_tag_test_tags ../dummy_tag_test_trace &&
    riscv64-unknown-linux-gnu-objcopy --add-section .tags=../dummy_tag_test_tags dummy_tag_test &&
    riscv64-unknown-linux-gnu-gcc -O1 -static ../sigtest.c -o sigtest &&
    riscv64-unknown-linux-gnu-gcc -O1 -static ../tagctrl.c -o tagctrl &&
    riscv64-unknown-linux-gnu-gcc -O1 -static ../memtagger.c -o memtagger &&
    riscv64-unknown-linux-gnu-gcc -O1 -static ../cow_test.c -o cow_test &&
    riscv64-unknown-linux-gnu-gcc -O1 -static ../memeater.c -o memeater &&
    riscv64-unknown-linux-gnu-gcc -O1 -static ../memeater_simple.c -o memeater_simple &&
    riscv64-unknown-linux-gnu-gcc -O1 -static ../hello_tagctrl.c -o hello &&
    riscv64-unknown-linux-gnu-gcc -O1 -static ../elf_tag_checker.c -o elf_tag_checker &&
    ../elf_tagger elf_tag_checker ../elf_tag_checker_tags ../elf_tag_checker_trace &&
    riscv64-unknown-linux-gnu-objcopy --add-section .tags=../elf_tag_checker_tags elf_tag_checker &&
    riscv64-unknown-linux-gnu-gcc -O2 -static ../stack_vuln.c -o stack_vuln &&
    riscv64-unknown-linux-gnu-gcc -O2 -static ../stack_tagged.c -o stack_tagged &&
    riscv64-unknown-linux-gnu-gcc -O2 -static ../secret_vuln.c -o secret_vuln &&
    riscv64-unknown-linux-gnu-gcc -O2 -static ../secret_tagged.c -o secret_tagged &&
    echo "\
        mknod dev/null c 1 3 && \
        mknod dev/tty c 5 0 && \
        mknod dev/zero c 1 5 && \
        mknod dev/console c 5 1 && \
        mknod dev/zram0 b 254 0 && \
        mknod dev/ram0  b 1 0 && \
        mknod dev/loop0 b 7 8 && \
        find . | cpio -H newc -o > "$LINUX"/initramfs.cpio\
        " | fakeroot &&
    if [ $? -ne 0 ]; then echo "build busybox failed!"; fi &&
    \
    echo "build linux..." &&
    cp $LINUX_CFG "$LINUX"/.config &&
    make -j$(nproc) -C "$LINUX" ARCH=riscv vmlinux 2>&1 1>/dev/null &&
    if [ $? -ne 0 ]; then echo "build linux failed!"; fi &&
    \
    echo "build bbl..." &&
    if [ ! -d $TOP/riscv-tools/riscv-pk/build ]; then
        mkdir -p $TOP/riscv-tools/riscv-pk/build
    fi   &&
    cd $TOP/riscv-tools/riscv-pk/build &&
    ../configure \
        --host=riscv64-unknown-elf \
        --with-payload="$LINUX"/vmlinux \
        2>&1 1>/dev/null &&
    make -j$(nproc) bbl 2>&1 1>/dev/null &&
    if [ $? -ne 0 ]; then echo "build bbl failed!"; fi &&
    \
    cd "$CDIR"
    cp $TOP/riscv-tools/riscv-pk/build/bbl ./boot.bin
else
    echo "make sure you have both linux and busybox downloaded."
    echo "usage:  [BUSYBOX=path/to/busybox] [LINUX=path/to/linux] make_root.sh"
fi
