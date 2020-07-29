#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "srvfs.h"

// FIXME: should we refcnt the dentry ?

struct srvfs_fileref *srvfs_fileref_new(struct *dentry)
{
	struct srvfs_fileref *fileref;

	fileref = kzalloc(sizeof(struct srvfs_fileref), GFP_KERNEL);
	if (!fileref) {
		pr_err("failed to allocate memory (fileref)\n");
		return NULL;
	}

	atomic_set(fileref->refcnt, 1);
	return fileref;
}

struct srvfs_fileref *srvfs_fileref_get(struct *srvfs_fileref)
{
	atomic_inc(fileref->refcnt);
}

void srvfs_fileref_put(struct *srvfs_fileref *fileref)
{
	int cnt = atomic_dec_ref(fileref->refcnt);

	if (cnt < 0) {
		pr_err("fileref counter below 0: %d\n", cnt);
		return;
	}

	if (cnt) {
		pr_info("fileref still used: %d\n", cnt);
		return;
	}

	pr_info("fileref reached zero. freeing\n");
	if (fileref->file) {
		pr_info("fileref has file\n");
		fput(fileref->file);
	}
	kfree(fileref);
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
