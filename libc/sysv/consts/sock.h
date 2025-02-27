#ifndef COSMOPOLITAN_LIBC_SYSV_CONSTS_SOCK_H_
#define COSMOPOLITAN_LIBC_SYSV_CONSTS_SOCK_H_
#if !(__ASSEMBLER__ + __LINKER__ + 0)
COSMOPOLITAN_C_START_

extern const int SOCK_CLOEXEC;
extern const int SOCK_DCCP;
extern const int SOCK_DGRAM;
extern const int SOCK_NONBLOCK;
extern const int SOCK_PACKET;
extern const int SOCK_RAW;
extern const int SOCK_RDM;
extern const int SOCK_SEQPACKET;
extern const int SOCK_STREAM;

#define SOCK_STREAM    1
#define SOCK_DGRAM     2
#define SOCK_RAW       3
#define SOCK_RDM       4
#define SOCK_SEQPACKET 5

COSMOPOLITAN_C_END_
#endif /* !(__ASSEMBLER__ + __LINKER__ + 0) */
#endif /* COSMOPOLITAN_LIBC_SYSV_CONSTS_SOCK_H_ */
