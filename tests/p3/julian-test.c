#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
 
int main(int argc, char *argv[]) {
    int status;
    int parent_pid = getpid();
    int child_pid = fork();
    long returnval;
    if(!child_pid) 
    { 
		// If Child
		//Send to parent 1st time
        char * send_buffer;
        send_buffer = (char*) malloc(6*sizeof(char));
        send_buffer = "Hello";
        printf("1st Child sending now\n");
        returnval = syscall(292, parent_pid, 6 *sizeof(char), send_buffer);
        printf("Child: return val %d\n", returnval);
        printf("Child: Message sent.\n");
        
        //Send to parent 2nd time
        char * send_buffer2;
        send_buffer2 = (char*) malloc(6*sizeof(char));
        send_buffer2 = "hello"; //lower case 'h'
        printf("2nd Child sending now\n");
       returnval = syscall(292, parent_pid, 6 *sizeof(char), send_buffer2);
        printf("Child: return val %d\n", returnval);
        printf("Child: Message sent.\n");
        return 0;
    } 
    else 
    { 
		// If Parent
		
		//Fork one more really long running child
		//Cheap way to test this
		if(!fork())
		{
			//New child
			//Do something really long running then send message to parent
			long int i;
			printf("Child is learning to count...\n");
			for(i = 0; i < 1000000000; i++)
			{
				
			}
			//Send to parent 3rd time
			char * send_buffer3;
			send_buffer3 = (char*) malloc(6*sizeof(char));
			send_buffer3 = "h3llo"; //3 in word
			printf("3rd Child sending now\n");
			returnval = syscall(292, parent_pid, 6 *sizeof(char), send_buffer3);
			printf("Child: return val %d\n", returnval);
			printf("Child: Message sent.\n");
			return 0;
		}
		else
		{

			//Parent
			
			//Wait for original child to finish
			printf("Parent waiting on original child\n");
			waitpid(child_pid, &status,0);
			//Now try to receive first time
			char * recv_buffer;
			recv_buffer = (char*) malloc(6*sizeof(char));
			printf("Parent receiving now 1\n");
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
			printf("Parent receiving now 2\n");
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
			
			//Now try to receive third time no blocking
			char * recv_buffer3;
			recv_buffer3 = (char*) malloc(6*sizeof(char));
			printf("Parent receiving now3\n");
			returnval = syscall(293, child_pid, 6*sizeof(char), recv_buffer3);
			printf("Parent: return val %d\n", returnval);
			if(returnval == (6*sizeof(char) ) )
			{
				printf("Parent: Message received - %s\n", recv_buffer2);
			}
			else
			{
				printf("Message size incorrect\n");
			}
			
			//Now try to receive third time with blocking
			char * recv_buffer4;
			recv_buffer4 = (char*) malloc(6*sizeof(char));
			printf("Parent receiving now4\n");
			//Receive from anyone, only long running child should send anyway
			returnval = syscall(294, -1, 6*sizeof(char), recv_buffer4,1);
			printf("Parent: return val %d\n", returnval);
			if(returnval == (6*sizeof(char) ) )
			{
				printf("Parent: Message received - %s\n", recv_buffer2);
			}
			else
			{
				printf("Message size incorrect\n");
			}
		
		}
		
        return 0;
    }
}
