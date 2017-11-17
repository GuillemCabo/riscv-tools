cd $TOP/riscv-tools
echo Backing up previous Busybox directory \(if any\)
mkdir -p busybox-1.21.1
mv -f busybox-1.21.1{,-old-`date -Isec`}
curl -L http://busybox.net/downloads/busybox-1.21.1.tar.bz2 | tar -xj
cp busybox_config.fpga busybox-1.21.1/.config
