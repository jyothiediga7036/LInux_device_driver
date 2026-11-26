obj-m += pci_demo_driver.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all module app clean

all: module app

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

app: pcie_test

pcie_test: pcie_test.c
	$(CC) -Wall -Wextra -Wpedantic -O2 -o $@ $<

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) pcie_test
