/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2023 Justine Alexandra Roberts Tunney                              │
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
#include "libc/runtime/runtime.h"
#include "libc/sysv/consts/map.h"
#include "libc/sysv/consts/msync.h"
#include "libc/sysv/consts/prot.h"
#include "libc/testlib/testlib.h"

TEST(msync, changeFileMappingAndExit) {
  int ws;
  char byte;
  char *map;
  ASSERT_SYS(0, 3, tmpfd());
  ASSERT_SYS(0, 0, ftruncate(3, 4));
  ASSERT_NE(MAP_FAILED,
            (map = mmap(0, 4, PROT_READ | PROT_WRITE, MAP_SHARED, 3, 0)));
  if (!fork()) {
    map[0] = 1;
    ASSERT_SYS(0, 0, msync(map, 4, MS_SYNC));
    _Exit(123);
  }
  ASSERT_NE(-1, wait(&ws));
  ASSERT_TRUE(WIFEXITED(ws));
  ASSERT_EQ(123, WEXITSTATUS(ws));
  ASSERT_SYS(0, 1, pread(3, &byte, 1, 0));
  ASSERT_EQ(1, byte);
  ASSERT_EQ(1, map[0]);
  ASSERT_SYS(0, 0, munmap(map, 4));
  ASSERT_SYS(0, 0, close(3));
}
