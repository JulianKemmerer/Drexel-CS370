#include <stdio.h>
#include <unistd.h>

int main(){
	int i;
	for(i = 10; i > 0; i--){
		if(i == 10 || i == 5 || i == 3 || i == 2 || i == 1)
			printf("Do Nothing Says: will die in %d seconds!\n", i);
		sleep(1);
	}
	exit(0);
}
