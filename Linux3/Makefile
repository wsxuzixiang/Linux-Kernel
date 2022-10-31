# 定义内核源码的目录
KERNELDIR ?=/lib/modules/$(shell uname -r)/build
# 定义当前目录
PWD        := $(shell pwd)
# 要生成的内核模块
obj-m := autoNode.o

all:
	make -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mode.c .tmp_versions *.mod *.order *.symvers *.mod.c
