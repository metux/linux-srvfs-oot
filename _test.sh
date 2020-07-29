
TESTDIR=/xs
MODULE=srvfs
FSNAME=srvfs

set -e

do_unmount() {
	if mountpoint $TESTDIR >/dev/null; then
		echo "unmounting"
		umount $TESTDIR
	else
		echo "not mounted"
	fi
}

do_unload() {
	if cat /proc/modules | grep "$MODULE " >/dev/null; then
		echo "unloading module"
		rmmod $MODULE
	else
		echo "module not loaded"
	fi
}

do_load() {
	if cat /proc/modules | grep "$MODULE " >/dev/null; then
		echo "module already loaded"
	else
		echo "loading module"
		insmod kernel/$MODULE.ko
	fi
}

do_mount() {
	mkdir -p $TESTDIR
	mount none $TESTDIR -t $FSNAME
}

do_list() {
	ls -la $TESTDIR
}
