/* clang-format off */
/* ===-- multi3.c - Implement __multi3 -------------------------------------===
 *
 *                     The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===

__static_yoink("huge_compiler_rt_license");

 * This file implements __multi3 for the compiler_rt library.
 *
 * ===----------------------------------------------------------------------===
 */

#include "third_party/compiler_rt/int_lib.h"

#ifdef CRT_HAS_128BIT

/* Returns: a * b */

static
ti_int
__mulddi3(du_int a, du_int b)
{
    twords r;
    const int bits_in_dword_2 = (int)(sizeof(di_int) * CHAR_BIT) / 2;
    const du_int lower_mask = (du_int)~0 >> bits_in_dword_2;
    r.s.low = (a & lower_mask) * (b & lower_mask);
    du_int t = r.s.low >> bits_in_dword_2;
    r.s.low &= lower_mask;
    t += (a >> bits_in_dword_2) * (b & lower_mask);
    r.s.low += (t & lower_mask) << bits_in_dword_2;
    r.s.high = t >> bits_in_dword_2;
    t = r.s.low >> bits_in_dword_2;
    r.s.low &= lower_mask;
    t += (b >> bits_in_dword_2) * (a & lower_mask);
    r.s.low += (t & lower_mask) << bits_in_dword_2;
    r.s.high += t >> bits_in_dword_2;
    r.s.high += (a >> bits_in_dword_2) * (b >> bits_in_dword_2);
    return r.all;
}

/* Returns: a * b */

COMPILER_RT_ABI ti_int
__multi3(ti_int a, ti_int b)
{
    twords x;
    x.all = a;
    twords y;
    y.all = b;
    twords r;
    r.all = __mulddi3(x.s.low, y.s.low);
    r.s.high += x.s.high * y.s.low + x.s.low * y.s.high;
    return r.all;
}

#endif /* CRT_HAS_128BIT */
