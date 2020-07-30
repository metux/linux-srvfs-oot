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
	struct srvfs_fileref *fileref = proxy->private_data; \
	struct file *target = fileref->file; \
	(void)(target); \
	pr_info("%s()\n", __FUNCTION__);

#define PROXY_NOTSUP \
	pr_info("%s() no backend file handler\n", __FUNCTION__);

#define PROXY_INVAL_RET \
	PROXY_NOTSUP \
	return -EINVAL;

#define PROXY_NOTSUP_RET \
	PROXY_NOTSUP \
	return -ENOTSUPP;

#define PROXY_RET(x) \
	PROXY_NOTSUP \
	return x;

#define PASS_TO_VFS(vfsop, args...) \
	{ \
		PROXY_INTRO \
		return vfsop(args); \
	}

/* file operations passed to VFS */

static loff_t proxy_llseek (struct file *proxy, loff_t offset, int whence)
	PASS_TO_VFS(vfs_llseek, target, offset, whence);

static ssize_t proxy_read (struct file *proxy, char __user *buf, size_t len, loff_t *offset)
	PASS_TO_VFS(vfs_read, target, buf, len, offset);

static ssize_t proxy_write (struct file *proxy, const char __user *buf, size_t len, loff_t *offset)
	PASS_TO_VFS(vfs_write, target, buf, len, offset);

static long proxy_unlocked_ioctl (struct file *proxy, unsigned int cmd, unsigned long arg)
	PASS_TO_VFS(vfs_ioctl, target, cmd, arg);

static int proxy_fsync (struct file *proxy, loff_t start, loff_t end, int datasync)
	PASS_TO_VFS(vfs_fsync_range, target, start, end, datasync);

static ssize_t proxy_splice_write(struct pipe_inode_info *pipe,
				  struct file *proxy, loff_t *ppos,
				  size_t len, unsigned int flags)
	PASS_TO_VFS(do_splice_from, pipe, target, ppos, len, flags);

/* file operations passed directly to the backend file */

static int proxy_lock (struct file *proxy, int flags, struct file_lock *lock)
{
	PROXY_INTRO
	if (target->f_op->lock)
		return target->f_op->lock(target, flags, lock);
	PROXY_NOTSUP_RET
}

// FIXME
static int proxy_flock (struct file *proxy, int flags, struct file_lock *lock)
{
	PROXY_INTRO
	if (target->f_op->flock)
		return target->f_op->flock(target, flags, lock);
	PROXY_NOTSUP_RET
}

// FIXME
static unsigned long proxy_get_unmapped_area(struct file *proxy, unsigned long a, unsigned long b, unsigned long c, unsigned long d)
{
	PROXY_INTRO
	if (target->f_op->get_unmapped_area)
		return target->f_op->get_unmapped_area(target, a, b, c, d);
	PROXY_RET(-ENOTSUPP);
}

static ssize_t proxy_dedupe_file_range(struct file *proxy, u64 loff, u64 olen,
				       struct file *dst_file, u64 dst_loff)
{
	PROXY_INTRO
	if (target->f_op->dedupe_file_range)
		return target->f_op->dedupe_file_range(target, loff, olen,
						       dst_file, dst_loff);
	PROXY_RET(-EINVAL);
}

static int proxy_flush (struct file *proxy, fl_owner_t id)
{
	PROXY_INTRO
	if (target->f_op->flush)
		return target->f_op->flush(target, id);
	PROXY_RET(0);
}

static long proxy_compat_ioctl (struct file *proxy, unsigned int a, unsigned long b)
{
	PROXY_INTRO
	if (target->f_op->compat_ioctl)
		return target->f_op->compat_ioctl(target, a, b);
	PROXY_RET(-ENOIOCTLCMD);
}

static int proxy_fasync (int x, struct file *proxy, int y)
{
	PROXY_INTRO
	if (target->f_op->fasync)
		return target->f_op->fasync(x, target, y);
	PROXY_RET(0)
}

static ssize_t proxy_sendpage (struct file *proxy, struct page *page, int x,
			       size_t size, loff_t *offset, int flags)
{
	PROXY_INTRO
	if (target->f_op->sendpage)
		return target->f_op->sendpage(target, page, x, size, offset, flags);
	PROXY_RET(-EINVAL);
}

/* file operations with special implementation */

static int proxy_open (struct inode *inode, struct file *proxy)
{
	PROXY_INTRO
	pr_info("%s() should never be called\n", __FUNCTION__);
	WARN_ON(1);
	PROXY_RET(-EINVAL);
}

