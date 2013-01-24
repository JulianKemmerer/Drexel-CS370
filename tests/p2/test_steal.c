#include <stdio.h>
#include <stdlib.h>
#include <linux/unistd.h>
#include <errno.h>

void usage() {
	printf("Usage: test_steal $PID\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) 
{
	if(argc != 2) 
		usage();
	long pid = atol(argv[1]);
	if(pid)
	{
		if(syscall(286, pid) == -1)
			printf("pid doesn't exist!\n");
	}
	else
		usage();
}
