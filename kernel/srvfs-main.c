#include "srvfs.h"

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

struct dentry *srvfs_mount(struct file_system_type *fs_type,
			   int flags, const char *dev_name, void *data)
{
	return mount_nodev(fs_type, flags, data, srvfs_fill_super);
}

static struct file_system_type srvfs_type = {
	.owner 		= THIS_MODULE,
	.name		= "srvfs",
	.mount		= srvfs_mount,
	.kill_sb	= kill_litter_super,
};

static int __init srvfs_init(void)
{
	pr_info("srvfs: loaded\n");
	return register_filesystem(&srvfs_type);
}

static void __exit srvfs_exit(void)
{
	unregister_filesystem(&srvfs_type);
	pr_info("srvfs: unloaded\n");
}

module_init(srvfs_init);
module_exit(srvfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mtx");
