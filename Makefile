# If KERNELRELEASE is defined, we've been invoked from the
# kernel  build system and can use its language.

ifneq ($(KERNELRELEASE),)
	obj-m := simple.o

# Otherwise we were called directly fom the command line
#  invoke the kernel build system.

else

	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

test:
	mount /dev/loop0 /mnt
	cp * /mnt/root/simple 
	$(shell qemu-system-x86_64 -s -kernel testing/bzImage -drive file=testing/rootfs.ext4 -append "root=/dev/sda")
	umount /mnt
endif
