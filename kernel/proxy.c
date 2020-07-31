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

#define PROXY_NO_BACKEND \
	pr_info("%s() no backend file handler\n", __FUNCTION__); \
	BUG_ON(1)

#define PROXY_PASS_FILE(opname, args...) \
	PROXY_INTRO \
	if (target->f_op->opname) { \
		return target->f_op->opname(args); \
	} \
	PROXY_RET(-EOPNOTSUPP); \

#define PROXY_RET(x) \
	PROXY_NO_BACKEND \
	return x;

#define PASS_TO_VFS(vfsop, args...) \
	{ \
		PROXY_INTRO \
		return vfsop(args); \
	}

#define PASS_TO_FILE(opname, args...) \
{ \
	PROXY_PASS_FILE(opname, args) \
}

/* === file operations passed directly to the backend file === */

static loff_t proxy_llseek(struct file *proxy, loff_t offset, int whence)
	PASS_TO_FILE(llseek, target, offset, whence);

static ssize_t proxy_read(struct file *proxy, char __user *buf, size_t len,
			  loff_t *offset)
	PASS_TO_FILE(read, target, buf, len, offset);

static ssize_t proxy_write(struct file *proxy, const char __user *buf,
			   size_t len, loff_t *offset)
	PASS_TO_FILE(write, target, buf, len, offset);

#ifdef CONFIG_SRVFS_VFS_READWRITE
static ssize_t proxy_vfs_read(struct file *proxy, char __user *buf, size_t len,
			      loff_t *offset)
	PASS_TO_VFS(vfs_read, target, buf, len, offset);

static ssize_t proxy_vfs_write(struct file *proxy, const char __user *buf,
			       size_t len, loff_t *offset)
	PASS_TO_VFS(vfs_write, target, buf, len, offset);
#endif

static long proxy_unlocked_ioctl(struct file *proxy, unsigned int cmd,
				 unsigned long arg)
	PASS_TO_FILE(unlocked_ioctl, target, cmd, arg);

static int proxy_fsync(struct file *proxy, loff_t start, loff_t end,
		       int datasync)
	PASS_TO_FILE(fsync, target, start, end, datasync);

static ssize_t proxy_splice_write(struct pipe_inode_info *pipe,
				  struct file *proxy, loff_t *ppos,
				  size_t len, unsigned int flags)
	PASS_TO_FILE(splice_write, pipe, target, ppos, len, flags);

static int proxy_setlease(struct file *proxy, long arg,
			  struct file_lock ** lease, void ** priv)
	PASS_TO_FILE(setlease, target, arg, lease, priv);

/* this *might* cause trouble w/ NFSd, which wants to retrieve 
   the conflicting lock */
static int proxy_lock(struct file *proxy, int cmd, struct file_lock *fl)
	PASS_TO_FILE(lock, target, cmd, fl);

static ssize_t proxy_dedupe_file_range(struct file *proxy, u64 loff, u64 olen,
				       struct file *dst_file, u64 dst_loff)
	PASS_TO_FILE(dedupe_file_range, target, loff, olen, dst_file,
		     dst_loff);

static int proxy_flush(struct file *proxy, fl_owner_t id)
	PASS_TO_FILE(flush, target, id);

static long proxy_compat_ioctl(struct file *proxy, unsigned int cmd,
			       unsigned long arg)
	PASS_TO_FILE(compat_ioctl, target, cmd, arg);

static int proxy_fasync(int fd, struct file *proxy, int on)
	PASS_TO_FILE(fasync, fd, target, on);

static ssize_t proxy_sendpage(struct file *proxy, struct page *page, int offs,
			      size_t len, loff_t *pos, int more)
	PASS_TO_FILE(sendpage, target, page, offs, len, pos, more);

static unsigned int proxy_poll (struct file *proxy,
				struct poll_table_struct *pt)
	PASS_TO_FILE(poll, target, pt);

static ssize_t proxy_copy_file_range(struct file *proxy, loff_t pos_in,
	struct file *file_out, loff_t pos_out, size_t size, unsigned int flags)
	PASS_TO_FILE(copy_file_range, target, pos_in, file_out,
		     pos_out, size, flags);

static int proxy_clone_file_range(struct file *proxy, loff_t pos_in,
				  struct file *file_out, loff_t pos_out,
				  u64 len)
	PASS_TO_FILE(clone_file_range, target, pos_in, file_out,
		    pos_out, len);

static long proxy_fallocate(struct file *proxy, int mode, loff_t offset,
			    loff_t len)
	PASS_TO_FILE(fallocate, target, mode, offset, len);

