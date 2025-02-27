/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2022 Justine Alexandra Roberts Tunney                              │
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
#include "libc/fmt/fmt.h"
#include "libc/fmt/magnumstrs.internal.h"
#include "libc/intrin/dos2errno.internal.h"
#include "libc/intrin/kprintf.h"
#include "libc/str/str.h"

// note: these are supplementary errno magnum mappings
//       don't include the ones in libc/sysv/consts.sh

int main(int argc, char *argv[]) {
  int i;
  for (i = 0; kDos2Errno[i].doscode; ++i) {
    kprintf("dos error %10hu maps to rva %10d errno %10d which is %s%n",
            kDos2Errno[i].doscode, kDos2Errno[i].systemv,
            *(const int *)((intptr_t)kDos2Errno + kDos2Errno[i].systemv),
            _strerrno(
                *(const int *)((intptr_t)kDos2Errno + kDos2Errno[i].systemv)));
  }
  return 0;
}
