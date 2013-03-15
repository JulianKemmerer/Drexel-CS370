#Based on makefile shown here
#http://www.cyberciti.biz/tips/compiling-linux-kernel-module.html

obj-m = tarfs.o
KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
