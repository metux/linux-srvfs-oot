#ifndef __LINUX_FS_SRVFS_H
#define __LINUX_FS_SRVFS_H

#include <linux/fs.h>
#include <asm/atomic.h>

#define SRVFS_MAGIC 0x19980123

struct srvfs_inode {
	atomic_t counter;
	int mode;
	struct dentry *dentry;
	struct file *file;
};

struct srvfs_sb {
	atomic_t inode_counter;
};

extern struct file_operations srvfs_file_ops;
extern const struct inode_operations srvfs_rootdir_inode_operations;
extern const struct file_operations proxy_file_ops;

int srvfs_fill_super (struct super_block *sb, void *data, int silent);
int srvfs_inode_id (struct super_block *sb);
int srvfs_insert_file (struct super_block *sb, struct dentry *dentry);

#endif /* __LINUX_FS_SRVFS_H */
