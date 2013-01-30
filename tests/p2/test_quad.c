#include <stdio.h>
#include <stdlib.h>
#include <linux/unistd.h>

void usage() {
	printf("Usage: test_swipe $target $victim\n");
	exit(0);
}

int main(int argc, char *argv[]) 
{
	//Use first arg as pid, check if exists
	if(argc < 2)
	{
		usage();
	}
	
	pid_t target = atol(argv[1]);
	
	//Call quad
	long new_time_slice = syscall(287, target);
	
	
	if(new_time_slice == -1)
	{
		printf("Quad: An error occured.\n");
		exit(-1);
	}
	printf("Timeslice quadruple successful. New timeslice = %ld\n", new_time_slice);
	return 0;
}
