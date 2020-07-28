#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "srvfs.h"

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

int srvfs_create_file (struct super_block *sb, struct dentry *root, const char* name, int idx)
{
	struct dentry *dentry;
	struct inode *inode;
	struct srvfs_inode *priv;

	priv = kmalloc(sizeof(struct srvfs_inode), GFP_KERNEL);
	if (!priv) {
		pr_err("srvfs_create_file(): failed to malloc inode priv\n");
		goto err;
	}

	atomic_set(&priv->counter, 0);
	priv->mode = 0;

	dentry = d_alloc_name(root, name);
	if (!dentry)
		goto err;

	priv->dentry = dentry;

	inode = new_inode(sb);
	if (!inode) {
		dput(dentry);
		goto err;
	}
	inode->i_mode = S_IFREG | S_IWUSR | S_IRUGO;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_fop = &srvfs_file_ops;
	inode->i_ino = idx;
	inode->i_private = priv;

	d_add(dentry, inode);
	return 1;
// FIXME: release resources
err:
	return 0;
}

static int srvfs_dir_unlink(struct inode *inode, struct dentry *dentry)
{
	struct srvfs_inode *priv = inode->i_private;
	pr_info("srvfs: unlink:\n");

	if (priv->dentry == dentry) {
		pr_info("srvfs: unlink: dentries match\n");
	}
	else{
		pr_info("srvfs: dentries dont match\n");
	}
	return -EPERM;
}

const struct inode_operations simple_dir_inode_operations = {
	.lookup		= simple_lookup,
	.unlink		= srvfs_dir_unlink,
};

/*
 * Stuff to pass in when registering the filesystem.
 */
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
	return register_filesystem(&srvfs_type);
}

static void __exit srvfs_exit(void)
{
	unregister_filesystem(&srvfs_type);
}

module_init(srvfs_init);
module_exit(srvfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mtx");
