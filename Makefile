
all:
	$(MAKE) -C kernel all
	$(MAKE) -C tests all

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C tests clean
