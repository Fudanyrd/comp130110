#pragma once
/** 
 * Citation: large portion of this code is borrowed from 
 * Pintos project of PKU-OS, external link: 
 * <https://github.com/PKU-OS/pintos>
 * 
 * The full copyright is given here.
 *
 *  Copyright (C) 2004, 2005, 2006 Board of Trustees, Leland Stanford
 *  Jr. University.  All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __FDUTIL_STDINT_
#define __FDUTIL_STDINT_

typedef signed char int8_t;
#define INT8_MAX 127
#define INT8_MIN (-INT8_MAX - 1)

typedef signed short int int16_t;
#define INT16_MAX 32767
#define INT16_MIN (-INT16_MAX - 1)

typedef signed int int32_t;
#define INT32_MAX 2147483647
#define INT32_MIN (-INT32_MAX - 1)

typedef signed long long int int64_t;
#define INT64_MAX 9223372036854775807LL
#define INT64_MIN (-INT64_MAX - 1)

typedef unsigned char uint8_t;
#define UINT8_MAX 255

typedef unsigned short int uint16_t;
#define UINT16_MAX 65535

typedef unsigned int uint32_t;
#define UINT32_MAX 4294967295U

typedef unsigned long long int uint64_t;
#define UINT64_MAX 18446744073709551615ULL

typedef int64_t intptr_t;
#define INTPTR_MIN INT64_MIN
#define INTPTR_MAX INT64_MAX

typedef uint64_t uintptr_t;
#define UINTPTR_MAX UINT64_MAX

typedef int64_t intmax_t;
#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX

typedef uint64_t uintmax_t;
#define UINTMAX_MAX UINT64_MAX

#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX

#define SIZE_MAX UINT32_MAX

#endif /**< fdutil/stdint.h */
