#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

/*
 * Boilerplate stuff.
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("mtx");

#define SRVFS_MAGIC 0x19980123

#define NCOUNTERS 4
static atomic_t counters[NCOUNTERS];


struct srvfs_inode {
	atomic_t counter;
	int mode;
};


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
static struct file_operations srvfs_file_ops = {
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
		.ops = &srvfs_file_ops,
		.mode = S_IWUSR|S_IRUGO
	},
	{
		.name = "counter1",
		.ops = &srvfs_file_ops,
		.mode = S_IWUSR|S_IRUGO
	},
	{
		.name = "counter2",
		.ops = &srvfs_file_ops,
		.mode = S_IWUSR|S_IRUGO
	},
	{
		.name = "counter3",
		.ops = &srvfs_file_ops,
		.mode = S_IWUSR|S_IRUGO
	},
	{ "", NULL, 0 }
};

/*
 * Superblock stuff.  This is all boilerplate to give the vfs something
 * that looks like a filesystem to work with.
 */

static int srvfs_create_file (struct super_block *sb, struct dentry *root, const char* name, int idx)
{
	struct dentry *dentry;
	struct inode *inode;
	struct srvfs_inode *priv;

	priv = kmalloc(sizeof(struct srvfs_inode), GFP_KERNEL);
	if (!priv) {
		pr_err("srvfs_create_file(): failed to malloc inode priv\n");
		goto err;
	}

	atomic_set(&priv->counter, 0);

	dentry = d_alloc_name(root, name);
	if (!dentry)
		goto err;
	inode = new_inode(sb);
	if (!inode) {
		dput(dentry);
		goto err;
	}
	inode->i_mode = S_IFREG | S_IWUSR | S_IRUGO;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_fop = &srvfs_file_ops;
	inode->i_ino = idx;

	d_add(dentry, inode);
	return 1;
// FIXME: release resources
err:
	return 0;
}

static const struct super_operations simple_super_operations = {
	.statfs	= simple_statfs,
};

/*
 * "Fill" a superblock with mundane stuff.
 */
static int srvfs_fill_super (struct super_block *sb, void *data, int silent)
{
	struct inode *inode;
	struct dentry *root;
	int i;
	struct tree_descr *files = OurFiles;

	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_magic = SRVFS_MAGIC;
	sb->s_op = &simple_super_operations;
	sb->s_time_gran = 1;

	inode = new_inode(sb);
	if (!inode)
		return -ENOMEM;
	/*
	 * because the root inode is 1, the files array must not contain an
	 * entry at index 1
	 */
	inode->i_ino = 1;
	inode->i_mode = S_IFDIR | 0755;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_op = &simple_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;
	set_nlink(inode, 2);
	root = d_make_root(inode);
	if (!root) {
		pr_info("fill_super(): could not create root\n");
		return -ENOMEM;
	}

	for (i = 0; !files->name || files->name[0]; i++, files++) {
		if (!files->name)
			continue;

		/* warn if it tries to conflict with the root inode */
		if (unlikely(i == 1))
			printk(KERN_WARNING "%s: %s passed in a files array"
				"with an index of 1!\n", __func__,
			sb->s_type->name);

		if (!srvfs_create_file(sb, root, files->name, i))
			goto out;
	}
	sb->s_root = root;
	return 0;
out:
	d_genocide(root);
	shrink_dcache_parent(root);
	dput(root);
	return -ENOMEM;
}

/*
 * Stuff to pass in when registering the filesystem.
 */
struct dentry *srvfs_mount(struct file_system_type *fs_type,
			   int flags, const char *dev_name, void *data)
{
	return mount_nodev(fs_type, flags, data, srvfs_fill_super);
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
