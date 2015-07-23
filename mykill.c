#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char **argv)
{
	pid_t pid1;
	if(argc != 2){
		printf("Error: must have exactly one argument.\n");
		exit(1);
	}
	pid1 = (pid_t) atoi(argv[1]);
	printf("PID: %d\n", pid1);

	kill(pid1, SIGUSR1);
	
	return 0;
}
