#ifndef RELOAD_LOGFILE_H
#define RELOAD_LOGFILE_H

ssize_t reload_logfile_fd_read(int sock, void *buf, ssize_t bufsize, int *fd);
ssize_t reload_logfile_fd_write(int sock, void *buf, ssize_t buflen, int fd);
int reload_logfile(time_t res);

#endif
