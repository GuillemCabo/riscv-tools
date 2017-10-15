cd $TOP/riscv-tools
echo Backing up previous Linux directory \(if any\)
mkdir -p linux-4.6.2
mv -f linux-4.6.2{,-old-`date -Isec`}
curl https://www.kernel.org/pub/linux/kernel/v4.x/linux-4.6.2.tar.xz | tar -xJ
cd linux-4.6.2
git init
git remote add origin https://github.com/lowrisc/riscv-linux.git
git fetch
git checkout -f -t origin/ethernet-v0.5
cd ..
echo Backing up previous Busybox directory \(if any\)
mkdir -p busybox-1.21.1
mv -f busybox-1.21.1{,-old-`date -Isec`}
curl -L http://busybox.net/downloads/busybox-1.21.1.tar.bz2 | tar -xj
