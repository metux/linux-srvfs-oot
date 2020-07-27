#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

/*
 * Boilerplate stuff.
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("mtx");

#define SRVFS_MAGIC 0x19980123

/*
 * Implement an array of counters.
 */
#define NCOUNTERS 4
static atomic_t counters[NCOUNTERS];



/*
 * The operations on our "files".
 */

/*
 * Open a file.  All we have to do here is to copy over a
 * copy of the counter pointer so it's easier to get at.
 */
static int srvfs_open(struct inode *inode, struct file *filp)
{
	if (inode->i_ino > NCOUNTERS)
		return -ENODEV;  /* Should never happen.  */
	filp->private_data = counters + inode->i_ino - 1;
	return 0;
}

#define TMPSIZE 20
/*
 * Read a file.  Here we increment and read the counter, then pass it
 * back to the caller.  The increment only happens if the read is done
 * at the beginning of the file (offset = 0); otherwise we end up counting
 * by twos.
 */
static ssize_t srvfs_read_file(struct file *filp, char *buf,
	size_t count, loff_t *offset)
{
	int v, len;
	char tmp[TMPSIZE];
	atomic_t *counter = (atomic_t *) filp->private_data;
	/*
	 * Encode the value, and figure out how much of it we can pass back.
	 */
	v = atomic_read(counter);
	if (*offset > 0)
		v -= 1;  /* the value returned when offset was zero */
	else
		atomic_inc(counter);
	len = snprintf(tmp, TMPSIZE, "%d\n", v);
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

/*
 * Write a file.
 */
static ssize_t srvfs_write_file(struct file *filp, const char *buf,
				size_t count, loff_t *offset)
{
	char tmp[TMPSIZE];
	long fd_id;
	atomic_t *counter = (atomic_t *) filp->private_data;
	/*
	 * Only write from the beginning.
	 */
	if (*offset != 0)
		return -EINVAL;
	/*
	 * Read the value from the user.
	 */
	if (count >= TMPSIZE)
		return -EINVAL;
	memset(tmp, 0, TMPSIZE);
	if (copy_from_user(tmp, buf, count))
		return -EFAULT;
	/*
	 * Store it in the counter and we are done.
	 */
	fd_id = simple_strtol(tmp, NULL, 10);
	pr_info("requested to assign fd %ld\n", fd_id);
	atomic_set(counter, simple_strtol(tmp, NULL, 10));
	return count;
}

/*
 * Now we can put together our file operations structure.
 */
static struct file_operations lfs_file_ops = {
	.open	= srvfs_open,
	.read	= srvfs_read_file,
	.write	= srvfs_write_file,
};

/*
 * OK, create the files that we export.
 */
struct tree_descr OurFiles[] = {
	{ NULL, NULL, 0 },  /* Skipped */
	{
		.name = "counter0",
		.ops = &lfs_file_ops,
		.mode = S_IWUSR|S_IRUGO
	},
	{
		.name = "counter1",
		.ops = &lfs_file_ops,
		.mode = S_IWUSR|S_IRUGO
	},
	{
		.name = "counter2",
		.ops = &lfs_file_ops,
		.mode = S_IWUSR|S_IRUGO
	},
	{
		.name = "counter3",
		.ops = &lfs_file_ops,
		.mode = S_IWUSR|S_IRUGO
	},
	{ "", NULL, 0 }
};

/*
 * Superblock stuff.  This is all boilerplate to give the vfs something
 * that looks like a filesystem to work with.
 */

/*
 * "Fill" a superblock with mundane stuff.
 */
static int lfs_fill_super (struct super_block *sb, void *data, int silent)
{
	return simple_fill_super(sb, SRVFS_MAGIC, OurFiles);
}

/*
 * Stuff to pass in when registering the filesystem.
 */
struct dentry *srvfs_mount(struct file_system_type *fs_type,
			   int flags, const char *dev_name, void *data)
{
	return mount_nodev(fs_type, flags, data, lfs_fill_super);
}

static struct file_system_type lfs_type = {
	.owner 		= THIS_MODULE,
	.name		= "srvfs",
	.mount		= srvfs_mount,
	.kill_sb	= kill_litter_super,
};

static int __init srvfs_init(void)
{
	int i;

	for (i = 0; i < NCOUNTERS; i++)
		atomic_set(counters + i, 0);
	return register_filesystem(&lfs_type);
}

static void __exit srvfs_exit(void)
{
	unregister_filesystem(&lfs_type);
}

module_init(srvfs_init);
module_exit(srvfs_exit);
