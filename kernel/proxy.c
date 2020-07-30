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

#define PROXY_NO_BACKEND \
	pr_info("%s() no backend file handler\n", __FUNCTION__);

#define PROXY_RET(x) \
	PROXY_NO_BACKEND \
	return x;

#define PASS_TO_VFS(vfsop, args...) \
	{ \
		PROXY_INTRO \
		return vfsop(args); \
	}

#define PASS_TO_FILE(opname, defret, args...) \
{ \
	PROXY_INTRO \
	if (target->f_op->opname) \
		return target->f_op->opname(args); \
	PROXY_RET(defret); \
}

/* === file operations passed to VFS === */

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

static int proxy_setlease(struct file *proxy, long arg,
			  struct file_lock ** lease, void ** priv)
	PASS_TO_VFS(vfs_setlease, target, arg, lease, priv);

/* this *might* cause trouble w/ NFSd, which wants to retrieve 
   the conflicting lock */
static int proxy_lock (struct file *proxy, int cmd, struct file_lock *fl)
	PASS_TO_VFS(vfs_lock_file, target, cmd, fl, NULL);

/* === file operations passed directly to the backend file === */

static ssize_t proxy_dedupe_file_range(struct file *proxy, u64 loff, u64 olen,
				       struct file *dst_file, u64 dst_loff)
	PASS_TO_FILE(dedupe_file_range, -EINVAL, target, loff, olen, dst_file,
		     dst_loff);

static int proxy_flush (struct file *proxy, fl_owner_t id)
	PASS_TO_FILE(flush, 0, target, id);

static long proxy_compat_ioctl (struct file *proxy, unsigned int cmd,
				unsigned long arg)
	PASS_TO_FILE(compat_ioctl, -ENOIOCTLCMD, target, cmd, arg);

static int proxy_fasync (int fd, struct file *proxy, int on)
	PASS_TO_FILE(fasync, 0, fd, target, on);

static ssize_t proxy_sendpage (struct file *proxy, struct page *page, int offs,
			       size_t len, loff_t *pos, int more)
	PASS_TO_FILE(sendpage, -EINVAL, target, page, offs, len, pos, more);

static unsigned int proxy_poll (struct file *proxy,
				struct poll_table_struct *pt)
	// FIXME: should we return -EPERM instead ?
	PASS_TO_FILE(poll, 0, target, pt);

static ssize_t proxy_copy_file_range(struct file *proxy, loff_t pos_in,
	struct file *file_out, loff_t pos_out, size_t size, unsigned int flags)
	PASS_TO_FILE(copy_file_range, -EOPNOTSUPP, target, pos_in, file_out,
		     pos_out, size, flags);

static int proxy_clone_file_range(struct file *proxy, loff_t pos_in,
				  struct file *file_out, loff_t pos_out,
				  u64 len)
	PASS_TO_FILE(clone_file_range, -EOPNOTSUPP, target, pos_in, file_out,
		    pos_out, len);

static long proxy_fallocate(struct file *proxy, int mode, loff_t offset, loff_t len)
	PASS_TO_FILE(fallocate, -EOPNOTSUPP, target, mode, offset, len);

static int proxy_mmap (struct file *proxy, struct vm_area_struct *vma)
	PASS_TO_FILE(mmap, -ENODEV, target, vma);

#ifndef CONFIG_MMU
static unsigned proxy_mmap_capabilities(struct file *proxy)
	PASS_TO_FILE(mmap_capabilities, -EOPNOTSUPP, target);
#endif /* CONFIG_MMU */


/* file operations with special implementation */

static void proxy_show_fdinfo(struct seq_file *m, struct file *proxy)
{
	PROXY_INTRO
	if (target->f_op->show_fdinfo)
		return target->f_op->show_fdinfo(m, target);
	PROXY_NO_BACKEND
}

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

