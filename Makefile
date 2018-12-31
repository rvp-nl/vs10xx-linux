
BASE_DIR = $(shell pwd)

export ARCH = arm

# Point these to the kernel source and toolchain
export KROOT = /PATH/TO/linux-2.6.33.9
export CROSS_COMPILE = /PATH/TO/tlchn/bin/arm-unknown-linux-uclibcgnueabi-

export CC = $(CROSS_COMPILE)gcc
export SPARSEFLAGS = C=2

all:
	cd module && $(MAKE) $(MAKEFLAGS) $(SPARSEFLAGS) all
	cd ioctl && $(MAKE) $(MAKEFLAGS) $(SPARSEFLAGS) all

clean:
	$(MAKE) -C module clean
	$(MAKE) -C ioctl clean
	rm -f vs10xx.tar.gz

package: all
	rm -rf temp
	mkdir temp
	cp module/vs10xx.ko ioctl/ioctl temp/
	(cd temp ; tar cvzf ../vs10xx.tar.gz vs10xx.ko ioctl)
	rm -rf temp

