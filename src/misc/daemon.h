#ifndef DAEMON_H
#define DAEMON_H

void daemonShutdown();
void signal_handler(int sig);
int daemonize(char *rundir, char *pidfile);

#endif
