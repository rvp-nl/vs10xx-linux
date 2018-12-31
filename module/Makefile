
vs10xx-objs := vs10xx_iocomm.o vs10xx_queue.o vs10xx_device.o vs10xx_main.o

obj-m +=  vs10xx.o

all: modules

modules:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules

modules_install:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules_install

kernel_clean:
	@$(MAKE) -C $(KROOT) M=$(PWD) clean

clean: kernel_clean
	rm -rf Module.symvers modules.order .tmp_versions

