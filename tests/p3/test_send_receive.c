#include <stdio.h>
#include <unistd.h>

int released = 0;

int child_send(int pid[3]) {
	int i;
	for(i = 0; i < 3; i++) {
		if(pid[i] != getpid()) { // Send everyone else a message
			char * send_buffer;
			send_buffer = (char*) malloc(6);
			send_buffer = "Hello";
			syscall(292, pid[i], sizeof(char*) * 6, send_buffer);
			printf("PID %d: Sent message to %d\n", getpid(), pid[i]);
		}
	}
}
int child_wait(int pid[3]) {
	int received = 0;
	while(!released) { // Until all is released
		int i;	
		for(i = 0; i < 3; i++) {
			if(pid[i] != getpid()) { // Don't worry bout self sent messages
				char * recv_buffer;
				recv_buffer = (char*) malloc(6);
				long return_val = syscall(293, pid[i], sizeof(char*) *6, recv_buffer);
				if(return_val != -1) {
					printf("PID %d: Received message %s\n", getpid(), recv_buffer);
					received++;
					if(strcmp(recv_buffer, "Done") == 0){
						released = 1;
						printf("PID %d: Escaped early!\n", getpid());
						break;
					}
				}
			}
			
		}

		if(received >= 2) {
			released = 1;
			for(i = 0;i < 3; i++) { // Tell others to escape
				if(pid[i] != getpid()) {
					char * send_buffer;
					send_buffer = (char*) malloc(6);
					send_buffer = "Done";
					syscall(292, pid[i], sizeof(char*) * 6, send_buffer);	
				}
			}
		}
	}
}
int main(int argc, char* argv[]) {
	int pid[3] = {0, 0, 0};
	int child1, child2, child3;
	int fd1[2], fd2[2], fd3[2];
	pipe(fd1);
	pipe(fd2);
	pipe(fd3);
	child1 = fork();
	if(!child1) { // If Child1
		close(fd1[1]);
		read(fd1[0], &pid, sizeof(int) * 3);
		printf("CHILD1: pids %d, %d, %d\n", pid[0], pid[1], pid[2]);
		close(fd1[0]);
		child_send(pid);
		child_wait(pid);
	} else { // If Parent
		child2 = fork();
		if(!child2) { // If Child2
			close(fd2[1]);
			read(fd2[0], &pid, sizeof(int) * 3);
			printf("CHILD2: pids %d, %d, %d\n", pid[0], pid[1], pid[2]);
			close(fd2[0]);
			child_send(pid);
			child_wait(pid);
		} else { // If Parent
			child3 = fork();
			if(!child3) { // If Child3
				close(fd3[1]);
				read(fd3[0], &pid, sizeof(int) * 3);
				printf("CHILD3: pids %d, %d, %d\n", pid[0], pid[1], pid[2]);
				close(fd3[0]);	
				child_send(pid);
				child_wait(pid);
			} else { // If Parent
				close(fd1[0]);
				close(fd2[0]);
				close(fd3[0]);
				pid[0] = child1;
				pid[1] = child2;
				pid[2] = child3;
				
				write(fd1[1], &pid, sizeof(int) * 3);
				write(fd2[1], &pid, sizeof(int) * 3);
				write(fd3[1], &pid, sizeof(int) * 3);
				printf("PARENT: Wrote to children\n");
			}
		}
		}

}
