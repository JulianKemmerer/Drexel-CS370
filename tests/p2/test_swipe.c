#include <stdio.h>
#include <stdlib.h>
#include <linux/unistd.h>
#include <errno.h>

void usage() {
	printf("Usage: test_swipe $target $victim\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) 
{
	if(argc != 3) 
		usage();
	long target = atol(argv[1]);
	long victim = atol(argv[2]);
	unsigned int time_stolen = syscall(288, target, victim);
	if(time_stolen != -1)
		printf("Timeslice successfully stolen. %u's new time slice is : %d\n", target, time_stolen);
	else
		printf("An error occured.\n");
}
