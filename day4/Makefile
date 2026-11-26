#obj-m += ioctl_revstr_driver.o
#obj-m += interrupt_basic.o
#obj-m += key_init.o
#obj-m += timer_interrupt.o
#obj-m += tasklet_interrupt.o
#obj-m += execption.o // don't run bcoz systm can reboot
#obj-m += kmalloc_driver1.o
obj-m += vmalloc_driver.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
