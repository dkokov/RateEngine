#ifndef DAEMON_H
#define DAEMON_H

void daemonShutdown();
void signal_handler(int sig);

/* background service mode (-d): signals + fork/setsid + stdio to /dev/null
 * + chdir(rundir) + pidfile */
void daemonize(char *rundir, char *pidfile);

/* foreground service mode (-f): signals + chdir(rundir) + pidfile, no fork and
 * stdio kept - for containers/systemd(Type=simple). Returns RE_SUCCESS/RE_ERROR. */
int run_foreground(char *rundir, char *pidfile);

void stop_daemon(char *pidfile);

#endif
