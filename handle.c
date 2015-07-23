#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "util.h"


/*
 * First, print out the process ID of this process.
 *
 * Then, set up the signal handler so that ^C causes
 * the program to print "Nice try.\n" and continue looping.
 *
 * Finally, loop forever, printing "Still here\n" once every
 * second.
 */
void sigintHandler(int sig) {

	ssize_t bytes; 
	const int STDOUT = 1; 
	bytes = write(STDOUT, "Nice try.\n", 10); 
	if(bytes != 10) 
		exit(-999);

}

void sigusr1Handler(int sig) {

	ssize_t bytes; 
	const int STDOUT = 1; 
	bytes = write(STDOUT, "exiting\n", 8); 
	if(bytes != 8) 
		exit(-999);
	exit(1);

}

int main(int argc, char **argv)
{

	int counter = 0;
	pid_t inputPID;
	struct timespec t1;

	t1.tv_sec = 0;
	t1.tv_nsec = 100000000L;
	
	inputPID = getpid();

	printf("PID: %d\n", inputPID);

	Signal(SIGINT, sigintHandler);
	Signal(SIGUSR1, sigusr1Handler);

	while(1){

		if(counter == 10){
			counter = 0;
			ssize_t bytes; 
			const int STDOUT = 1; 
			bytes = write(STDOUT, "Still here.\n", 12); 
			if(bytes != 12) 
   				exit(-999);
		}

		nanosleep(&t1, NULL);

		counter++;
	}


  return 0;
}


