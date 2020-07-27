#include "srvfs.h"

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/file.h>
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

static void srvfs_sb_evict_inode(struct inode *inode)
{
	struct srvfs_fileref *fileref = inode->i_private;

	pr_info("srvfs_evict_inode(): %ld\n", inode->i_ino);
	clear_inode(inode);
	if (fileref)
		srvfs_fileref_put(fileref);
	else
		pr_info("evicting root/dir inode\n");
}

static void srvfs_sb_put_super(struct super_block *sb)
{
	pr_info("srvfs: freeing superblock");
	if (sb->s_fs_info) {
		kfree(sb->s_fs_info);
		sb->s_fs_info = NULL;
	}
}

static const struct super_operations srvfs_super_operations = {
	.statfs		= simple_statfs,
	.evict_inode	= srvfs_sb_evict_inode,
	.put_super	= srvfs_sb_put_super,
};

int srvfs_inode_id (struct super_block *sb)
{
	struct srvfs_sb *priv = sb->s_fs_info;
	return atomic_inc_return(&priv->inode_counter);
}

static int srvfs_create_file (struct super_block *sb, struct dentry *root, const char* name)
{
	struct dentry *dentry;

	dentry = d_alloc_name(root, name);
	if (!dentry)
		return -ENOMEM;

	return srvfs_insert_file(sb, dentry);
}

int srvfs_fill_super (struct super_block *sb, void *data, int silent)
{
	struct inode *inode;
	struct dentry *root;
	int i;
	struct srvfs_sb* sbpriv;
	int ret = 0;

	sbpriv = kmalloc(sizeof(struct srvfs_sb), GFP_KERNEL);
	if (sbpriv == NULL)
		goto err_sbpriv;

	atomic_set(&sbpriv->inode_counter, 1);

	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_magic = SRVFS_MAGIC;
	sb->s_op = &srvfs_super_operations;
	sb->s_time_gran = 1;
	sb->s_fs_info = sbpriv;

	inode = new_inode(sb);
	if (!inode)
		goto err_inode;

	/*
	 * because the root inode is 1, the files array must not contain an
	 * entry at index 1
	 */
	inode->i_ino = srvfs_inode_id(sb);
	inode->i_mode = S_IFDIR | 0755;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_op = &srvfs_rootdir_inode_operations;
	inode->i_fop = &simple_dir_operations;
	set_nlink(inode, 2);
	root = d_make_root(inode);
	if (!root) {
		pr_info("fill_super(): could not create root\n");
		goto err_root;
	}
	sb->s_root = root;

	for (i = 0; i<ARRAY_SIZE(names); i++) {
		ret = srvfs_create_file(sb, root, names[i]);
		if (ret) {
			pr_err("srvfs_create_file() returned: %d\n", ret);
			goto out;
		}
	}
	return 0;
out:
	d_genocide(root);
	shrink_dcache_parent(root);
	dput(root);
	goto err_inode;

err_root:
	iput(inode);

err_inode:
	kfree(sbpriv);

err_sbpriv:
	return -ENOMEM;
}
