#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
 
int main(int argc, char *argv[]) {
    int status;
    int parent_pid = getpid();
    int child_pid = fork();
    if(!child_pid) 
    { 
		// If Child
		//Send to parent 1st time
        char * send_buffer;
        send_buffer = (char*) malloc(6*sizeof(char));
        send_buffer = "Hello";
        printf("Child sending now\n");
        long returnval = syscall(292, parent_pid, 6 *sizeof(char), send_buffer);
        printf("Child: return val %d\n", returnval);
        printf("Child: Message sent.\n");
        
        //Send to parent 2nd time
        char * send_buffer2;
        send_buffer2 = (char*) malloc(6*sizeof(char));
        send_buffer2 = "hello"; //lower case 'h'
        printf("Child sending now\n");
       returnval = syscall(292, parent_pid, 6 *sizeof(char), send_buffer2);
        printf("Child: return val %d\n", returnval);
        printf("Child: Message sent.\n");
        return 0;
    } 
    else 
    { 
		// If Parent
		//Wait for child to finish
        waitpid(child_pid, &status,0);
        //Now try to receive first time
        char * recv_buffer;
        recv_buffer = (char*) malloc(6*sizeof(char));
        printf("Parent receiving now\n");
        long returnval = syscall(293, child_pid, 6*sizeof(char), recv_buffer);
        printf("Parent: return val %d\n", returnval);
        if(returnval == (6*sizeof(char) ) )
        {
			printf("Parent: Message received - %s\n", recv_buffer);
		}
		else
		{
			printf("Message size incorrect\n");
		}
		
		
		//Now try to receive second time
        char * recv_buffer2;
        recv_buffer2 = (char*) malloc(6*sizeof(char));
        printf("Parent receiving now\n");
        returnval = syscall(293, child_pid, 6*sizeof(char), recv_buffer2);
        printf("Parent: return val %d\n", returnval);
        if(returnval == (6*sizeof(char) ) )
        {
			printf("Parent: Message received - %s\n", recv_buffer2);
		}
		else
		{
			printf("Message size incorrect\n");
		}
		
        return 0;
    }
}
