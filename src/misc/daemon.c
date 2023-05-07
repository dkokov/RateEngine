#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#include "globals.h"

#include "daemon.h"

/* Send SIGTERM to the RateEngine daemon */
void stop_daemon(char *pidfileName)
{
	int pid;
	int pidFile;
	char buf[8];
	
	pidFile = open(pidfileName, O_RDONLY, 0600);
	
	if(pidFile >= 0) {
		bzero(buf,sizeof(buf));
		
		if((read(pidFile,buf,sizeof(buf))) >= 0) {
			pid = atoi(buf);
			
			if(pid) {
				kill(pid,SIGTERM);
				fprintf(stderr,"\nSend SIGTERM signal to the RateEngine proccess with PID:%d!\n",pid);
			}
		} else fprintf(stderr,"\nCannot read a pid file '%s'!\n",pidfileName);
	} else fprintf(stderr,"\nCannot open a pid file '%s'!\n",pidfileName);
}

/* SignalHandler in the RateEngine daemon */
void signal_handler(int sig)
{
    switch(sig) {
        case SIGHUP:
        case SIGINT:
        case SIGTERM:
            daemonShutdown();
            LOG("signal_handler()","A RateEngine daemon is terminating...");
            exit(EXIT_SUCCESS);
            break;
        case SIGKILL:
            daemonShutdown();
            LOG("signal_handler()","A RateEngine daemon is killing...");
            exit(EXIT_SUCCESS);
            break;
		case SIGUSR1:
			LOG("signal_handler()","CallControl threads is stoping...");
			break;
		case SIGUSR2:
			break;
        default:
            LOG("signal_handler()","Unhandled signal %s", strsignal(sig));
            break;
    }
}

void daemonShutdown(void)
{
	/* stop tcp server cycle */
//	tcp_server_sflag = 0;
	
	/* stop cache engine cycle */
//	cache_engine_sflag = 0;
	
	/* stop 're5_manager()' loop */
	loop_flag = 'f';
	
	/* close pid file */
	close(pidFilehandle);
	
	/* remove pid file */
	remove(mcfg->system_pid_file);
}

void daemonize(char *rundir, char *pidfile)
{
    int fd;
    int pid, sid;
    char str[10];
    struct sigaction newSigAction;
    sigset_t newSigSet;
    
    /* Check if parent process id is set */
    if (getppid() == 1) {
        exit(EXIT_FAILURE);
    }
    /* Set signal mask - signals we want to block */
    sigemptyset(&newSigSet);
    sigaddset(&newSigSet, SIGCHLD);  			/* ignore child - i.e. we don't need to wait for it */
    sigaddset(&newSigSet, SIGTSTP);  			/* ignore tty stop signals */
    sigaddset(&newSigSet, SIGTTOU);  			/* ignore tty background writes */
    sigaddset(&newSigSet, SIGTTIN);  			/* ignore tty background reads */
    sigprocmask(SIG_BLOCK, &newSigSet, NULL);   /* Block the above specified signals */

    /* Set up a signal handler */
    newSigAction.sa_handler = signal_handler;
    sigemptyset(&newSigAction.sa_mask);
    newSigAction.sa_flags = 0;

    /* Signals to handle */
    sigaction(SIGUSR1, &newSigAction, NULL);
    sigaction(SIGUSR2, &newSigAction, NULL);
    sigaction(SIGTERM, &newSigAction, NULL);
	sigaction(SIGKILL, &newSigAction, NULL);

    /* Fork*/
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        fprintf(stderr,"\nRE process created: %d\n", pid);
        LOG("demonize()","RE process is created: %d",pid);
        exit(EXIT_SUCCESS);
    }

    /* Child continues */

	/* Set file permissions 750 */
    umask(027); 

    /* Get a new process group */
    sid = setsid();
    if (sid < 0) {
		exit(EXIT_FAILURE);
    }

	/* redirect std* to null */
	fd = open("/dev/null", O_RDONLY);
	if (fd != 0) {
		dup2(fd, 0);
		close(fd);
	}

	fd = open("/dev/null", O_WRONLY);
	if (fd != 1) {
		dup2(fd, 1);
		close(fd);
	}

	fd = open("/dev/null", O_WRONLY);
	if (fd != 2) {
		dup2(fd, 2);
		close(fd);
	}

	/* change running directory */
    chdir(rundir); 

    /* Ensure only one copy */
    pidFilehandle = open(pidfile, O_RDWR|O_CREAT, 0600);
    if(pidFilehandle == -1 ) {
       LOG("daemonize()","Could not open PID lock file %s, exiting", pidfile);
       exit(EXIT_FAILURE);
    }

    /* Try to lock file */
    if(lockf(pidFilehandle,F_TLOCK,0) == -1) {
        LOG("daemonize()","Could not lock PID lock file %s, exiting", pidfile);
        fprintf(stderr,"\nCould not lock PID lock file %s,exiting\n",pidfile);
        exit(EXIT_FAILURE);
    }

    /* Get and format PID */
    sprintf(str,"%d\n",getpid());

    /* write pid to lockfile */
    write(pidFilehandle, str, strlen(str));    
}
