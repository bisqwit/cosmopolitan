// clang-format off
//===----------------------- config_elast.h -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP_CONFIG_ELAST
#define _LIBCPP_CONFIG_ELAST

#include "third_party/libcxx/__config"

#if defined(ELAST)
#define _LIBCPP_ELAST ELAST
#elif defined(_NEWLIB_VERSION)
#define _LIBCPP_ELAST __ELASTERROR
#elif defined(__Fuchsia__)
// No _LIBCPP_ELAST needed on Fuchsia
#elif defined(__wasi__)
// No _LIBCPP_ELAST needed on WASI
#elif defined(__linux__) || defined(_LIBCPP_HAS_MUSL_LIBC) || defined(__COSMOPOLITAN__)
#define _LIBCPP_ELAST 4095
#elif defined(__APPLE__)
// No _LIBCPP_ELAST needed on Apple
#elif defined(__sun__)
#define _LIBCPP_ELAST ESTALE
#elif defined(_LIBCPP_MSVCRT_LIKE)
#define _LIBCPP_ELAST (_sys_nerr - 1)
#else
// Warn here so that the person doing the libcxx port has an easier time:
#warning ELAST for this platform not yet implemented
#endif

#endif // _LIBCPP_CONFIG_ELAST
