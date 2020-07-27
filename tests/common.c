#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>

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

int assign_fd(const char* srvfs, const char* ctrlname, int local_fd)
{
	char buffer[1024];
	int ctrl_fd = open_ctrlfile(srvfs, ctrlname);

	fprintf(stderr, "INFO assigning fd %d to %s/%s (%d)\n", local_fd, srvfs, ctrlname, ctrl_fd);
	snprintf(buffer, sizeof(buffer), "%ld", local_fd);
	write(ctrl_fd, buffer, strlen(buffer));
	close(ctrl_fd);
	return 0;
}
