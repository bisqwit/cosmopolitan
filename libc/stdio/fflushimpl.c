/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2021 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/
#include "libc/calls/calls.h"
#include "libc/errno.h"
#include "libc/intrin/weaken.h"
#include "libc/mem/mem.h"
#include "libc/runtime/runtime.h"
#include "libc/stdio/internal.h"
#include "libc/stdio/stdio.h"
#include "libc/sysv/consts/o.h"

int __fflush_impl(FILE *f) {
  size_t i;
  ssize_t rc;
  if (f->getln) {
    if (_weaken(free)) {
      _weaken(free)(f->getln);
    }
    f->getln = 0;
  }
  if (f->beg && !f->end && (f->iomode & O_ACCMODE) != O_RDONLY) {
    for (i = 0; i < f->beg; i += rc) {
      if ((rc = write(f->fd, f->buf + i, f->beg - i)) == -1) {
        f->state = errno;
        return -1;
      }
    }
    f->beg = 0;
  }
  return 0;
}
