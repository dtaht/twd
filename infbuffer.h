
#ifndef INFBUFFER_H
#define INFBUFFER_H

typedef struct
{
  void            **data;
  volatile size_t   ridx;
  volatile size_t   widx;
  volatile size_t   tidx;
} infbuffer__s;

int	infbuffer_init		(infbuffer__s *,size_t);
void	infbuffer_add		(infbuffer__s *,void *);
void	infbuffer_rem		(infbuffer__s *,void **);
void	infbuffer_destroy	(infbuffer__s *);

#endif