static unsigned long proxy_get_unmapped_area(struct file *proxy,
					     unsigned long orig_addr,
					     unsigned long len,
					     unsigned long pgoff,
					     unsigned long flags)
{
	PROXY_INTRO

	/* emulate what get_unmapped_area() does when f_op->get_unmapped_area
	   is NULL - call the current MM's get_unmapped_area() vector */
	if (target->f_op->get_unmapped_area)
		return target->f_op->get_unmapped_area(target, orig_addr,
						       len, pgoff, flags);
	else
		return current->mm->get_unmapped_area(target, orig_addr,
						      len, pgoff, flags);
}

static int proxy_flock (struct file *proxy, int flags, struct file_lock *fl)
{
	PROXY_INTRO

	if (target->f_op->flock)
		return target->f_op->flock(target, flags, fl);

	/* I'm not completely sure, whether this fallback is really correct */
	return locks_lock_file_wait(target, fl);
}


// FIXME
static ssize_t proxy_splice_read(struct file *proxy, loff_t *off, struct pipe_inode_info *info, size_t size, unsigned int flags)
{
	PROXY_INTRO

	if (target->f_op->splice_read)
		return target->f_op->splice_read(target, off, info, size, flags);

	PROXY_RET(-EOPNOTSUPP);
}

// yet unimplemented .. do we need them at all ?

//FIXME
/* not implemented yet
static int proxy_check_flags(int flags)
{
	PROXY_INTRO
	if (target->f_op->check_flags)
		return target->f_op->check_flags(target, flags);
	PROXY_RET(-EOPNOTSUPP);
}
*/

//static ssize_t proxy_read_iter (struct kiocb *, struct iov_iter *)
//static ssize_t proxy_write_iter (struct kiocb *, struct iov_iter *)
//int (*iterate) (struct file *, struct dir_context *);
//int (*iterate_shared) (struct file *, struct dir_context *);


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

#define STR(s) #s

#define COPY_FILEOP(opname) \
	if (file->f_op->opname) { \
		pr_info("assigning file operation " STR(opname) "\n"); \
		fileref->f_ops.opname = proxy_##opname; \
	} else { \
		pr_info("skipping operation " STR(opname) "\n"); \
		fileref->f_ops.opname = NULL; \
	}

#define SET_FILEOP(opname) \
	pr_info("auto assigning file operation " STR(opname) "\n"); \
	fileref->f_ops.opname = proxy_##opname;

void srvfs_proxy_fill_fops(struct file *file)
{
	struct srvfs_fileref *fileref = file->private_data;

	fileref->f_ops.owner = THIS_MODULE;
	memset(&fileref->f_ops, 0, sizeof(fileref->f_ops));

	SET_FILEOP(open);
	SET_FILEOP(release);

	COPY_FILEOP(llseek);
	COPY_FILEOP(read);
	COPY_FILEOP(write);
	COPY_FILEOP(fsync);
	COPY_FILEOP(fasync);
	COPY_FILEOP(poll);
	COPY_FILEOP(unlocked_ioctl);
	COPY_FILEOP(compat_ioctl);
	COPY_FILEOP(mmap);
	COPY_FILEOP(flush);
	COPY_FILEOP(lock);
	COPY_FILEOP(sendpage);
	COPY_FILEOP(get_unmapped_area);
	COPY_FILEOP(flock);
	COPY_FILEOP(splice_write);
	COPY_FILEOP(splice_read);
	COPY_FILEOP(setlease);
	COPY_FILEOP(fallocate);
	COPY_FILEOP(show_fdinfo);
	COPY_FILEOP(copy_file_range);
	COPY_FILEOP(clone_file_range);
	COPY_FILEOP(dedupe_file_range);
#ifndef CONFIG_MMU
	COPY_FILEOP(mmap_capabilities);
#endif
// cant support them yet :(
//	.read_iter = proxy_read_iter,
//	.write_iter = proxy_write_iter,
//	.iterate = proxy_iterate,
//	.iterate_shared = proxy_iterate_shared,
//	.check_flags = proxy_check_flags,

	file->f_op = &fileref->f_ops;
}
