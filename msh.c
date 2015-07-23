/* 
 * msh - A mini shell program with job control
 * 
 * <David Ko - dpk326>
 * <Cameron Hooper - cdh2587> 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include "util.h"
#include "jobs.h"


/* Global variables */
int verbose = 0;            /* if true, print additional output */

extern char **environ;      /* defined in libc */
static char prompt[] = "msh> ";    /* command line prompt (DO NOT CHANGE) */
static struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
void usage(void);
void sigquit_handler(int sig);

/* Helper function for writing */
void safe_write(char* str, int len);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{

	//declarations
	pid_t pid1;
	sigset_t sigchld_set;
	int isBG;
	char sbuf [60];
	int jid, slen;

	sigemptyset(&sigchld_set);
	sigaddset(&sigchld_set, SIGCHLD);

	char** argv = (char**)malloc(MAXLINE*sizeof(char));
	isBG = parseline(cmdline, argv);
    
    if(argv[0] == NULL){
		return;
	}
    
    
	if(!builtin_cmd(argv)){
		// process execution

		if(sigprocmask(SIG_BLOCK, &sigchld_set, NULL) == -1) {
			app_error("sigprocmask error");
		}

		if((pid1 = fork()) == 0){					

			setpgid(0,0);
			if(sigprocmask(SIG_UNBLOCK, &sigchld_set, NULL) == -1) {
				app_error("sigprocmask error");
			}
			if(execve(argv[0], argv, environ) < 0){
        
				//cannot execute the command
				printf("%s: Command not found\n", argv[0]);
        
				exit(1);
			}
		}

		if(isBG){
			addjob(jobs, pid1, BG, cmdline);
			if(sigprocmask(SIG_UNBLOCK, &sigchld_set, NULL) == -1) {
				app_error("sigprocmask error");
			}
			jid = pid2jid(jobs, pid1);
			slen = sprintf(sbuf, "[%d] (%d) %s", jid, pid1, cmdline);
			safe_write(sbuf, slen);

		}
		else{
			addjob(jobs, pid1, FG, cmdline);
			if(sigprocmask(SIG_UNBLOCK, &sigchld_set, NULL) == -1) {
				app_error("sigprocmask error");
			}
			waitfg(pid1);
		}

	}
	return;
    
}


/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 * Return 1 if a builtin command was executed; return 0
 * if the argument passed in is *not* a builtin command.
 */
