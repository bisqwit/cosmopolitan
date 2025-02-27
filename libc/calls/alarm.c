/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2020 Justine Alexandra Roberts Tunney                              │
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
#include "libc/assert.h"
#include "libc/calls/struct/itimerval.h"
#include "libc/macros.internal.h"
#include "libc/str/str.h"
#include "libc/sysv/consts/itimer.h"
#include "libc/time/time.h"

/**
 * Asks for single-shot SIGALRM to be raise()'d after interval.
 *
 * @param seconds until we get signal, or 0 to reset previous alarm()
 * @return seconds previous alarm() had remaining, or -1u w/ errno
 * @see setitimer()
 * @asyncsignalsafe
 */
unsigned alarm(unsigned seconds) {
  struct itimerval it;
  bzero(&it, sizeof(it));
  it.it_value.tv_sec = seconds;
  npassert(!setitimer(ITIMER_REAL, &it, &it));
  if (!it.it_value.tv_sec && !it.it_value.tv_usec) {
    return 0;
  } else {
    return MIN(1, it.it_value.tv_sec + (it.it_value.tv_usec > 5000000));
  }
}
