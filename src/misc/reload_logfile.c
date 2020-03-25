#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#include "re5_fstat.h"
#include "globals.h"

/* 
 * https://01.org/linuxgraphics/gfx-docs/drm/driver-api/dma-buf.html
 *  
 * https://keithp.com/blogs/fd-passing/ 
 * https://github.com/keith-packard/fdpassing
 * */

ssize_t reload_logfile_fd_write(int sock, void *buf, ssize_t buflen, int fd)
{
	int pid;
    ssize_t     size;
    struct msghdr   msg;
    struct iovec    iov;
    union {
        struct cmsghdr  cmsghdr;
        char        control[CMSG_SPACE(sizeof (int))];
    } cmsgu;
    struct cmsghdr  *cmsg;

	size = 0;
	pid = getpid();

    iov.iov_base = buf;
    iov.iov_len = buflen;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (fd != -1) {
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof (int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        DBG2("reload_logfile_fd_write()","passing fd %d,PID:%d",fd,pid);
        *((int *) CMSG_DATA(cmsg)) = fd;
    } else {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        DBG2("reload_logfile_fd_write()","not passing fd,PID:%d",pid);
    }

    size = sendmsg(sock, &msg, 0);

    if(size < 0)
		DBG2("reload_logfile_fd_write()","sendmsg,PID:%d",pid);
    
    return size;
}

ssize_t reload_logfile_fd_read(int sock, void *buf, ssize_t bufsize, int *fd)
{
	int pid;
    ssize_t  size;
    struct timeval timeout;
    
    size = 0;
    pid = getpid();

    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;
	
	if(fd) {
        struct msghdr   msg;
        struct iovec    iov;
        union {
            struct cmsghdr  cmsghdr;
            char        control[CMSG_SPACE(sizeof (int))];
        } cmsgu;
        struct cmsghdr  *cmsg;

        iov.iov_base = buf;
        iov.iov_len = bufsize;

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);

    	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0)
        	DBG2("reload_logfile_fd_read()","setsockopt failed,PID:%d",pid);
                
        size = recvmsg (sock, &msg, 0);
        if (size < 0) {
            DBG2("reload_logfile_fd_read()","recvmsg,PID:%d",pid);
            return 0;
        }
        
        cmsg = CMSG_FIRSTHDR(&msg);
        if(cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
            if(cmsg->cmsg_level != SOL_SOCKET) {
                DBG2("reload_logfile_fd_read()","invalid cmsg_level %d,PID:%d",cmsg->cmsg_level,pid);
                return 0;
            }
            
            if(cmsg->cmsg_type != SCM_RIGHTS) {
                DBG2("reload_logfile_fd_read()","invalid cmsg_type %d,PID:%d",cmsg->cmsg_type,pid);
                return 0;
            }

            *fd = *((int *) CMSG_DATA(cmsg));
            DBG2("reload_logfile_fd_read()","received fd %d,PID:%d",*fd,pid);
        } else *fd = -1;
    } else {
        size = read (sock, buf, bufsize);
        if(size < 0) {
            DBG2("reload_logfile_fd_read()","read,PID:%d",pid);
            return 0;
        }
    }
    
    return size;
}


int reload_logfile(time_t res)
{
	char new_fn[256];
	struct tm *tt;
	unsigned int filesize;
	
	tt = localtime(&res);
		
	filesize = re5_fstat(config.log_filename);
	if(filesize >= log_max_file_size) {				
		sprintf(new_fn,"%s.%.4d%.2d%.2d%.2d%.2d%.2d",config.log_filename,
										 (tt->tm_year+1900),(tt->tm_mon+1),(tt->tm_mday),
										 (tt->tm_hour),(tt->tm_min),(tt->tm_sec));
		re_reload_syslog(config.log,config.log_filename,new_fn);
			
		LOG("reload_logfile()","A syslog is reloaded(%s)!",new_fn);
	
		return RE_ERROR;
	}
	
	return RE_SUCCESS;
}
