#include <syslog.h>

void open_syslog(void)
{
  setlogmask (LOG_UPTO (LOG_NOTICE));   
  openlog ("RateEngine", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}

void close_syslog(void)
{
  closelog ();
}
