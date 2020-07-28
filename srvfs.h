#ifndef __LINUX_FS_SRVFS_H
#define __LINUX_FS_SRVFS_H

#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

#define SRVFS_MAGIC 0x19980123

struct srvfs_inode {
	atomic_t counter;
	int mode;
	struct dentry *dentry;
};

extern struct file_operations srvfs_file_ops;

int srvfs_fill_super (struct super_block *sb, void *data, int silent);
int srvfs_create_file (struct super_block *sb, struct dentry *root, const char* name, int idx);

#endif /* __LINUX_FS_SRVFS_H */