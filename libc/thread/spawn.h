#ifndef COSMOPOLITAN_LIBC_THREAD_SPAWN_H_
#define COSMOPOLITAN_LIBC_THREAD_SPAWN_H_
#include "libc/thread/tls.h"
#if !(__ASSEMBLER__ + __LINKER__ + 0)
COSMOPOLITAN_C_START_

struct spawn {
  int ptid;
  char *stk;
  char *tls;
  struct CosmoTib *tib;
};

int _spawn(int (*)(void *, int), void *, struct spawn *);
int _join(struct spawn *);
char *_mktls(struct CosmoTib **);

COSMOPOLITAN_C_END_
#endif /* !(__ASSEMBLER__ + __LINKER__ + 0) */
#endif /* COSMOPOLITAN_LIBC_THREAD_SPAWN_H_ */
