#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "srvfs.h"

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

static int srvfs_file_open(struct inode *inode, struct file *filp)
{
	pr_info("open\n");
	filp->private_data = inode->i_private;
	return 0;
}

static int srvfs_file_release(struct inode *inode, struct file *filp)
{
	pr_info("closing\n");
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

static void do_switch(struct file *filp)
{
	struct srvfs_inode *priv = filp->private_data;
	pr_info("doing the switch\n");
	d_delete(priv->dentry);
	dput(priv->dentry);
	priv->dentry = NULL;
}

static ssize_t srvfs_file_write(struct file *filp, const char *buf,
				size_t count, loff_t *offset)
{
	char tmp[TMPSIZE];
	long fd_id;
	struct srvfs_inode *priv = filp->private_data;

	if (*offset != 0)
		return -EINVAL;

	if (count >= TMPSIZE)
		return -EINVAL;
	memset(tmp, 0, TMPSIZE);
	if (copy_from_user(tmp, buf, count))
		return -EFAULT;

	fd_id = simple_strtol(tmp, NULL, 10);
	pr_info("requested to assign fd %ld\n", fd_id);
	if (fd_id == 666)
		do_switch(filp);

	atomic_set(&priv->counter, simple_strtol(tmp, NULL, 10));
	return count;
}

struct file_operations srvfs_file_ops = {
	.open		= srvfs_file_open,
	.read		= srvfs_file_read,
	.write		= srvfs_file_write,
	.release	= srvfs_file_release,
};