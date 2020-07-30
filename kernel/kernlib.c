#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/* library functions copied over from the kernel, as they're
   exported to modules

   when moving in-tree, we can export them from their original
   places and drop them from here.
*/

#include "srvfs.h"

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <linux/splice.h>
#include <linux/highmem.h>

/* copied from fs/splice.c -- unfortunately not exported */
ssize_t splice_from_pipe(struct pipe_inode_info *pipe, struct file *out,
			 loff_t *ppos, size_t len, unsigned int flags,
			 splice_actor *actor)
{
	ssize_t ret;
	struct splice_desc sd = {
		.total_len = len,
		.flags = flags,
		.pos = *ppos,
		.u.file = out,
	};

	pipe_lock(pipe);
	ret = __splice_from_pipe(pipe, &sd, actor);
	pipe_unlock(pipe);

	return ret;
}

/* copied from fs/splice.c -- unfortunately not exported */
static int write_pipe_buf(struct pipe_inode_info *pipe, struct pipe_buffer *buf,
			  struct splice_desc *sd)
{
	int ret;
	void *data;
	loff_t tmp = sd->pos;

	data = kmap(buf->page);
	ret = __kernel_write(sd->u.file, data + buf->offset, sd->len, &tmp);
	kunmap(buf->page);

	return ret;
}

/* copied from fs/splice.c -- unfortunately not exported */
static ssize_t default_file_splice_write(struct pipe_inode_info *pipe,
					 struct file *out, loff_t *ppos,
					 size_t len, unsigned int flags)
{
	ssize_t ret;

	ret = splice_from_pipe(pipe, out, ppos, len, flags, write_pipe_buf);
	if (ret > 0)
		*ppos += ret;

	return ret;
}

/* copied from fs/splice.c */
long do_splice_from(struct pipe_inode_info *pipe, struct file *out,
		    loff_t *ppos, size_t len, unsigned int flags)
{
	ssize_t (*splice_write)(struct pipe_inode_info *, struct file *,
				loff_t *, size_t, unsigned int);

	if (out->f_op->splice_write)
		splice_write = out->f_op->splice_write;
	else
		splice_write = default_file_splice_write;

	return splice_write(pipe, out, ppos, len, flags);
}

/* copied over from vfs fs/ioctl.c -- unfortunately not exported */
long vfs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int error = -ENOTTY;
	if (!filp->f_op->unlocked_ioctl)
		goto out;

	error = filp->f_op->unlocked_ioctl(filp, cmd, arg);
	if (error == -ENOIOCTLCMD)
		error = -ENOTTY;
out:
	return error;
}
