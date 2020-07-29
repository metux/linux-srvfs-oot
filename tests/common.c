#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#include "common.h"

void fail(const char* msg)
{
	fprintf(stderr, "failed: %s\n", msg);
	exit(1);
}

int open_ctrlfile(const char* srvfs, const char* name)
{
	char srvfile[PATH_MAX];
	int ctrl_fd;
	snprintf(srvfile, sizeof(srvfile), "%s/%s", srvfs, name);
	fprintf(stderr, "INFO: srv file name: %s\n", srvfile);

	unlink(srvfile);
	ctrl_fd = open(srvfile, O_CREAT | O_RDWR);

	if (ctrl_fd == -1)
		fail("opening control file");

	fprintf(stderr, "INFO: opened control file at fd %d\n", ctrl_fd);

	return ctrl_fd;
}

int open_localfile(const char* fn)
{
	int fd = open(fn, O_RDWR | O_CREAT);
	if (fd == -1)
		fail("creating local file\n");

	fprintf(stderr, "INFO: opened local file at fd %d\n", fd);

	return fd;
}