static int proxy_release (struct inode *inode, struct file *proxy)
{
	PROXY_INTRO
	(void)(inode);
	pr_info("closing proxy inode_id=%ld\n", inode->i_ino);
	srvfs_fileref_put(fileref);
	return 0;
}

static unsigned int proxy_poll (struct file *proxy, struct poll_table_struct *poll)
{
	PROXY_INTRO
	if (target->f_op->poll)
		return target->f_op->poll(target, poll);
	// FIXME: should we return -EPERM instead ?
	PROXY_RET(0)
}

// yet unimplemented .. do we need them at all ?

//static ssize_t proxy_read_iter (struct kiocb *, struct iov_iter *)
//static ssize_t proxy_write_iter (struct kiocb *, struct iov_iter *)
//int (*iterate) (struct file *, struct dir_context *);
//int (*iterate_shared) (struct file *, struct dir_context *);

//FIXME
static int proxy_mmap (struct file *proxy, struct vm_area_struct *vma)
{
	PROXY_INTRO
	if (target->f_op->mmap)
		return target->f_op->mmap(target, vma);
	PROXY_NOTSUP_RET
}

/* not implemented yet
static int proxy_check_flags(int flags)
{
	PROXY_INTRO
	if (target->f_op->check_flags)
		return target->f_op->check_flags(target, flags);
	PROXY_NOTSUP_RET
}
*/

// FIXME
static ssize_t proxy_splice_read(struct file *proxy, loff_t *off, struct pipe_inode_info *info, size_t size, unsigned int flags)
{
	PROXY_INTRO
	if (target->f_op->splice_read)
		return target->f_op->splice_read(target, off, info, size, flags);
	PROXY_NOTSUP_RET
}

// FIXME
static int proxy_setlease(struct file *proxy, long a, struct file_lock ** lock, void ** b)
{
	PROXY_INTRO
	if (target->f_op->setlease)
		return target->f_op->setlease(target, a, lock, b);
	PROXY_NOTSUP_RET
}

// FIXME
static long proxy_fallocate(struct file *proxy, int mode, loff_t offset, loff_t len)
{
	PROXY_INTRO
	if (target->f_op->fallocate)
		return target->f_op->fallocate(target, mode, offset, len);
	PROXY_NOTSUP_RET
}

// FIXME
static void proxy_show_fdinfo(struct seq_file *m, struct file *proxy)
{
	PROXY_INTRO
	if (target->f_op->show_fdinfo)
		return target->f_op->show_fdinfo(m, target);
	PROXY_NOTSUP
}

#ifndef CONFIG_MMU
static unsigned proxy_mmap_capabilities(struct file *proxy)
{
	PROXY_INTRO
	if (target->f_op->mmap_capabilities)
		return target->f_op->mmap_capabilities(target);
	PROXY_NOTSUP_RET
}
#endif /* CONFIG_MMU */

// FIXME
static ssize_t proxy_copy_file_range(struct file *proxy, loff_t off1, struct file *file2,
	loff_t off2, size_t size, unsigned int flags)
{
	PROXY_INTRO
	if (target->f_op->copy_file_range)
		return target->f_op->copy_file_range(target, off1, file2, off2, size, flags);
	PROXY_NOTSUP_RET
}

// FIXME
static int proxy_clone_file_range(struct file *proxy, loff_t off1, struct file *file2, loff_t off2, u64 flags)
{
	PROXY_INTRO
	if (target->f_op->clone_file_range)
		return target->f_op->clone_file_range(target, off1, file2, off2, flags);
	PROXY_NOTSUP_RET
}

// FIXME

const struct file_operations proxy_file_ops = {
	.owner = THIS_MODULE,
	.llseek = proxy_llseek,
	.open = proxy_open,
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
//	.check_flags = proxy_check_flags,
	.flock = proxy_flock,
	.splice_write = proxy_splice_write,
	.splice_read = proxy_splice_read,
	.setlease = proxy_setlease,
	.fallocate = proxy_fallocate,
	.show_fdinfo = proxy_show_fdinfo,
#ifndef CONFIG_MMU
 	.mmap_capabilities = proxy_mmap_capabilities,
#endif
	.copy_file_range = proxy_copy_file_range,
	.clone_file_range = proxy_clone_file_range,
	.dedupe_file_range = proxy_dedupe_file_range,
};
