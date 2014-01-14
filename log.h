#ifndef _TWD_LOG_H
#define _TWD_LOG_H

#define handle_error_en(en, msg) \
  do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_warning_en(en, msg) \
  do { errno = en; perror(msg); } while (0)

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

// FIXME need varargs here

#define WARN(err,message) logger.warn(&logger, err, message);
#define INFO(err,message) logger.info(&logger, err, message);
#define ERR(err,message) logger.error(&logger, err, message);
#define DEBUG(level, err, message) logger.debug(&logger, level, err, message);

// log_cb *

typedef struct log_cb {
  char * id;
  int priv_size;
  void (*warn) ( struct log_cb  *self,  int err, char * message);
  void (*info) ( struct log_cb  *self,  int err, char * message);
  void (*error) ( struct log_cb  *self, int err, char * message);
  void (*debug) ( struct log_cb  *self, int level, int err, char * message);

  /* this needs to be thunk out  */

  void (*init) ( struct log_cb  *self, const char *name);
  void (*reset) ( struct log_cb  *self);
  void (*destroy) ( struct log_cb  *self);
  void (*change) ( struct log_cb  *self);
  void  (*dump) ( struct log_cb  *self);
  void  (*dump_stats) ( struct log_cb  *self, void *stats);
  void*  priv;

} logger_t;

// typedef struct log_cb logger_t;

extern logger_t console_logger;
extern logger_t sys_logger;

/* 
extern logger_t json_logger;
extern logger_t twd_logger;
*/

#endif 
