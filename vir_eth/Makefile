CONFIG_MODULE_SIG=n

ifneq ($(KERNELRELEASE),)
        obj-m := myVir.o
else
        KERNELDIR ?= /lib/modules/$(shell uname -r)/build
        PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules 
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
endif
