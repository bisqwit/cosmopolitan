/*-*- mode:unix-assembly; indent-tabs-mode:t; tab-width:8; coding:utf-8     -*-│
│vi: set et ft=asm ts=8 tw=8 fenc=utf-8                                     :vi│
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
#include "libc/macros.internal.h"

//	Saves caller CPU state to cacheline.
//
//	@param	rdi points to jmp_buf
//	@return	eax contains 0 when set, and 1 if jumped
//	@return	rdx contains value passed to longerjmp()
//	@returnstwice
	.ftrace1
setlongerjmp:
	.ftrace2
#ifdef __x86_64__
	lea	8(%rsp),%rax
	mov	%rax,(%rdi)
	mov	%rbx,8(%rdi)
	mov	%rbp,16(%rdi)
	mov	%r12,24(%rdi)
	mov	%r13,32(%rdi)
	mov	%r14,40(%rdi)
	mov	%r15,48(%rdi)
	mov	(%rsp),%rax
	mov	%rax,56(%rdi)
	xor	%eax,%eax
	xor	%edx,%edx
	ret
#elif defined(__aarch64__)
	stp	x19,x20,[x0,#0]
	stp	x21,x22,[x0,#16]
	stp	x23,x24,[x0,#32]
	stp	x25,x26,[x0,#48]
	stp	x27,x28,[x0,#64]
	stp	x29,x30,[x0,#80]
	mov	x2,sp
	str	x2,[x0,#104]
	stp	d8,d9,[x0,#112]
	stp	d10,d11,[x0,#128]
	stp	d12,d13,[x0,#144]
	stp	d14,d15,[x0,#160]
	mov	x0,#0
	mov	x1,#0
	ret
#else
#error "unsupported architecture"
#endif
	.endfn	setlongerjmp,globl
