#ifndef __LINUX_FS_SRVFS_H
#define __LINUX_FS_SRVFS_H

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/fs.h>
#include <linux/kref.h>
#include <asm/atomic.h>

#define SRVFS_MAGIC 0x29980123

#define CONFIG_SRVFS_VFS_READWRITE

struct srvfs_fileref {
	atomic_t counter;
	int mode;
	struct file *file;
	struct kref refcount;
	struct file_operations f_ops;
};

struct srvfs_sb {
	atomic_t inode_counter;
};

extern struct file_operations srvfs_file_ops;
extern const struct inode_operations srvfs_rootdir_inode_operations;
extern const struct file_operations proxy_file_ops;

struct srvfs_fileref *srvfs_fileref_new(void);
struct srvfs_fileref *srvfs_fileref_get(struct srvfs_fileref* fileref);
void srvfs_fileref_put(struct srvfs_fileref* fileref);
void srvfs_fileref_set(struct srvfs_fileref* fileref, struct file* newfile);

int srvfs_fill_super (struct super_block *sb, void *data, int silent);
int srvfs_inode_id (struct super_block *sb);
int srvfs_insert_file (struct super_block *sb, struct dentry *dentry);

void srvfs_proxy_fill_fops(struct file *file);

#endif /* __LINUX_FS_SRVFS_H */