static int proxy_mmap(struct file *proxy, struct vm_area_struct *vma)
	PASS_TO_FILE(mmap, target, vma);

#ifndef CONFIG_MMU
static unsigned proxy_mmap_capabilities(struct file *proxy)
	PASS_TO_FILE(mmap_capabilities, target);
#endif /* CONFIG_MMU */

static int proxy_flock(struct file *proxy, int flags, struct file_lock *fl)
	PASS_TO_FILE(flock, target, flags, fl);

static unsigned long proxy_get_unmapped_area(struct file *proxy,
					     unsigned long orig_addr,
					     unsigned long len,
					     unsigned long pgoff,
					     unsigned long flags)
	PASS_TO_FILE(get_unmapped_area, target, orig_addr, len, pgoff, flags);

static ssize_t proxy_splice_read(struct file *proxy, loff_t *off,
				 struct pipe_inode_info *info, size_t size,
				 unsigned int flags)
	PASS_TO_FILE(splice_read, target, off, info, size, flags);

static int proxy_iterate(struct file *proxy, struct dir_context *ctx)
	PASS_TO_FILE(iterate, target, ctx);

static int proxy_iterate_shared(struct file *proxy, struct dir_context *ctx)
	PASS_TO_FILE(iterate_shared, target, ctx);

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
	(void)(inode);
	(void)(proxy);
	WARN_ON(1);
	return -EINVAL;
}

static int proxy_release(struct inode *inode, struct file *proxy)
{
	PROXY_INTRO
	(void)(inode);
	srvfs_fileref_put(fileref);
	return 0;
}

static ssize_t proxy_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	ssize_t ret;
	struct kiocb target_iocb;
	struct file *proxy = iocb->ki_filp;
	PROXY_INTRO

	/* create a kiocb for the target file */
	init_sync_kiocb(&target_iocb, target);
	target_iocb.ki_pos = iocb->ki_pos;

	// FIXME: do we also need to tweak ki_flags ? */

	/* call the actual handler */
	ret = target->f_op->read_iter(&target_iocb, iter);

	/* write back to proxy iocb */
	iocb->ki_pos = target_iocb.ki_pos;

	return ret;
}

static ssize_t proxy_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	ssize_t ret;
	struct file *proxy = iocb->ki_filp;
	struct kiocb target_iocb;
	PROXY_INTRO

	/* create a kiocb for the target file */
	init_sync_kiocb(&target_iocb, target);
	target_iocb.ki_pos = iocb->ki_pos;

	// FIXME: do we also need to tweak ki_flags ? */

	/* call the actual handler */
	ret = target->f_op->write_iter(&target_iocb, iter);

	/* write back to proxy iocb */
	iocb->ki_pos = target_iocb.ki_pos;

	return ret;
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

#define STR(s) #s

#define COPY_FILEOP(opname) \
	if (fileref->file->f_op->opname) { \
		pr_info("assigning " STR(opname) " ptr=%pF\n", fileref->file->f_op->opname); \
		fileref->f_ops.opname = proxy_##opname; \
	} else { \
		pr_info("assigning " STR(opname) " <NULL>\n"); \
		fileref->f_ops.opname = NULL; \
	}

#define TEST_FILEOP(opname) \
	if (fileref->file->f_op->opname) { \
		pr_info("got valid file operation " STR(opname) " ptr=%pF\n", fileref->file->f_op->opname); \
	} else { \
		pr_info("got NULL file operation " STR(opname) "\n"); \
	}

#define SET_FILEOP(opname) \
	fileref->f_ops.opname = proxy_##opname;

void srvfs_proxy_fill_fops(struct file *file)
{
	struct srvfs_fileref *fileref = file->private_data;

	memset(&fileref->f_ops, 0, sizeof(fileref->f_ops));
	fileref->f_ops.owner = THIS_MODULE;

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
	COPY_FILEOP(read_iter);
	COPY_FILEOP(write_iter);
	COPY_FILEOP(iterate);
	COPY_FILEOP(iterate_shared);
#ifndef CONFIG_MMU
	COPY_FILEOP(mmap_capabilities);
#endif

#ifdef CONFIG_SRVFS_VFS_READWRITE
	if (!fileref->f_ops.read)
		fileref->f_ops.read = proxy_vfs_read;
	if (!fileref->f_ops.write)
		fileref->f_ops.write = proxy_vfs_write;
#endif

	TEST_FILEOP(check_flags);

	file->f_op = &fileref->f_ops;
}
