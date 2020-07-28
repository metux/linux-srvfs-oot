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


loff_t proxy_llseek (struct file *file, loff_t off, int mode)
{
	struct srvfs_inode *priv = file->private_data;
	struct file *target = priv->file;
	pr_info("%s()\n", __FUNCTION__);
	if (target->f_op->llseek)
		return target->f_op->llseek(target, off, mode);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

ssize_t proxy_read (struct file *file, char __user *buf , size_t len, loff_t * off)
{
	struct srvfs_inode *priv = file->private_data;
	struct file *target = priv->file;
	pr_info("%s()\n", __FUNCTION__);
	if (target->f_op->read)
		return target->f_op->read(target, buf, len, off);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

ssize_t proxy_write (struct file *file, const char __user * buf, size_t len, loff_t * off)
{
	struct srvfs_inode *priv = file->private_data;
	struct file *target = priv->file;
	pr_info("%s()\n", __FUNCTION__);
	if (target->f_op->write)
		return target->f_op->write(target, buf, len, off);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

//        ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
//        ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
//        int (*iterate) (struct file *, struct dir_context *);
//        int (*iterate_shared) (struct file *, struct dir_context *);
//        unsigned int (*poll) (struct file *, struct poll_table_struct *);
//        long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
//        long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
//        int (*mmap) (struct file *, struct vm_area_struct *);

int proxy_open (struct inode *inode, struct file *file)
{
//	struct srvfs_inode *priv = file->private_data;
//	struct file *target = priv->file;
	pr_info("%s() not implemented yet\n", __FUNCTION__);
//	if (target->f_op->write)
//		return target->f_op>read(target, buf, len, off);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

int proxy_flush (struct file * file, fl_owner_t id)
{
	struct srvfs_inode *priv = file->private_data;
	struct file *target = priv->file;
	pr_info("%s()\n", __FUNCTION__);
	if (target->f_op->flush)
		return target->f_op->flush(target, id);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

// FIXME: bookkeeping !
int proxy_release (struct inode * inode, struct file * file)
{
	struct srvfs_inode *priv = file->private_data;
	struct file *target = priv->file;
	pr_info("%s()\n", __FUNCTION__);
	if (target->f_op->release)
		return target->f_op->release(inode, target);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

int proxy_fsync (struct file *file, loff_t off1, loff_t off2, int datasync)
{
	struct srvfs_inode *priv = file->private_data;
	struct file *target = priv->file;
	pr_info("%s()\n", __FUNCTION__);
	if (target->f_op->fsync)
		return target->f_op->fsync(target, off1, off2, datasync);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

int proxy_fasync (int x, struct file *file , int y)
{
	struct srvfs_inode *priv = file->private_data;
	struct file *target = priv->file;
	pr_info("%s()\n", __FUNCTION__);
	if (target->f_op->fasync)
		return target->f_op->fasync(x, target, y);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

int proxy_lock (struct file * file, int flags, struct file_lock *lock)
{
	struct srvfs_inode *priv = file->private_data;
	struct file *target = priv->file;
	pr_info("%s()\n", __FUNCTION__);
	if (target->f_op->lock)
		return target->f_op->lock(target, flags, lock);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

int proxy_flock (struct file *file , int flags, struct file_lock *lock)
{
	struct srvfs_inode *priv = file->private_data;
	struct file *target = priv->file;
	pr_info("%s()\n", __FUNCTION__);
	if (target->f_op->flock)
		return target->f_op->flock(target, flags, lock);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

//        ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
//        unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
//        int (*check_flags)(int);
//        ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
//        ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
//        int (*setlease)(struct file *, long, struct file_lock **, void **);
//        long (*fallocate)(struct file *file, int mode, loff_t offset,
//                          loff_t len);

void proxy_show_fdinfo(struct seq_file *m, struct file *file)
{
	struct srvfs_inode *priv = file->private_data;
	struct file *target = priv->file;
	if (target->f_op->show_fdinfo)
		return target->f_op->show_fdinfo(m, target);
	pr_info("%s() operation not supported\n", __FUNCTION__);
}

//#ifndef CONFIG_MMU
//        unsigned (*mmap_capabilities)(struct file *);
//#endif
//        ssize_t (*copy_file_range)(struct file *, loff_t, struct file *,
//                        loff_t, size_t, unsigned int);
//        int (*clone_file_range)(struct file *, loff_t, struct file *, loff_t,
//                        u64);
//        ssize_t (*dedupe_file_range)(struct file *, u64, u64, struct file *,
//                        u64);

struct file_operations proxy_ops = {
	.owner = THIS_MODULE,
	.llseek = proxy_llseek,
	.read = proxy_read,
	.write = proxy_write,
//	.read_iter = proxy_read_iter,
//	.write_iter = proxy_write_iter,
//	.iterate = proxy_iterate,
//	.iterate_shared = proxy_iterate_shared,
//	.poll = proxy_poll,
//	.unlocked_ioctl = proxy_unlocked_ioctl,
//	.compat_ioctl = proxy_compat_ioctl,
//	.mmap = proxy_mmap,
	.flush = proxy_flush,
	.release = proxy_release,
	.fsync = proxy_fsync,
	.lock = proxy_lock,
//	.sendpage = proxy_sendpage,
//	.get_unmapped_area = proxy_get_unmapped_area,
//	.check_flags = proxy_check_flags,
	.flock = proxy_flock,
//	.splice_write = proxy_splice_write,
//	.splice_read = proxy_splice_read,
//	.setlease = proxy_setlease,
//	.fallocate = proxy_fallocate,
	.show_fdinfo = proxy_show_fdinfo,
//	.mmap_capabilities = proxy_mmap_capabilities,
//	.copy_file_range = proxy_copy_file_range,
//	.clone_file_range = proxy_clone_file_range,
//	.dedupe_file_range = proxy_dedupe_file_range,
};

static int srvfs_file_open(struct inode *inode, struct file *file)
{
	struct srvfs_inode *priv = inode->i_private;
	pr_info("open inode_id=%ld\n", inode->i_ino);
	file->private_data = inode->i_private;

	if (priv->file) {
		pr_info("open inode: already assigned another file\n");
		file->f_op = &proxy_ops;
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
