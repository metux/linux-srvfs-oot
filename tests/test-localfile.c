
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "common.h"

#define SRVFILENAME	"test-localfile"

int doit(const char* srvfs, const char* fn)
{
	int ctrl_fd = open_ctrlfile(srvfs, SRVFILENAME);
	int local_fd = open_localfile(fn);

	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "%ld", local_fd);
	write(ctrl_fd, buffer, strlen(buffer));
	close(ctrl_fd);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc<2)
		fail("parameters: <srvfs> <localfile");

	doit(argv[1], argv[2]);
}
