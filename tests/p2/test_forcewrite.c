#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/unistd.h>
#include <fcntl.h>

void usage() {
	printf("Usage: test_forcewrite <target_file>\n");
	exit(0);
}

int main(int argc, char *argv[]) 
{
	//Use first arg as file, check if exists
	if(argc < 2)
	{
		usage();
	}
	
	char * filename = argv[1];

	char * normal = "Normal Write.\n";
	int normalSize = (strlen(normal) +1)*sizeof(char);
	char * forced = "Forced Write.\n";
	int forcedSize = (strlen(forced) +1)*sizeof(char);

	//Open the file for writing (appending too)
	//O_WRONLY | O_APPEND
	int fd = open(filename,O_RDONLY);
	
	//Check that opening suceeded
    if(fd < 0)
    {
		printf("Could not open the file.\n");
        return -1;
	}
 
	//Try to write normally
    if(write(fd,normal, normalSize) != normalSize)
    {
		printf("Could not write to the file normally.\n");
    }
    
    //Try to force write
    if(syscall(291,fd,forced, forcedSize) != forcedSize)
    {
		printf("Could not force write to the file.\n");
		return -1;
    }
    return 0;
}
