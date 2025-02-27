#ifndef COSMOPOLITAN_LIBC_SYSV_CONSTS_AT_H_
#define COSMOPOLITAN_LIBC_SYSV_CONSTS_AT_H_
#if !(__ASSEMBLER__ + __LINKER__ + 0)
COSMOPOLITAN_C_START_

/**
 * @fileoverview AT_xxx constants for fcntl(), fopenat(), etc..
 * @see libc/sysv/consts/auxv.h for getauxval() constants
 */

extern const int AT_FDCWD;
extern const int AT_SYMLINK_FOLLOW;
extern const int AT_SYMLINK_NOFOLLOW;
extern const int AT_REMOVEDIR;
extern const int AT_EACCESS;
extern const int AT_EMPTY_PATH;

COSMOPOLITAN_C_END_
#endif /* !(__ASSEMBLER__ + __LINKER__ + 0) */

#define AT_FDCWD            AT_FDCWD
#define AT_SYMLINK_FOLLOW   AT_SYMLINK_FOLLOW
#define AT_SYMLINK_NOFOLLOW AT_SYMLINK_NOFOLLOW
#define AT_REMOVEDIR        AT_REMOVEDIR

#endif /* COSMOPOLITAN_LIBC_SYSV_CONSTS_AT_H_ */
