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
#ifndef __FDUTIL_STDDEF_
#define __FDUTIL_STDDEF_

/** Maximum CPUs */
#define NCPU 4

#ifndef NULL
#define NULL ((void *)0)
#endif // NULL
#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)

/** GCC predefines the types we need for ptrdiff_t and size_t,
   so that we don't have to guess. */
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __SIZE_TYPE__ size_t;

#endif /**< lib/stddef.h */