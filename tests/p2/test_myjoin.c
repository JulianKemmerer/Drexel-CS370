#include <stdio.h>
#include <stdlib.h>
#include <linux/unistd.h>
#include <errno.h>

void usage() {
	printf("Usage: test_myjoin %PID\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	if(argc != 2)
		usage();
	long pid = atol(argv[1]);
	if(pid) {
		printf("test_myjoin: Will be waiting here forever until you DIE \n");
		if(syscall(290, pid) == -1)
			printf("test_myjoin: err... never mind something messed up\n");
	}
	else
		usage();

	printf("test_myjoin: I AM FREE!!!\n");
	exit(EXIT_SUCCESS);
}
