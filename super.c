#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "srvfs.h"

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

static const char *names[] = {
	"counter0",
	"counter1",
	"counter2",
	"counter3",
};

static void srvfs_evict_inode(struct inode *inode)
{
	pr_info("srvfs_evict_inode()\n");
	clear_inode(inode);
	if (inode->i_private)
		kfree(inode->i_private);
}

static const struct super_operations srvfs_super_operations = {
	.statfs		= simple_statfs,
	.evict_inode	= srvfs_evict_inode,
};

int srvfs_fill_super (struct super_block *sb, void *data, int silent)
{
	struct inode *inode;
	struct dentry *root;
	int i;

	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_magic = SRVFS_MAGIC;
	sb->s_op = &srvfs_super_operations;
	sb->s_time_gran = 1;

	inode = new_inode(sb);
	if (!inode)
		return -ENOMEM;
	/*
	 * because the root inode is 1, the files array must not contain an
	 * entry at index 1
	 */
	inode->i_ino = 1;
	inode->i_mode = S_IFDIR | 0755;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_op = &simple_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;
	set_nlink(inode, 2);
	root = d_make_root(inode);
	if (!root) {
		pr_info("fill_super(): could not create root\n");
		return -ENOMEM;
	}

	for (i = 0; i<ARRAY_SIZE(names); i++) {
		if (!srvfs_create_file(sb, root, names[i], i+1))
			goto out;
	}
	sb->s_root = root;
	return 0;
out:
	d_genocide(root);
	shrink_dcache_parent(root);
	dput(root);
	return -ENOMEM;
}
