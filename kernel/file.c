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
	struct srvfs_fileref *fileref = inode->i_private;
	pr_info("open inode_id=%ld\n", inode->i_ino);
	file->private_data = srvfs_fileref_get(fileref);

	if (fileref->file) {
		pr_info("open inode: already assigned another file\n");
		srvfs_proxy_fill_fops(file);
//		file->f_op = &proxy_file_ops;
	}
	else {
		pr_info("open inode: no file assigned yet\n");
	}

	return 0;
}

static int srvfs_file_release(struct inode *inode, struct file *file)
{
	struct srvfs_fileref *fileref = file->private_data;
	pr_info("closing vanilla control file: inode_id=%ld\n", inode->i_ino);
	srvfs_fileref_put(fileref);
	return 0;
}

#define TMPSIZE 20
/*
 * Read a file.  Here we increment and read the counter, then pass it
 * back to the caller.  The increment only happens if the read is done
 * at the beginning of the file (offset = 0); otherwise we end up counting
 * by twos.
 */
static ssize_t srvfs_file_read(struct file *file, char *buf,
	size_t count, loff_t *offset)
{
	int v, len;
	char tmp[TMPSIZE];
	struct srvfs_fileref *fileref = file->private_data;

	if (fileref->file) {
		pr_err("srvfs_file_read() routed to the wrong file\n");
	} else {
		pr_info("srvfs_file_read() read on vanilla control file\n");
	}

	/*
	 * Encode the value, and figure out how much of it we can pass back.
	 */
	v = atomic_read(&fileref->counter);
	if (*offset > 0)
		v -= 1;  /* the value returned when offset was zero */
	else
		atomic_inc(&fileref->counter);
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

#define STR(s) #s

#define CHECK_OP(name) \
	if (newfile->f_op->name == NULL) \
		pr_warn("assigned file misses " STR(name) " operation"); \
	else \
		pr_info("assigned file has " STR(name) " operation: %pF", newfile->f_op->name); \

static int do_switch(struct file *file, long fd)
{
	struct srvfs_fileref *fileref= file->private_data;
	struct file *newfile = fget(fd);
	pr_info("doing the switch: fd=%ld\n", fd);

	if (!newfile) {
		pr_info("invalid fd passed\n");
		goto setref;
	}

	if (newfile->f_inode == file->f_inode) {
		pr_err("whoops. trying to link inode with itself!\n");
		goto loop;
	}

	if (newfile->f_inode->i_sb == file->f_inode->i_sb) {
		pr_err("whoops. trying to link inode within same fs!\n");
		goto loop;
	}

	pr_info("assigning inode %ld\n", newfile->f_inode->i_ino);
	if (newfile && newfile->f_path.dentry)
		pr_info("target inode fn: %s\n", newfile->f_path.dentry->d_name.name);
	else
		pr_info("target inode fn unknown\n");

	CHECK_OP(read)
	CHECK_OP(write)
	CHECK_OP(flush)
	CHECK_OP(release)

setref:
	srvfs_fileref_set(fileref, newfile);
	return 0;

loop:
	fput(newfile);
	return -ELOOP;
}

static ssize_t srvfs_file_write(struct file *file, const char *buf,
				size_t count, loff_t *offset)
{
	char tmp[TMPSIZE];
	long fd;
	int ret;

	if ((*offset != 0) || (count >= TMPSIZE))
		return -EINVAL;

	memset(tmp, 0, TMPSIZE);
	if (copy_from_user(tmp, buf, count))
		return -EFAULT;

	fd = simple_strtol(tmp, NULL, 10);
	ret = do_switch(file, fd);

	if (ret)
		return ret;

	return count;
}

struct file_operations srvfs_file_ops = {
	.owner		= THIS_MODULE,
	.open		= srvfs_file_open,
	.read		= srvfs_file_read,
	.write		= srvfs_file_write,
	.release	= srvfs_file_release,
};

int srvfs_insert_file (struct super_block *sb, struct dentry *dentry)
{
	struct inode *inode;
	struct srvfs_fileref *fileref;
	int mode = S_IFREG | S_IWUSR | S_IRUGO;

	fileref = srvfs_fileref_new();
	if (!fileref)
		goto nomem;

	inode = new_inode(sb);
	if (!inode)
		goto err_inode;

	atomic_set(&fileref->counter, 0);

	inode_init_owner(inode, sb->s_root->d_inode, mode);

	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_fop = &srvfs_file_ops;
	inode->i_ino = srvfs_inode_id(inode->i_sb);
	inode->i_private = fileref;

	pr_info("new inode id: %ld\n", inode->i_ino);

	d_drop(dentry);
	d_add(dentry, inode);
	return 0;

err_inode:
	srvfs_fileref_put(fileref);
nomem:
	pr_err("failed to allocate memory\n");
	return -ENOMEM;
}
