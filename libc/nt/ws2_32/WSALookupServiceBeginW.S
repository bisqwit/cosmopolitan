#include "libc/nt/codegen.h"
.imp	ws2_32,__imp_WSALookupServiceBeginW,WSALookupServiceBeginW

	.text.windows
        .ftrace1
WSALookupServiceBegin:
        .ftrace2
#ifdef __x86_64__
	push	%rbp
	mov	%rsp,%rbp
	mov	__imp_WSALookupServiceBeginW(%rip),%rax
	jmp	__sysv2nt
#elif defined(__aarch64__)
	mov	x0,#0
	ret
#endif
	.endfn	WSALookupServiceBegin,globl
	.previous
