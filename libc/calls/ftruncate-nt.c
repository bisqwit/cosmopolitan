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
#include "libc/calls/syscall_support-nt.internal.h"
#include "libc/nt/enum/filemovemethod.h"
#include "libc/nt/errors.h"
#include "libc/nt/files.h"
#include "libc/nt/runtime.h"
#include "libc/sysv/errfuns.h"

textwindows int sys_ftruncate_nt(int64_t handle, uint64_t length) {
  bool32 ok;
  int64_t tell;
  tell = -1;
  if ((ok = SetFilePointerEx(handle, 0, &tell, kNtFileCurrent))) {
    ok = SetFilePointerEx(handle, length, NULL, kNtFileBegin) &&
         SetEndOfFile(handle);
    npassert(SetFilePointerEx(handle, tell, NULL, kNtFileBegin));
  }
  if (ok) {
    return 0;
  } else if (GetLastError() == kNtErrorAccessDenied) {
    return einval();  // ftruncate() doesn't raise EACCES
  } else {
    return __winerr();
  }
}
