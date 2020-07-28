KBUILD_DIR ?= /lib/modules/$(shell uname -r)/build

obj-m := srvfs.o

srvfs-objs := \
	srvfs-main.o \
	file.o \
	super.o \
	root.o

KVER := $(shell uname -r)
KPATH := /lib/modules/$(KVER)/build

all:
	$(MAKE) -C $(KPATH) M=$(CURDIR)

clean:
	$(MAKE) -C $(KPATH) M=$(CURDIR) clean
