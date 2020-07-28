#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "srvfs.h"

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

static int srvfs_dir_unlink(struct inode *inode, struct dentry *dentry)
{
	struct srvfs_inode *priv = dentry->d_inode->i_private;
	pr_info("srvfs: unlink\n");

	if (priv == NULL) {
		pr_err("srvfs unlink: dentry's inode has no priv\n");
		return -EFAULT;
	}

	if (priv->dentry != dentry) {
		pr_err("srvfs unlink: dentry's dont match\n");
		return -EFAULT;
	}

	priv->dentry = NULL;
	d_delete(dentry);
	dput(dentry);

	pr_info("srvfs unlink: sucessfully unlinked\n");

	return 0;
}

static int srvfs_dir_create (struct inode *inode, struct dentry *dentry, umode_t mode, bool excl)
{
	pr_info("srvfs_dir_create(): trying to create dir entry\n");
	if (excl)
		pr_info("srvfs_dir_create() exclusive\n");
	else
		pr_info("srvfs_dir_create() not exclusive\n");

	return srvfs_insert_file(inode->i_sb, dentry);
}

const struct inode_operations srvfs_rootdir_inode_operations = {
	.lookup		= simple_lookup,
	.unlink		= srvfs_dir_unlink,
	.create		= srvfs_dir_create,
};

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
