KBUILD_DIR ?= /lib/modules/$(shell uname -r)/build

# SPDX-License-Identifier: GPL-2.0
obj-m := srvfs.o

srvfs-objs := \
	srvfs-main.o

#
# Kernel module build dependency
#all:
#	make -C $(KBUILD_DIR) M=$(PWD) modules
#
# Kernel module clean dependency
#clean:
#	make -C $(KBUILD_DIR) M=$(PWD) clean

KVER := $(shell uname -r)
KPATH := /lib/modules/$(KVER)/build

all:
	$(MAKE) -C $(KPATH) M=$(CURDIR)
