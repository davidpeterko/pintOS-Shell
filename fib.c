#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

const int MAX = 13;

static void doFib(int n, int doPrint);


/*
 * unix_error - unix-style error routine.
 */
inline static void 
unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}


int main(int argc, char **argv)
{
  int arg;
  int print;

  if(argc != 2){
    fprintf(stderr, "Usage: fib <num>\n");
    exit(-1);
  }

  if(argc >= 3){
    print = 1;
  }

  arg = atoi(argv[1]);
  if(arg < 0 || arg > MAX){
    fprintf(stderr, "number must be between 0 and %d\n", MAX);
    exit(-1);
  }

  doFib(arg, 1);

  return 0;
}

/* 
 * Recursively compute the specified number. If print is
 * true, print it. Otherwise, provide it to my parent process.
 *
 * NOTE: The solution must be recursive and it must fork
 * a new child for each call. Each process should call
 * doFib() exactly once.
 */
static void 
doFib(int n, int doPrint)
{
	//PID declarations
	pid_t pid1;			//PID1 of process 1
	//pid_t pidreturn1;	//return PID of process 1
	int status1;		//status (return value) of process 1

	pid_t pid2;			//PID2 of process 2
	//pid_t pidreturn2;	//return PID of process 2
	int status2;		//status (return value) of process 2
	
	int total = 0; 			//total 
	
	//BASE CASE
	if(n == 0 || n == 1){
		
		//check if doPrint = true
		if(doPrint){
			printf("%d\n", n);
		}
		exit(n);
	}
	
	pid1 = fork();				//CREATING CHILD PROCESS
	
	if(pid1 == 0){				//child process1
	
		doFib(n-1, doPrint);
	}
	
	pid2 = fork();				//child process2
	
	if(pid2 == 0){
		
		doFib(n-2, doPrint);
	}
	
	//waitpid calls	
	waitpid(pid1, &status1, 0);
	waitpid(pid2, &status2, 0);
	
	//checking status1 
	if(WIFEXITED(status1)){
		
		total = WEXITSTATUS(status1);
	}
	
	//checking status2
	if(WIFEXITED(status2)){
		
		total += WEXITSTATUS(status2);
	}
	
	//we have total of fib to output
	if(doPrint){
		printf("%d\n",total);
	}
	
	//last value printed should be the parent 
	
	exit(total);
}


