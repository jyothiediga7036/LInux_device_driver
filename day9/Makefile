#obj-m += gpio_driver.o
#obj-m += usb_Driver.o
obj-m += usb_mouse.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
