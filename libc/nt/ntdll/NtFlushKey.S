#include "libc/nt/ntdllimport.h"
.ntimp	NtFlushKey,NtFlushKey

	.text.windows
        .ftrace1
NtFlushKey:
        .ftrace2
#ifdef __x86_64__
	push	%rbp
	mov	%rsp,%rbp
	mov	%rdi,%rcx
	sub	$32,%rsp
	call	*__imp_NtFlushKey(%rip)
	leave
#elif defined(__aarch64__)
	mov	x0,#0
#endif
	ret
	.endfn	NtFlushKey,globl
	.previous
