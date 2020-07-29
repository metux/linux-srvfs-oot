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


#define PROXY_INTRO \
	struct srvfs_inode *priv = file->private_data; \
	struct file *target = priv->file; \
	pr_info("%s()\n", __FUNCTION__);

static loff_t proxy_llseek (struct file *file, loff_t off, int mode)
{
	PROXY_INTRO
	if (target->f_op->llseek)
		return target->f_op->llseek(target, off, mode);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static ssize_t proxy_read (struct file *file, char __user *buf , size_t len, loff_t * off)
{
	PROXY_INTRO
	if (target->f_op->read)
		return target->f_op->read(target, buf, len, off);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static ssize_t proxy_write (struct file *file, const char __user * buf, size_t len, loff_t * off)
{
	PROXY_INTRO
	if (target->f_op->write)
		return target->f_op->write(target, buf, len, off);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

//static ssize_t proxy_read_iter (struct kiocb *, struct iov_iter *)
//static ssize_t proxy_write_iter (struct kiocb *, struct iov_iter *)
//int (*iterate) (struct file *, struct dir_context *);
//int (*iterate_shared) (struct file *, struct dir_context *);

unsigned int proxy_poll (struct file *file, struct poll_table_struct *poll)
{
	PROXY_INTRO
	if (target->f_op->poll)
		return target->f_op->poll(target, poll);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

long proxy_unlocked_ioctl (struct file *file, unsigned int a, unsigned longb)
{
	PROXY_INTRO
	if (target->f_op->unlocked_ioctl)
		return target->f_op->unlocked_ioctl(target, a, b);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

long proxy_compat_ioctl (struct file *file, unsigned int a, unsigned long b)
{
	PROXY_INTRO
	if (target->f_op->compat_ioctl)
		return target->f_op->compat_ioctl(target, a, b);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;

}

int proxy_mmap (struct file *file, struct vm_area_struct *vma)
{
	PROXY_INTRO
	if (target->f_op->mmap)
		return target->f_op->mmap(target, vma);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static int proxy_open (struct inode *inode, struct file *file)
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
	PROXY_INTRO
	if (target->f_op->flush)
		return target->f_op->flush(target, id);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

/* NOTE: we're NOT passing this down to the target, this would break
   heavily as the file descriptor is still in use

   We probably should do some book keeping in order to prevent the
   proxy file from begin removed while its still open. or at least
   the target file and associated private data
*/
static int proxy_release (struct inode * inode, struct file * file)
{
	pr_info("%s() dummy\n", __FUNCTION__);
	return -ENOTSUPP;
}

static int proxy_fsync (struct file *file, loff_t off1, loff_t off2, int datasync)
{
	PROXY_INTRO
	if (target->f_op->fsync)
		return target->f_op->fsync(target, off1, off2, datasync);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static int proxy_fasync (int x, struct file *file , int y)
{
	PROXY_INTRO
	if (target->f_op->fasync)
		return target->f_op->fasync(x, target, y);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static int proxy_lock (struct file * file, int flags, struct file_lock *lock)
{
	PROXY_INTRO
	if (target->f_op->lock)
		return target->f_op->lock(target, flags, lock);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static int proxy_flock (struct file *file , int flags, struct file_lock *lock)
{
	PROXY_INTRO
	if (target->f_op->flock)
		return target->f_op->flock(target, flags, lock);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static ssize_t proxy_sendpage (struct file *file, struct page *page, int x, size_t size, loff_t *offset, int flags)
{
	PROXY_INTRO
	if (target->f_op->fasync)
		return target->f_op->fasync(target, page, x, size, offset, flags);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static unsigned long proxy_get_unmapped_area(struct file *file, unsigned long a, unsigned long b, unsigned long c, unsigned long d)
{
	PROXY_INTRO
	if (target->f_op->get_unmapped_area)
		return target->f_op->get_unmapped_area(target, a, b, c, d);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static int proxy_check_flags(int flags)
{
	PROXY_INTRO
	if (target->f_op->check_flags)
		return target->f_op->check_flags(target, flags);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static ssize_t proxy_splice_write(struct pipe_inode_info *info, struct file *file, loff_t *off, size_t size, unsigned int flags);
{
	PROXY_INTRO
	if (target->f_op->splice_write)
		return target->f_op->splice_write(info, target, off, size, flags);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static ssize_t proxy_splice_read(struct file *file, loff_t *off, struct pipe_inode_info *info, size_t size, unsigned int flags)
{
	PROXY_INTRO
	if (target->f_op->splice_read)
		return target->f_op->splice_read(target, off, info, size, flags);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static int proxy_setlease(struct file *file, long a, struct file_lock ** lock, void ** b)
{
	PROXY_INTRO
	if (target->f_op->splice_read)
		return target->f_op->splice_read(target, a, lock, b);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static long proxy_fallocate(struct file *file, int mode, loff_t offset, loff_t len)
{
	PROXY_INTRO
	if (target->f_op->fallocate)
		return target->f_op->splice_read(target, a, lock, b);
	pr_info("%s() operation not supported\n", __FUNCTION__);
	return -ENOTSUPP;
}

static void proxy_show_fdinfo(struct seq_file *m, struct file *file)
{
	PROXY_INTRO
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
	.poll = proxy_poll,
	.unlocked_ioctl = proxy_unlocked_ioctl,
	.compat_ioctl = proxy_compat_ioctl,
	.mmap = proxy_mmap,
	.flush = proxy_flush,
	.release = proxy_release,
	.fsync = proxy_fsync,
	.fasync = proxy_fasync,
	.lock = proxy_lock,
	.sendpage = proxy_sendpage,
	.get_unmapped_area = proxy_get_unmapped_area,
	.check_flags = proxy_check_flags,
	.flock = proxy_flock,
	.splice_write = proxy_splice_write,
	.splice_read = proxy_splice_read,
	.setlease = proxy_setlease,
	.fallocate = proxy_fallocate,
	.show_fdinfo = proxy_show_fdinfo,
	.mmap_capabilities = proxy_mmap_capabilities,
	.copy_file_range = proxy_copy_file_range,
	.clone_file_range = proxy_clone_file_range,
	.dedupe_file_range = proxy_dedupe_file_range,
};
