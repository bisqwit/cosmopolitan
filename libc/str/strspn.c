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
#include "libc/str/str.h"

/**
 * Returns prefix length, consisting of chars in accept.
 *
 * @param accept is nul-terminated character set
 * @see strcspn(), strtok_r()
 * @asyncsignalsafe
 */
size_t strspn(const char *s, const char *accept) {
  int c;
  size_t i;
  bool lut[256];
#ifndef TINY
  if (!accept[0]) {
    return 0;
  }
  if (!accept[1]) {
    for (i = 0;; i++) {
      if (s[i] != accept[0]) {
        return i;
      }
    }
  }
#endif
  bzero(lut, sizeof(lut));
  while ((c = *accept++ & 255)) {
    lut[c] = true;
  }
  for (i = 0;; i++) {
    if (!lut[s[i] & 255]) {
      return i;
    }
  }
}
