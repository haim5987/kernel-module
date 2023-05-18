obj-m += new_sys_call.o

# Path to the kernel source code
KDIR := /lib/modules/$(shell uname -r)/build

# Path to the current directory
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
