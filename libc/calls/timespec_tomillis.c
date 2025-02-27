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
#include "libc/calls/struct/timespec.h"
#include "libc/limits.h"
#include "libc/stdckdint.h"

/**
 * Reduces `ts` from 1e-9 to 1e-3 granularity w/ ceil rounding.
 *
 * This function uses ceiling rounding. For example, if `ts` is one
 * nanosecond, then one millisecond will be returned. Ceil rounding
 * is needed by many interfaces, e.g. setitimer(), because the zero
 * timestamp has a special meaning.
 *
 * This function also detects overflow in which case `INT64_MAX` or
 * `INT64_MIN` may be returned. The `errno` variable isn't changed.
 *
 * @return 64-bit scalar milliseconds since epoch
 */
int64_t timespec_tomillis(struct timespec ts) {
  int64_t ms;
  // reduce precision from nanos to millis
  if (ts.tv_nsec <= 999000000) {
    ts.tv_nsec = (ts.tv_nsec + 999999) / 1000000;
  } else {
    ts.tv_nsec = 0;
    if (ts.tv_sec < INT64_MAX) {
      ts.tv_sec += 1;
    }
  }
  // convert to scalar result
  if (!ckd_mul(&ms, ts.tv_sec, 1000ul) && !ckd_add(&ms, ms, ts.tv_nsec)) {
    return ms;
  } else if (ts.tv_sec < 0) {
    return INT64_MIN;
  } else {
    return INT64_MAX;
  }
}
