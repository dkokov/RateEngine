#ifndef DAEMON_H
#define DAEMON_H

void daemonShutdown();
void signal_handler(int sig);
void daemonize(char *rundir, char *pidfile);
void stop_daemon(char *pidfile);

#endif