int builtin_cmd(char **argv) 
{
	if(strcmp(argv[0], "quit") == 0){
		exit(0);

		return 1;
	}

	if(strcmp(argv[0], "fg") == 0){
		//printf("Accepted command: fg.\n");

		do_bgfg(argv);

		return 1;
	}
	else if(strcmp(argv[0], "bg") == 0){
		//printf("Accepted command: bg.\n");
		
		do_bgfg(argv);
	
		return 1;
	}
	else if(strcmp(argv[0], "jobs") == 0){
		//printf("Accepted command: jobs.\n");
	
		//print out list of jobs
		listjobs(jobs);
	
		return 1;
	}

	return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
	//built-in cmd calls bg,fg commands here
	//int num;
	int PID1;
	int JID1;
	struct job_t *job1;

	//no PID or JID or argv
	if(argv[1] == NULL){
		if(strcmp(argv[0], "fg") == 0){
			printf("fg command requires PID or %%jobid argument\n");
		} else {
			printf("bg command requires PID or %%jobid argument\n");
		}
		return;
	}	

	//check if PID
	if(sscanf(argv[1], "%i", &PID1) == 1){
			

		job1 = getjobpid(jobs, PID1);	
		//check jobs list if it is an actual process, othewise the pione will return NULL
		if(job1 == NULL ){
			printf("(%d): No such process\n", PID1);
			return;
		}
	}
	//check if JID
	else if(argv[1][0] == '%'){

		JID1 = argv[1][1] - '0';		//gets the JID without the %
		//printf("%d\n", JID1);
		job1 = getjobjid(jobs, JID1);

		if(job1 == NULL){
		
			printf("%%%d: No such job\n", JID1);
			return;
		}
	}
	else{
		if(strcmp(argv[0], "fg") == 0){
			printf("fg: argument must be a PID or %%jobid\n");
		} else {
			printf("bg: argument must be a PID or %%jobid\n");
		}
		return;
	}
	
	//set state and send signal
	if(strcmp(argv[0], "fg") == 0){
		job1->state = FG;

		kill(-(job1->pid), SIGCONT);

		waitfg(job1->pid);
	}else{
		job1->state = BG;
	
		kill(-(job1->pid), SIGCONT);

		printf("[%d] (%d) %s", job1->jid, job1->pid, job1->cmdline);
	}

	return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
	int status;
	struct job_t *fgJob = getjobpid(jobs, pid);
	struct timespec t1;

        t1.tv_sec = 0;
        t1.tv_nsec = 50000000L;

/*	
	waitpid(pid, &status, WUNTRACED);
	
	if(WIFEXITED(status) || WCOREDUMP(status)) {
		deletejob(jobs, pid);
	}
	else if(WIFSIGNALED(status)) {
		fgJob = getjobpid(jobs, pid);
		printf("Job [%d] (%d) terminated by signal %d\n", fgJob->jid, fgJob->pid, SIGINT);
		deletejob(jobs, pid);
	}
	else if(WIFSTOPPED(status)) {
		fgJob = getjobpid(jobs, pid);
		printf("Job [%d] (%d) stopped by signal %d\n", fgJob->jid, fgJob->pid, SIGTSTP);
		fgJob->state = ST;
	}
*/
	while(waitpid(pid, &status, WNOHANG | WUNTRACED)==0){
		if(fgJob->state == ST) {
			break;
		}
		nanosleep(&t1, NULL);
	}
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
	int status;
	pid_t savedPID;
        char sbuf [50];
        int slen;

	
	//uses WNOHANG to return immediately if no child has exited
	//WNOHANG = no wait
	//sigchild -> check if job status is in background
	//sent when a child process is terminated or finishes
	//preivous conditional: (savedPID = waitpid(-1, &status, WNOHANG))
	while(((savedPID = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)){
		
		//retrieve the job of the PID
		struct job_t *savedJob = getjobpid(jobs, savedPID);
		/*
		//if its the fg job, then return
		if(savedJob->state == FG){
			return;
		}
		*/

		if(WIFEXITED(status) || WCOREDUMP(status)) {
                	deletejob(jobs, savedPID);
        	}
        	else if(WIFSIGNALED(status)) {
                	slen = sprintf(sbuf, "Job [%d] (%d) terminated by signal %d\n", savedJob->jid, savedJob->pid, SIGINT);
			safe_write(sbuf, slen);
                	deletejob(jobs, savedPID);
        	}
        	else if(WIFSTOPPED(status)) {
                	slen = sprintf(sbuf, "Job [%d] (%d) stopped by signal %d\n", savedJob->jid, savedJob->pid, SIGTSTP);
			safe_write(sbuf, slen);
                	savedJob->state = ST;
		}



		//removing job from job list
		//deletejob(jobs, savedPID); 

	//note: segmentation fault when exiting while loop
	}
	return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
	pid_t pid;
	//char sbuf [50];
	//int jid, slen;
	
	// if a fg job is running, pass the signal to kill it
	if((pid = fgpid(jobs)) > 0){
		kill(-pid, SIGINT);
		//write out
		//jid = pid2jid(jobs, pid);
		//slen = sprintf(sbuf, "Job [%d] (%d) terminated by signal %d\n", jid, pid, SIGINT);
		//safe_write(sbuf, slen);
	}
	
	return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
	pid_t pid;
	//char sbuf [50];
	//int jid, slen;
	
	// if a fg job is running, pass the signal to stop it
	if((pid = fgpid(jobs)) > 0){
		kill(-pid, SIGTSTP);
		//write out 
		//jid = pid2jid(jobs, pid);
		//slen = sprintf(sbuf, "Job [%d] (%d) stopped by signal %d\n", jid, pid, SIGTSTP);
		//safe_write(sbuf, slen);
	}
    return;
}

/*********************
 * End signal handlers
 *********************/



/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    ssize_t bytes;
    const int STDOUT = 1;
    bytes = write(STDOUT, "Terminating after receipt of SIGQUIT signal\n", 45);
    if(bytes != 45)
       exit(-999);
    exit(1);
}

/*
 * safe_write - use the write system call to safely print to the
 *    terminal from a signal handler
 */
void safe_write(char* str, int len)
{
	ssize_t bytes;
	const int STDOUT = 1;
	bytes = write(STDOUT, str, len);
	if(bytes != len)
		exit(-999);
}
