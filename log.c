#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdlib.h>

#include "log.h"

#define __read_mostly

/* All the verbosity of C with all the complexity of C++ */

/* Console log error messages */

struct log_control {
  int facility;
  int level;
  int warn;
  int error; 
  int info;
  int debug;
  int debug_level;
};

typedef struct log_control log_control_t;

static void *logger_not_avail(int err) {
  fprintf(stderr,"Logger not available now: %d\n", err);
  exit(err);
  return NULL;
}

#define FULL_PRIV_CHECK(a, type) ( (type *) a == NULL ? logger_not_avail(-1) : \
				   ((type *) a)->priv == NULL ?		\
				   logger_not_avail(-1) : ((type *) a)->priv )

#define PRIV_CHECK(a) FULL_PRIV_CHECK(a, logger_t)

static void log_warn (logger_t *self, int err, char *message)
{
  log_control_t *priv = (log_control_t *) PRIV_CHECK(self);
  priv->warn++;
  fprintf(stderr,"WARN: %d: %s\n", err, message);
}

static void log_error (logger_t *self, int err, char *message)
{
  log_control_t *priv = (log_control_t *) PRIV_CHECK(self);
  priv->error++;
  fprintf(stderr,"ERROR: %d: %s\n", err, message);
}

static void log_info (logger_t *self, int err, char *message)
{
  log_control_t *priv = (log_control_t *) PRIV_CHECK(self);
  priv->info++;
  fprintf(stderr,"INFO: %d: %s\n", err, message);
}

static void log_debug (logger_t *self, int level, int err, char *message)
{
  log_control_t *priv = (log_control_t *) PRIV_CHECK(self);
  priv->debug++;
  if(priv->debug_level < level) 
    fprintf(stderr,"DEBUG: %d %s\n", err, message);
}

static void log_init(logger_t *self, const char *name)
{
  if(self->priv == NULL) {
    self->priv = calloc(sizeof(log_control_t),1);
    log_control_t * priv = (log_control_t *) PRIV_CHECK(self);
  }
}

static void log_reset(logger_t *self)
{
}

static void log_destroy(logger_t *self)
{
  if(self != NULL) {
    log_control_t * priv = (log_control_t *) PRIV_CHECK(self);
    if(priv != NULL)
      {
	free(priv);
      } 
  }
}

static void log_change(logger_t *self)
{
}

static void log_dump(logger_t *self)
{
}

static void log_dump_stats(logger_t *self, void *stats)
{
  log_control_t *priv = (log_control_t *) PRIV_CHECK(self);
  fprintf(stderr, "info: %d warn: %d error: %d debug %d messages sent\n", 
	 priv->info, priv->warn, priv->error, priv->debug);
}

logger_t console_logger __read_mostly = {
        .id             =       "std",
        .priv_size      =       sizeof(log_control_t),

        .warn           =       log_warn,
        .error          =       log_error,
	.info           =       log_info,
        .debug          =       log_debug,

        .init           =       log_init,
        .reset          =       log_reset,
        .destroy        =       log_destroy,
        .change         =       log_change,
        .dump           =       log_dump,
        .dump_stats     =       log_dump_stats,
};

/* Syslog version (to make sean happy */


struct syslog_control {
  int facility;
  int level;
  int warn;
  int error; 
  int info;
  int debug;
  int debug_level;
};

typedef struct syslog_control syslog_control_t;

static void syslog_warn (logger_t *self, int err, char *message) {
  syslog_control_t *priv = (syslog_control_t *) PRIV_CHECK(self);
  priv->warn++;
  syslog(LOG_WARNING, "%s\n", message);
}

static void syslog_error (logger_t *self, int err, char *message) {
  syslog_control_t *priv = (syslog_control_t *) PRIV_CHECK(self);
  priv->error++;
  syslog(LOG_ERR, "%s\n", message);
}

static void syslog_info (logger_t *self, int err, char *message) {
  syslog_control_t *priv = (syslog_control_t *) PRIV_CHECK(self);
  priv->info++;
  syslog(LOG_NOTICE, "%s\n", message);
}
static void syslog_debug (logger_t *self, int level, int err, char *message) {
  syslog_control_t *priv = (syslog_control_t *) PRIV_CHECK(self);
  priv->debug++;
  if(priv->debug_level < level) 
    syslog(LOG_DEBUG, "%s\n", message);
}

static void syslog_init(logger_t *self, const char *name)
{
  printf("enabling syslogging");
  openlog(name,LOG_CONS|LOG_NDELAY,LOG_LOCAL6);
  if(self->priv == NULL) {
    self->priv = calloc(sizeof(syslog_control_t),1);
    syslog_control_t * priv = (syslog_control_t *) PRIV_CHECK(self);
    priv->facility = LOG_CONS | LOG_NDELAY;
    priv->level = LOG_LOCAL6;
  }
}

static void syslog_reset(logger_t *self)
{
  syslog_control_t *priv = (syslog_control_t *) PRIV_CHECK(self);
}

static void syslog_destroy(logger_t *self)
{
if(self != NULL) {
    syslog_control_t *priv = (syslog_control_t *) PRIV_CHECK(self);
    if(priv != NULL)
      {
	free(priv);
      } 
}
closelog();
}

static void syslog_change(logger_t *self)
{
}

static void syslog_dump(logger_t *self)
{
}

static void syslog_dump_stats(logger_t *self, void *stats)
{
  syslog_control_t *priv = (syslog_control_t *) PRIV_CHECK(self);
  syslog(LOG_INFO, "info: %d warn: %d error: %d debug %d messages sent", 
	 priv->info, priv->warn, priv->error, priv->debug);
}

logger_t sys_logger __read_mostly = {
        .id             =       "syslog",
        .priv_size      =       sizeof(syslog_control_t),

        .warn           =       syslog_warn,
        .error          =       syslog_error,
	.info           =       syslog_info,
        .debug          =       syslog_debug,

        .init           =       syslog_init,
        .reset          =       syslog_reset,
        .destroy        =       syslog_destroy,
        .change         =       syslog_change,
        .dump           =       syslog_dump,
        .dump_stats     =       syslog_dump_stats,
};

#ifdef TEST

int main() {
logger_t logger = console_logger;
logger.init(&logger,"TEST");
WARN(3,"testing warning");
INFO(3,"testing info");
ERR(3,"testing err");
DEBUG(3,3,"testing debug");
logger.dump_stats(&logger, NULL); // will enhance this later

INFO(3,"Switching to syslog, tail /var/log/syslog to see");

logger.destroy(&logger);
logger = sys_logger;
logger.init(&logger,"TEST");
WARN(3,"testing warning");
INFO(3,"testing info");
ERR(3,"testing err");
DEBUG(3,3,"testing debug");
INFO(3,"Done testing syslog");
logger.destroy(&logger);
logger.dump_stats(&logger, NULL); // will enhance this later


}
#endif
