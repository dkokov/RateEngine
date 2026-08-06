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

/* Read the pid recorded in the pid file, 0 when unreadable/empty. Used only to
 * make the "already running" message informative - never to decide whether we
 * are allowed to start. */
static int daemon_pidfile_read(char *pidfile)
{
    int rpid = 0;
    int pf = open(pidfile, O_RDONLY, 0600);

    if(pf >= 0) {
        char pbuf[16];

        bzero(pbuf,sizeof(pbuf));

        if(read(pf,pbuf,sizeof(pbuf)-1) > 0) rpid = atoi(pbuf);

        close(pf);
    }

    return rpid;
}

/* Single-instance guard, part 1 of 2: is the pid file already locked by a LIVE
 * instance?
 *
 * F_TEST reports a lock held by another process. The previous probe read the
 * pid and called kill(pid,0), which is wrong in a container: foreground mode
 * records pid 1, so on the next start the new RateEngine is *itself* pid 1,
 * kill(1,0) succeeded and it refused to start - any crash/docker kill/OOM
 * bricked startup until the file was deleted by hand. A lock cannot go stale:
 * the kernel drops it when the holder dies. It also lives on the inode, so it
 * works across containers bind-mounting the same file.
 *
 * The authoritative lock is taken later by daemon_pidfile_write(); this check
 * exists so that '-d' can report the failure BEFORE fork() - record locks are
 * not inherited across fork, so the child takes the real lock only after the
 * parent has already printed "RE process created" and exited 0.
 *
 * Returns RE_SUCCESS when it is safe to start, RE_ERROR when already running. */
static int daemon_chk_running(char *pidfile)
{
    int pf = open(pidfile, O_RDWR, 0600);

    if(pf >= 0) {
        if(lockf(pf,F_TEST,0) == -1) {
            int rpid = daemon_pidfile_read(pidfile);

            close(pf);

            fprintf(stderr,"\nRateEngine is already running (PID %d)! "
                           "Cannot start a second instance.\n",rpid);
            LOG("daemon_chk_running()","RateEngine is already running (PID %d), "
                              "refusing to start a second instance",rpid);
            return RE_ERROR;
        }

        close(pf);
    }

    return RE_SUCCESS;
}

/* Signal disposition shared by background(-d) and foreground(-f) service mode.
 * Foreground also needs it: without a SIGTERM/SIGINT handler 'docker stop' and
 * Ctrl-C kill the process hard, leaving a stale pid file and SHM segment. */
static void daemon_setup_signals(void)
{
    struct sigaction newSigAction;
    sigset_t newSigSet;

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

    /* Signals to handle. SIGKILL cannot be caught - the sigaction() call for it
     * just fails harmlessly; SIGINT/SIGHUP matter for the foreground path
     * (Ctrl-C, 'docker stop' sends SIGTERM then SIGKILL). */
    sigaction(SIGUSR1, &newSigAction, NULL);
    sigaction(SIGUSR2, &newSigAction, NULL);
    sigaction(SIGTERM, &newSigAction, NULL);
    sigaction(SIGINT,  &newSigAction, NULL);
    sigaction(SIGHUP,  &newSigAction, NULL);
	sigaction(SIGKILL, &newSigAction, NULL);
}

/* Single-instance guard, part 2 of 2, and the authoritative one: open the pid
 * file, take an exclusive advisory lock, then record our pid. Must run AFTER
 * the final pid is known (i.e. after fork() in the -d path, since record locks
 * are not inherited across fork). The lock is held for the process lifetime -
 * 'pidFilehandle' must stay open. Returns RE_SUCCESS/RE_ERROR. */
static int daemon_pidfile_write(char *pidfile)
{
    char str[16];

    /* NOT O_TRUNC: truncating before we own the lock would wipe the pid of a
     * running instance. The file is truncated below, once the lock is ours. */
    pidFilehandle = open(pidfile, O_RDWR|O_CREAT, 0600);
    if(pidFilehandle == -1 ) {
       LOG("daemon_pidfile_write()","Could not open PID lock file %s, exiting", pidfile);
       fprintf(stderr,"\nCould not open PID lock file %s, exiting\n",pidfile);
       return RE_ERROR;
    }

    /* Try to lock file - failure means another live instance holds it */
    if(lockf(pidFilehandle,F_TLOCK,0) == -1) {
        int rpid = daemon_pidfile_read(pidfile);

        LOG("daemon_pidfile_write()","RateEngine is already running (PID %d), "
                          "could not lock PID file %s",rpid,pidfile);
        fprintf(stderr,"\nRateEngine is already running (PID %d)! "
                       "Could not lock PID file %s.\n",rpid,pidfile);

        close(pidFilehandle);
        pidFilehandle = 0;

        return RE_ERROR;
    }

    /* The lock is ours - drop any stale pid left by a crashed instance */
    if(ftruncate(pidFilehandle,0) < 0) {
        LOG("daemon_pidfile_write()","Could not truncate PID file %s", pidfile);
        return RE_ERROR;
    }

    /* Get and format PID */
    sprintf(str,"%d\n",getpid());

    /* write pid to lockfile */
    if(write(pidFilehandle, str, strlen(str)) < 0) {
        LOG("daemon_pidfile_write()","Could not write PID to %s", pidfile);
        return RE_ERROR;
    }

    return RE_SUCCESS;
}

/* Foreground service mode (-f): everything daemonize() does except the fork,
 * the setsid() and the /dev/null redirect - stdout/stderr stay attached so a
 * container/systemd sees the process and its output directly. */
int run_foreground(char *rundir, char *pidfile)
{
    if(daemon_chk_running(pidfile)) return RE_ERROR;

    daemon_setup_signals();

    /* change running directory - the pid file path is relative to it */
    if(chdir(rundir) < 0) {
        LOG("run_foreground()","Could not chdir to '%s'",rundir);
        return RE_ERROR;
    }

    if(daemon_pidfile_write(pidfile)) return RE_ERROR;

    return RE_SUCCESS;
}

void daemonize(char *rundir, char *pidfile)
{
    int fd;
    int pid, sid;

    /* NOTE: the old 'if(getppid() == 1) exit(EXIT_FAILURE)' guard used to live
     * here as an "already daemonized" check. It made '-d' impossible inside a
     * container, where the entrypoint IS pid 1, so every child has ppid==1 and
     * the process died silently before writing a single log line. The pid file
     * lock below is the real single-instance guard. */

    if(daemon_chk_running(pidfile)) exit(EXIT_FAILURE);

    daemon_setup_signals();

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
    if(chdir(rundir) < 0) {
        LOG("daemonize()","Could not chdir to '%s'",rundir);
        exit(EXIT_FAILURE);
    }

    /* Ensure only one copy - pid of the child, after the fork */
    if(daemon_pidfile_write(pidfile)) exit(EXIT_FAILURE);
}
