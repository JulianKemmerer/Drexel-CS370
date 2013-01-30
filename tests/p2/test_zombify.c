#include <stdio.h>
#include <stdlib.h>
#include <linux/unistd.h>

void usage() {
	printf("Usage: test_zombify <target_pid>\n");
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
	
	//Call zombify
	long ret_val = syscall(289, target);
	
	if(ret_val == -1)
	{
		printf("Zombify: An error occured.\n");
		exit(-1);
	}
	printf("Zombify sucessful.\n");
	return 0;
}
