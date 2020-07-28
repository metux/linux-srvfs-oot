#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "srvfs.h"

#include <linux/fs.h>
#include <linux/slab.h>

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
	return srvfs_insert_file(inode->i_sb, dget(dentry));
}

const struct inode_operations srvfs_rootdir_inode_operations = {
	.lookup		= simple_lookup,
	.unlink		= srvfs_dir_unlink,
	.create		= srvfs_dir_create,
};
