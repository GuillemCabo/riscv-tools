cd $TOP/riscv-tools
mkdir -p linux-4.6.2
mv -f linux-4.6.2{,-old-`date -Isec`}
curl https://www.kernel.org/pub/linux/kernel/v4.x/linux-4.6.2.tar.xz | tar -xJ
cd linux-4.6.2
git init
git remote add origin https://github.com/lowrisc/riscv-linux.git
git fetch
git checkout -f -t origin/update-ramtest
patch -p1 < spi_sd_power_hack.patch
