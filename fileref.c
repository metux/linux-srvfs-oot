#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/slab.h>
#include <linux/file.h>

#include "srvfs.h"

struct srvfs_fileref *srvfs_fileref_new(struct dentry *dentry)
{
	struct srvfs_fileref *fileref;

	fileref = kzalloc(sizeof(struct srvfs_fileref), GFP_KERNEL);
	if (!fileref) {
		pr_err("failed to allocate memory (fileref)\n");
		return NULL;
	}

	kref_init(&fileref->refcount);
	return fileref;
}

struct srvfs_fileref *srvfs_fileref_get(struct srvfs_fileref *fileref)
{
	kref_get(&fileref->refcount);
	return fileref;
}

void srvfs_fileref_destroy(struct kref *ref)
{
	struct srvfs_fileref *fileref = container_of(ref, struct srvfs_fileref, refcount);
	pr_info("fileref reached zero. freeing\n");
	if (fileref->file) {
		pr_info("fileref has file\n");
		fput(fileref->file);
	}
	kfree(fileref);
}

void srvfs_fileref_put(struct srvfs_fileref *fileref)
{
	kref_put(&fileref->refcount, srvfs_fileref_destroy);
}

void srvfs_fileref_set(struct srvfs_fileref *fileref, struct file *newfile)
{
	struct file *oldfile;

	if (!newfile) {
		pr_info("no valid file descriptor. clearing\n");
	}

	oldfile = fileref->file;
	fileref->file = newfile;

	if (oldfile) {
		pr_info("unrefing old file\n");
		fput(oldfile);
	}
}
