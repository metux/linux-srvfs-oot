#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "srvfs.h"

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

static int srvfs_file_open(struct inode *inode, struct file *file)
{
	struct srvfs_inode *priv = inode->i_private;
	pr_info("open inode_id=%ld\n", inode->i_ino);
	file->private_data = inode->i_private;

	if (priv->file) {
		pr_info("open inode: already assigned another file\n");
		file->f_op = &proxy_file_ops;
	}
	else {
		pr_info("open inode: no file assigned yet\n");
	}

	return 0;
}

static int srvfs_file_release(struct inode *inode, struct file *filp)
{
	pr_info("closing inode_id=%ld\n", inode->i_ino);
	return 0;
}

#define TMPSIZE 20
/*
 * Read a file.  Here we increment and read the counter, then pass it
 * back to the caller.  The increment only happens if the read is done
 * at the beginning of the file (offset = 0); otherwise we end up counting
 * by twos.
 */
static ssize_t srvfs_file_read(struct file *filp, char *buf,
	size_t count, loff_t *offset)
{
	int v, len;
	char tmp[TMPSIZE];
	struct srvfs_inode *priv = filp->private_data;

	/*
	 * Encode the value, and figure out how much of it we can pass back.
	 */
	v = atomic_read(&priv->counter);
	if (*offset > 0)
		v -= 1;  /* the value returned when offset was zero */
	else
		atomic_inc(&priv->counter);
	len = snprintf(tmp, sizeof(tmp), "%d\n", v);
	if (*offset > len)
		return 0;
	if (count > len - *offset)
		count = len - *offset;
	/*
	 * Copy it back, increment the offset, and we're done.
	 */
	if (copy_to_user(buf, tmp + *offset, count))
		return -EFAULT;
	*offset += count;
	return count;
}

static int do_switch(struct file *filp, long fd)
{
	struct srvfs_inode *priv = filp->private_data;
	pr_info("doing the switch: fd=%ld\n", fd);
//	d_delete(priv->dentry);
//	dput(priv->dentry);
//	priv->dentry = NULL;

	if (priv->file) {
		pr_info("freeing existing file\n");
		fput(priv->file);
	}

	priv->file = fget(fd);
	if (!priv->file) {
		pr_err("not an valid fd\n");
		return -EINVAL;
	}

	if (priv->file->f_inode == filp->f_inode) {
		pr_err("whoops. trying to link inode with itself!\n");
		fput(priv->file);
		return -ELOOP;
	}

	pr_info("got valid fd. storing it\n");
	return 0;
}

static ssize_t srvfs_file_write(struct file *filp, const char *buf,
				size_t count, loff_t *offset)
{
	char tmp[TMPSIZE];
	long fd;
	struct srvfs_inode *priv = filp->private_data;
	int ret;

	if (*offset != 0)
		return -EINVAL;

	if (count >= TMPSIZE)
		return -EINVAL;
	memset(tmp, 0, TMPSIZE);
	if (copy_from_user(tmp, buf, count))
		return -EFAULT;

	fd = simple_strtol(tmp, NULL, 10);
	pr_info("requested to assign fd %ld\n", fd);
	ret = do_switch(filp, fd);

	atomic_set(&priv->counter, simple_strtol(tmp, NULL, 10));

	if (ret)
		return ret;

	return count;
}

struct file_operations srvfs_file_ops = {
	.open		= srvfs_file_open,
	.read		= srvfs_file_read,
	.write		= srvfs_file_write,
	.release	= srvfs_file_release,
};

int srvfs_insert_file (struct super_block *sb, struct dentry *dentry)
{
	struct inode *inode;
	struct srvfs_inode *priv;
	int mode = S_IFREG | S_IWUSR | S_IRUGO;

	priv = kzalloc(sizeof(struct srvfs_inode), GFP_KERNEL);
	if (!priv) {
		pr_err("srvfs_insert_file(): failed to malloc inode priv\n");
		return -ENOMEM;
	}

	inode = new_inode(sb);
	if (!inode) {
		pr_err("srvfs_insert_file(): failed to allocate inode\n");
		goto err;
	}

	atomic_set(&priv->counter, 0);
	priv->mode = 0;
	priv->dentry = dentry;

	inode_init_owner(inode, sb->s_root->d_inode, mode);

	// FIXME: still needed ?
	inode->i_mode = mode;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_fop = &srvfs_file_ops;
	inode->i_ino = srvfs_inode_id(inode->i_sb);
	inode->i_private = priv;

	pr_info("new inode id: %ld\n", inode->i_ino);

	d_drop(dentry);
	d_add(dentry, inode);
	return 0;

err:
	kfree(priv);
	return -ENOMEM;
}
