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

static loff_t proxy_llseek(struct file *proxy, loff_t offset, int whence)
	PASS_TO_FILE(llseek, -EOPNOTSUPP, target, offset, whence);

static ssize_t proxy_read(struct file *proxy, char __user *buf, size_t len,
			  loff_t *offset)
	PASS_TO_FILE(read, -EOPNOTSUPP, target, buf, len, offset);

static ssize_t proxy_write(struct file *proxy, const char __user *buf,
			   size_t len, loff_t *offset)
	PASS_TO_FILE(write, -EOPNOTSUPP, target, buf, len, offset);

static long proxy_unlocked_ioctl(struct file *proxy, unsigned int cmd,
				 unsigned long arg)
	PASS_TO_FILE(unlocked_ioctl, -EOPNOTSUPP, target, cmd, arg);

static int proxy_fsync(struct file *proxy, loff_t start, loff_t end,
		       int datasync)
	PASS_TO_FILE(fsync, -EOPNOTSUPP, target, start, end, datasync);

static ssize_t proxy_splice_write(struct pipe_inode_info *pipe,
				  struct file *proxy, loff_t *ppos,
				  size_t len, unsigned int flags)
	PASS_TO_FILE(splice_write, -EOPNOTSUPP, pipe, target, ppos, len, flags);

static int proxy_setlease(struct file *proxy, long arg,
			  struct file_lock ** lease, void ** priv)
	PASS_TO_FILE(setlease, -EOPNOTSUPP, target, arg, lease, priv);

/* this *might* cause trouble w/ NFSd, which wants to retrieve 
   the conflicting lock */
static int proxy_lock(struct file *proxy, int cmd, struct file_lock *fl)
	PASS_TO_FILE(lock, -EOPNOTSUPP, target, cmd, fl);

/* === file operations passed directly to the backend file === */

static ssize_t proxy_dedupe_file_range(struct file *proxy, u64 loff, u64 olen,
				       struct file *dst_file, u64 dst_loff)
	PASS_TO_FILE(dedupe_file_range, -EINVAL, target, loff, olen, dst_file,
		     dst_loff);

static int proxy_flush(struct file *proxy, fl_owner_t id)
	PASS_TO_FILE(flush, 0, target, id);

static long proxy_compat_ioctl(struct file *proxy, unsigned int cmd,
			       unsigned long arg)
	PASS_TO_FILE(compat_ioctl, -ENOIOCTLCMD, target, cmd, arg);

static int proxy_fasync(int fd, struct file *proxy, int on)
	PASS_TO_FILE(fasync, 0, fd, target, on);

static ssize_t proxy_sendpage(struct file *proxy, struct page *page, int offs,
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

static int proxy_mmap(struct file *proxy, struct vm_area_struct *vma)
	PASS_TO_FILE(mmap, -ENODEV, target, vma);

#ifndef CONFIG_MMU
static unsigned proxy_mmap_capabilities(struct file *proxy)
	PASS_TO_FILE(mmap_capabilities, -EOPNOTSUPP, target);
#endif /* CONFIG_MMU */

static int proxy_flock(struct file *proxy, int flags, struct file_lock *fl)
	PASS_TO_FILE(flock, -EOPNOTSUPP, target, flags, fl);

static unsigned long proxy_get_unmapped_area(struct file *proxy,
					     unsigned long orig_addr,
					     unsigned long len,
					     unsigned long pgoff,
					     unsigned long flags)
	PASS_TO_FILE(get_unmapped_area, -EOPNOTSUPP, target, orig_addr, len, pgoff, flags);

static ssize_t proxy_splice_read(struct file *proxy, loff_t *off, struct pipe_inode_info *info, size_t size, unsigned int flags)
	PASS_TO_FILE(splice_read, -EOPNOTSUPP, target, off, info, size, flags);

/* file operations with special implementation */

static void proxy_show_fdinfo(struct seq_file *m, struct file *proxy)
{
	PROXY_INTRO
	if (target->f_op->show_fdinfo)
		return target->f_op->show_fdinfo(m, target);
	PROXY_NO_BACKEND
}

static int proxy_open(struct inode *inode, struct file *proxy)
{
	PROXY_INTRO
	pr_info("%s() should never be called\n", __FUNCTION__);
	WARN_ON(1);
	PROXY_RET(-EINVAL);
}

static int proxy_release(struct inode *inode, struct file *proxy)
{
	PROXY_INTRO
	(void)(inode);
	pr_info("closing proxy inode_id=%ld\n", inode->i_ino);
	srvfs_fileref_put(fileref);
	return 0;
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
