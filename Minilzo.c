/* minilzo.c -- mini subset of the LZO real-time data compression library

   This file is part of the LZO real-time data compression library.

   Copyright (C) 2005 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2004 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2003 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2002 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2001 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2000 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1999 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1998 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1997 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The LZO library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License,
   version 2, as published by the Free Software Foundation.

   The LZO library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the LZO library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/lzo/
 */

/*
 * NOTE:
 *   the full LZO package can be found at
 *   http://www.oberhumer.com/opensource/lzo/
 */

#define __LZO_IN_MINILZO
#define LZO_BUILD

#if defined(LZO_CFG_FREESTANDING)
#  undef MINILZO_HAVE_CONFIG_H
#  define LZO_LIBC_FREESTANDING 1
#  define LZO_OS_FREESTANDING 1
#endif

#ifdef MINILZO_HAVE_CONFIG_H
#  include <config.h>
#endif
#include <limits.h>
#include <stddef.h>

#undef LZO_HAVE_CONFIG_H
#include "Minilzo.h"

#if !defined(MINILZO_VERSION) || (MINILZO_VERSION != 0x2020)
#  error "version mismatch in miniLZO source files"
#endif

#ifdef MINILZO_HAVE_CONFIG_H
#  define LZO_HAVE_CONFIG_H
#endif

#ifndef __LZO_CONF_H
#define __LZO_CONF_H

#if !defined(__LZO_IN_MINILZO)
#if defined(LZO_CFG_FREESTANDING)
#  define LZO_LIBC_FREESTANDING 1
#  define LZO_OS_FREESTANDING 1
#  define ACC_LIBC_FREESTANDING 1
#  define ACC_OS_FREESTANDING 1
#endif
#if defined(LZO_CFG_NO_UNALIGNED)
#  define ACC_CFG_NO_UNALIGNED 1
#endif
#if defined(LZO_HAVE_CONFIG_H)
#  define ACC_CONFIG_NO_HEADER 1
#endif
#if defined(__LZOCONF_H) || defined(__LZOCONF_H_INCLUDED)
#  error "include this file first"
#endif
#include "lzo/Lzoconf.h"
#endif

#if (LZO_VERSION < 0x02000) || !defined(__LZOCONF_H_INCLUDED)
#  error "version mismatch"
#endif

#if (LZO_CC_BORLANDC && LZO_ARCH_I086)
#  pragma option -h
#endif

#if (LZO_CC_MSC && (_MSC_VER >= 1000))
#  pragma warning(disable: 4127 4701)
#endif
#if (LZO_CC_MSC && (_MSC_VER >= 1300))
#  pragma warning(disable: 4820)
#  pragma warning(disable: 4514 4710 4711)
#endif

#if defined(__LZO_MMODEL_HUGE) && (!LZO_HAVE_MM_HUGE_PTR)
#  error "this should not happen - check defines for __huge"
#endif

#if defined(__LZO_IN_MINILZO) || defined(LZO_CFG_FREESTANDING)
#elif (LZO_OS_DOS16 || LZO_OS_OS216 || LZO_OS_WIN16)
#  define ACC_WANT_ACC_INCD_H 1
#  define ACC_WANT_ACC_INCE_H 1
#  define ACC_WANT_ACC_INCI_H 1
#elif 1
#  include <string.h>
#else
#  define ACC_WANT_ACC_INCD_H 1
#endif

#if (LZO_ARCH_I086)
#  define ACC_MM_AHSHIFT        LZO_MM_AHSHIFT
#  define ACC_PTR_FP_OFF(x)     (((const unsigned __far*)&(x))[0])
#  define ACC_PTR_FP_SEG(x)     (((const unsigned __far*)&(x))[1])
#  define ACC_PTR_MK_FP(s,o)    ((void __far*)(((unsigned long)(s)<<16)+(unsigned)(o)))
#endif

#if !defined(lzo_uintptr_t)
#  if defined(__LZO_MMODEL_HUGE)
#    define lzo_uintptr_t       unsigned long
#  elif 1 && defined(LZO_OS_OS400) && (LZO_SIZEOF_VOID_P == 16)
#    define __LZO_UINTPTR_T_IS_POINTER 1
     typedef char*              lzo_uintptr_t;
#    define lzo_uintptr_t       lzo_uintptr_t
#  elif (LZO_SIZEOF_SIZE_T == LZO_SIZEOF_VOID_P)
#    define lzo_uintptr_t       size_t
#  elif (LZO_SIZEOF_LONG == LZO_SIZEOF_VOID_P)
#    define lzo_uintptr_t       unsigned long
#  elif (LZO_SIZEOF_INT == LZO_SIZEOF_VOID_P)
#    define lzo_uintptr_t       unsigned int
#  elif (LZO_SIZEOF_LONG_LONG == LZO_SIZEOF_VOID_P)
#    define lzo_uintptr_t       unsigned long long
#  else
#    define lzo_uintptr_t       size_t
#  endif
#endif
LZO_COMPILE_TIME_ASSERT_HEADER(sizeof(lzo_uintptr_t) >= sizeof(lzo_voidp))

#if 1 && !defined(LZO_CFG_FREESTANDING)
#if 1 && !defined(HAVE_STRING_H)
#define HAVE_STRING_H 1
#endif
#if 1 && !defined(HAVE_MEMCMP)
#define HAVE_MEMCMP 1
#endif
#if 1 && !defined(HAVE_MEMCPY)
#define HAVE_MEMCPY 1
#endif
#if 1 && !defined(HAVE_MEMMOVE)
#define HAVE_MEMMOVE 1
#endif
#if 1 && !defined(HAVE_MEMSET)
#define HAVE_MEMSET 1
#endif
#endif

#if 1 && defined(HAVE_STRING_H)
#include <string.h>
#endif

#if defined(LZO_CFG_FREESTANDING)
#  undef HAVE_MEMCMP
#  undef HAVE_MEMCPY
#  undef HAVE_MEMMOVE
#  undef HAVE_MEMSET
#endif

#if !defined(HAVE_MEMCMP)
#  undef memcmp
#  define memcmp(a,b,c)         lzo_memcmp(a,b,c)
#elif !defined(__LZO_MMODEL_HUGE)
#  define lzo_memcmp(a,b,c)     memcmp(a,b,c)
#endif
#if !defined(HAVE_MEMCPY)
#  undef memcpy
#  define memcpy(a,b,c)         lzo_memcpy(a,b,c)
#elif !defined(__LZO_MMODEL_HUGE)
#  define lzo_memcpy(a,b,c)     memcpy(a,b,c)
#endif
#if !defined(HAVE_MEMMOVE)
#  undef memmove
#  define memmove(a,b,c)        lzo_memmove(a,b,c)
#elif !defined(__LZO_MMODEL_HUGE)
#  define lzo_memmove(a,b,c)    memmove(a,b,c)
#endif
#if !defined(HAVE_MEMSET)
#  undef memset
#  define memset(a,b,c)         lzo_memset(a,b,c)
#elif !defined(__LZO_MMODEL_HUGE)
#  define lzo_memset(a,b,c)     memset(a,b,c)
#endif

#undef NDEBUG
#if defined(LZO_CFG_FREESTANDING)
#  undef LZO_DEBUG
#  define NDEBUG 1
#  undef assert
#  define assert(e) ((void)0)
#else
#  if !defined(LZO_DEBUG)
#    define NDEBUG 1
#  endif
#  include <assert.h>
#endif

#if 0 && defined(__BOUNDS_CHECKING_ON)
#  include <unchecked.h>
#else
#  define BOUNDS_CHECKING_OFF_DURING(stmt)      stmt
#  define BOUNDS_CHECKING_OFF_IN_EXPR(expr)     (expr)
#endif

#if !defined(__lzo_inline)
#  define __lzo_inline
#endif
#if !defined(__lzo_forceinline)
#  define __lzo_forceinline
#endif
#if !defined(__lzo_noinline)
#  define __lzo_noinline
#endif

#if 1
#  define LZO_BYTE(x)       ((unsigned char) (x))
#else
#  define LZO_BYTE(x)       ((unsigned char) ((x) & 0xff))
#endif

#define LZO_MAX(a,b)        ((a) >= (b) ? (a) : (b))
#define LZO_MIN(a,b)        ((a) <= (b) ? (a) : (b))
#define LZO_MAX3(a,b,c)     ((a) >= (b) ? LZO_MAX(a,c) : LZO_MAX(b,c))
#define LZO_MIN3(a,b,c)     ((a) <= (b) ? LZO_MIN(a,c) : LZO_MIN(b,c))

#define lzo_sizeof(type)    ((lzo_uint) (sizeof(type)))

#define LZO_HIGH(array)     ((lzo_uint) (sizeof(array)/sizeof(*(array))))

#define LZO_SIZE(bits)      (1u << (bits))
#define LZO_MASK(bits)      (LZO_SIZE(bits) - 1)

#define LZO_LSIZE(bits)     (1ul << (bits))
#define LZO_LMASK(bits)     (LZO_LSIZE(bits) - 1)

#define LZO_USIZE(bits)     ((lzo_uint) 1 << (bits))
#define LZO_UMASK(bits)     (LZO_USIZE(bits) - 1)

#if !defined(DMUL)
#if 0

#  define DMUL(a,b) ((lzo_xint) ((lzo_uint32)(a) * (lzo_uint32)(b)))
#else
#  define DMUL(a,b) ((lzo_xint) ((a) * (b)))
#endif
#endif

#if 1 && !defined(LZO_CFG_NO_UNALIGNED)
#if 1 && (LZO_ARCH_AMD64 || LZO_ARCH_I386)
#  if (LZO_SIZEOF_SHORT == 2)
#    define LZO_UNALIGNED_OK_2
#  endif
#  if (LZO_SIZEOF_INT == 4)
#    define LZO_UNALIGNED_OK_4
#  endif
#endif
#endif

#if defined(LZO_UNALIGNED_OK_2)
  LZO_COMPILE_TIME_ASSERT_HEADER(sizeof(short) == 2)
#endif
#if defined(LZO_UNALIGNED_OK_4)
  LZO_COMPILE_TIME_ASSERT_HEADER(sizeof(lzo_uint32) == 4)
#elif defined(LZO_ALIGNED_OK_4)
  LZO_COMPILE_TIME_ASSERT_HEADER(sizeof(lzo_uint32) == 4)
#endif

#define MEMCPY8_DS(dest,src,len) \
    lzo_memcpy(dest,src,len); dest += len; src += len

#define BZERO8_PTR(s,l,n) \
    lzo_memset((lzo_voidp)(s),0,(lzo_uint)(l)*(n))

#define MEMCPY_DS(dest,src,len) \
    do *dest++ = *src++; while (--len > 0)

__LZO_EXTERN_C int __lzo_init_done;
__LZO_EXTERN_C const char __lzo_copyright[];
LZO_EXTERN(const lzo_bytep) lzo_copyright(void);

#ifndef __LZO_PTR_H
#define __LZO_PTR_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(lzo_uintptr_t)
#  if defined(__LZO_MMODEL_HUGE)
#    define lzo_uintptr_t   unsigned long
#  else
#    define lzo_uintptr_t   acc_uintptr_t
#    ifdef __ACC_INTPTR_T_IS_POINTER
#      define __LZO_UINTPTR_T_IS_POINTER 1
#    endif
#  endif
#endif

#if (LZO_ARCH_I086)
#define PTR(a)              ((lzo_bytep) (a))
#define PTR_ALIGNED_4(a)    ((ACC_PTR_FP_OFF(a) & 3) == 0)
#define PTR_ALIGNED2_4(a,b) (((ACC_PTR_FP_OFF(a) | ACC_PTR_FP_OFF(b)) & 3) == 0)
#else
#define PTR(a)              ((lzo_uintptr_t) (a))
#define PTR_LINEAR(a)       PTR(a)
#define PTR_ALIGNED_4(a)    ((PTR_LINEAR(a) & 3) == 0)
#define PTR_ALIGNED_8(a)    ((PTR_LINEAR(a) & 7) == 0)
#define PTR_ALIGNED2_4(a,b) (((PTR_LINEAR(a) | PTR_LINEAR(b)) & 3) == 0)
#define PTR_ALIGNED2_8(a,b) (((PTR_LINEAR(a) | PTR_LINEAR(b)) & 7) == 0)
#endif

#define PTR_LT(a,b)         (PTR(a) < PTR(b))
#define PTR_GE(a,b)         (PTR(a) >= PTR(b))
#define PTR_DIFF(a,b)       (PTR(a) - PTR(b))
#define pd(a,b)             ((lzo_uint) ((a)-(b)))

LZO_EXTERN(lzo_uintptr_t)
__lzo_ptr_linear(const lzo_voidp ptr);

typedef union
{
    char            a_char;
    unsigned char   a_uchar;
    short           a_short;
    unsigned short  a_ushort;
    int             a_int;
    unsigned int    a_uint;
    long            a_long;
    unsigned long   a_ulong;
    lzo_int         a_lzo_int;
    lzo_uint        a_lzo_uint;
    lzo_int32       a_lzo_int32;
    lzo_uint32      a_lzo_uint32;
    ptrdiff_t       a_ptrdiff_t;
    lzo_uintptr_t   a_lzo_uintptr_t;
    lzo_voidp       a_lzo_voidp;
    void*           a_void_p;
    lzo_bytep       a_lzo_bytep;
    lzo_bytepp      a_lzo_bytepp;
    lzo_uintp       a_lzo_uintp;
    lzo_uint *      a_lzo_uint_p;
    lzo_uint32p     a_lzo_uint32p;
    lzo_uint32*     a_lzo_uint32_p;
    unsigned char*  a_uchar_p;
    char*           a_char_p;
}
lzo_full_align_t;

#ifdef __cplusplus
}
#endif

#endif

#define LZO_DETERMINISTIC

#define LZO_DICT_USE_PTR
#if 0 && (LZO_ARCH_I086)
#  undef LZO_DICT_USE_PTR
#endif

#if defined(LZO_DICT_USE_PTR)
#  define lzo_dict_t    const lzo_bytep
#  define lzo_dict_p    lzo_dict_t __LZO_MMODEL *
#else
#  define lzo_dict_t    lzo_uint
#  define lzo_dict_p    lzo_dict_t __LZO_MMODEL *
#endif

#endif

#if !defined(MINILZO_CFG_SKIP_LZO_PTR)

LZO_PUBLIC(lzo_uintptr_t)
__lzo_ptr_linear(const lzo_voidp ptr)
{
    lzo_uintptr_t p;

#if (LZO_ARCH_I086)
    p = (((lzo_uintptr_t)(ACC_PTR_FP_SEG(ptr))) << (16 - ACC_MM_AHSHIFT)) + (ACC_PTR_FP_OFF(ptr));
#else
    p = (lzo_uintptr_t) PTR_LINEAR(ptr);
#endif

    return (p);
}

LZO_PUBLIC(unsigned)
__lzo_align_gap(const lzo_voidp ptr, lzo_uint size)
{
#if defined(__LZO_UINTPTR_T_IS_POINTER)
    size_t n = (size_t) ptr;
    n = (((n + size - 1) / size) * size) - n;
#else
    lzo_uintptr_t p, n;
    p = __lzo_ptr_linear(ptr);
    n = (((p + size - 1) / size) * size) - p;
#endif

    assert(size > 0);
    assert((long)n >= 0);
    assert(n <= s);
    return (unsigned)n;
}

#endif

/* If you use the LZO library in a product, you *must* keep this
 * copyright string in the executable of your product.
 */

const char __lzo_copyright[] =
#if !defined(__LZO_IN_MINLZO)
    LZO_VERSION_STRING;
#else
    "\r\n\n"
    "LZO data compression library.\n"
    "$Copyright: LZO (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005 Markus Franz Xaver Johannes Oberhumer\n"
    "<markus@oberhumer.com>\n"
    "http://www.oberhumer.com $\n\n"
    "$Id: minilzo.c,v 1.3 2006/03/16 23:34:38 jpeterson Exp $\n"
    "$Built: " __DATE__ " " __TIME__ " $\n"
    "$Info: " LZO_INFO_STRING " $\n";
#endif

LZO_PUBLIC(const lzo_bytep)
lzo_copyright(void)
{
#if (LZO_OS_DOS16 && LZO_CC_TURBOC)
    return (lzo_voidp) __lzo_copyright;
#else
    return (const lzo_bytep) __lzo_copyright;
#endif
}

LZO_PUBLIC(unsigned)
lzo_version(void)
{
    return (LZO_VERSION);
}

LZO_PUBLIC(const char *)
lzo_version_string(void)
{
    return (LZO_VERSION_STRING);
}

LZO_PUBLIC(const char *)
lzo_version_date(void)
{
    return (LZO_VERSION_DATE);
}

LZO_PUBLIC(const lzo_charp)
_lzo_version_string(void)
{
    return (LZO_VERSION_STRING);
}

LZO_PUBLIC(const lzo_charp)
_lzo_version_date(void)
{
    return (LZO_VERSION_DATE);
}

#define LZO_BASE 65521u
#define LZO_NMAX 5552

#define LZO_DO1(buf,i)  s1 += buf[i]; s2 += s1
#define LZO_DO2(buf,i)  LZO_DO1(buf,i); LZO_DO1(buf,i+1);
#define LZO_DO4(buf,i)  LZO_DO2(buf,i); LZO_DO2(buf,i+2);
#define LZO_DO8(buf,i)  LZO_DO4(buf,i); LZO_DO4(buf,i+4);
#define LZO_DO16(buf,i) LZO_DO8(buf,i); LZO_DO8(buf,i+8);

LZO_PUBLIC(lzo_uint32)
lzo_adler32(lzo_uint32 adler, const lzo_bytep buf, lzo_uint len)
{
    lzo_uint32 s1 = adler & 0xffff;
    lzo_uint32 s2 = (adler >> 16) & 0xffff;
    unsigned k;

    if (buf == NULL)
        return (1);

    while (len > 0)
    {
        k = len < LZO_NMAX ? (unsigned) len : LZO_NMAX;
        len -= k;
        if (k >= 16) do
        {
            LZO_DO16(buf,0);
            buf += 16;
            k -= 16;
        } while (k >= 16);
        if (k != 0) do
        {
            s1 += *buf++;
            s2 += s1;
        } while (--k > 0);
        s1 %= LZO_BASE;
        s2 %= LZO_BASE;
    }
    return (s2 << 16) | s1;
}

#undef LZO_DO1
#undef LZO_DO2
#undef LZO_DO4
#undef LZO_DO8
#undef LZO_DO16

#if !defined(MINILZO_CFG_SKIP_LZO_STRING)
#undef lzo_memcmp
#undef lzo_memcpy
#undef lzo_memmove
#undef lzo_memset
#if !defined(__LZO_MMODEL_HUGE)
#  undef LZO_HAVE_MM_HUGE_PTR
#endif
#define lzo_hsize_t             lzo_uint
#define lzo_hvoid_p             lzo_voidp
#define lzo_hbyte_p             lzo_bytep
#define LZOLIB_PUBLIC(r,f)      LZO_PUBLIC(r) f
#define lzo_hmemcmp             lzo_memcmp
#define lzo_hmemcpy             lzo_memcpy
#define lzo_hmemmove            lzo_memmove
#define lzo_hmemset             lzo_memset
#define __LZOLIB_HMEMCPY_CH_INCLUDED 1
#if !defined(LZOLIB_PUBLIC)
#  define LZOLIB_PUBLIC(r,f)    r __LZOLIB_FUNCNAME(f)
#endif
LZOLIB_PUBLIC(int, lzo_hmemcmp) (const lzo_hvoid_p s1, const lzo_hvoid_p s2, lzo_hsize_t len)
{
#if (LZO_HAVE_MM_HUGE_PTR) || !defined(HAVE_MEMCMP)
    const lzo_hbyte_p p1 = (const lzo_hbyte_p) s1;
    const lzo_hbyte_p p2 = (const lzo_hbyte_p) s2;
    if __lzo_likely(len > 0) do
    {
        int d = *p1 - *p2;
        if (d != 0)
            return (d);
        p1++; p2++;
    } while __lzo_likely(--len > 0);
    return (0);
#else
    return (memcmp(s1, s2, len));
#endif
}
LZOLIB_PUBLIC(lzo_hvoid_p, lzo_hmemcpy) (lzo_hvoid_p dest, const lzo_hvoid_p src, lzo_hsize_t len)
{
#if (LZO_HAVE_MM_HUGE_PTR) || !defined(HAVE_MEMCPY)
    lzo_hbyte_p p1 = (lzo_hbyte_p) dest;
    const lzo_hbyte_p p2 = (const lzo_hbyte_p) src;
    if (len <= 0 || p1 == p2)
        return (dest);
    do
        *p1++ = *p2++;
    while __lzo_likely(--len > 0);
    return (dest);
#else
    return (memcpy(dest, src, len));
#endif
}
LZOLIB_PUBLIC(lzo_hvoid_p, lzo_hmemmove) (lzo_hvoid_p dest, const lzo_hvoid_p src, lzo_hsize_t len)
{
#if (LZO_HAVE_MM_HUGE_PTR) || !defined(HAVE_MEMMOVE)
    lzo_hbyte_p p1 = (lzo_hbyte_p) dest;
    const lzo_hbyte_p p2 = (const lzo_hbyte_p) src;
    if (len <= 0 || p1 == p2)
        return (dest);
    if (p1 < p2)
    {
        do
            *p1++ = *p2++;
        while __lzo_likely(--len > 0);
    }
    else
    {
        p1 += len;
        p2 += len;
        do
            *--p1 = *--p2;
        while __lzo_likely(--len > 0);
    }
    return (dest);
#else
    return (memmove(dest, src, len));
#endif
}
LZOLIB_PUBLIC(lzo_hvoid_p, lzo_hmemset) (lzo_hvoid_p s, int c, lzo_hsize_t len)
{
#if (LZO_HAVE_MM_HUGE_PTR) || !defined(HAVE_MEMSET)
    lzo_hbyte_p p = (lzo_hbyte_p) s;
    if __lzo_likely(len > 0) do
        *p++ = (unsigned char) c;
    while __lzo_likely(--len > 0);
    return (s);
#else
    return (memset(s, c, len));
#endif
}
#undef LZOLIB_PUBLIC
#endif

#if !defined(__LZO_IN_MINILZO)

#define ACC_WANT_ACC_CHK_CH 1
#undef ACCCHK_ASSERT

    ACCCHK_ASSERT_IS_SIGNED_T(lzo_int)
    ACCCHK_ASSERT_IS_UNSIGNED_T(lzo_uint)

    ACCCHK_ASSERT_IS_SIGNED_T(lzo_int32)
    ACCCHK_ASSERT_IS_UNSIGNED_T(lzo_uint32)
    ACCCHK_ASSERT((LZO_UINT32_C(1) << (int)(8*sizeof(LZO_UINT32_C(1))-1)) > 0)
    ACCCHK_ASSERT(sizeof(lzo_uint32) >= 4)

#if !defined(__LZO_UINTPTR_T_IS_POINTER)
    ACCCHK_ASSERT_IS_UNSIGNED_T(lzo_uintptr_t)
#endif
    ACCCHK_ASSERT(sizeof(lzo_uintptr_t) >= sizeof(lzo_voidp))

    ACCCHK_ASSERT_IS_UNSIGNED_T(lzo_xint)
    ACCCHK_ASSERT(sizeof(lzo_xint) >= sizeof(lzo_uint32))
    ACCCHK_ASSERT(sizeof(lzo_xint) >= sizeof(lzo_uint))
    ACCCHK_ASSERT(sizeof(lzo_xint) == sizeof(lzo_uint32) || sizeof(lzo_xint) == sizeof(lzo_uint))

#endif
#undef ACCCHK_ASSERT

LZO_PUBLIC(int)
_lzo_config_check(void)
{
    lzo_bool r = 1;
    union { unsigned char c[2*sizeof(lzo_xint)]; lzo_xint l[2]; } u;

#if !defined(LZO_CFG_NO_CONFIG_CHECK)
#if defined(LZO_ABI_BIG_ENDIAN)
    u.l[0] = u.l[1] = 0; u.c[sizeof(lzo_xint) - 1] = 128;
    r &= (u.l[0] == 128);
#endif
#if defined(LZO_ABI_LITTLE_ENDIAN)
    u.l[0] = u.l[1] = 0; u.c[0] = 128;
    r &= (u.l[0] == 128);
#endif
#if defined(LZO_UNALIGNED_OK_2)
    u.l[0] = u.l[1] = 0;
    r &= ((* (const lzo_ushortp) (const lzo_voidp) &u.c[1]) == 0);
#endif
#if defined(LZO_UNALIGNED_OK_4)
    u.l[0] = u.l[1] = 0;
    r &= ((* (const lzo_uint32p) (const lzo_voidp) &u.c[1]) == 0);
#endif
#endif

    LZO_UNUSED(u);
    return ((r == 1) ? LZO_E_OK : LZO_E_ERROR);
}

int __lzo_init_done = 0;

LZO_PUBLIC(int)
__lzo_init_v2(unsigned v, int s1, int s2, int s3, int s4, int s5,
                          int s6, int s7, int s8, int s9)
{
    int r;

#if defined(__LZO_IN_MINILZO)
#elif (LZO_CC_MSC && ((_MSC_VER) < 700))
#else
#define ACC_WANT_ACC_CHK_CH 1
#undef ACCCHK_ASSERT
#define ACCCHK_ASSERT(expr)  LZO_COMPILE_TIME_ASSERT(expr)
#endif
#undef ACCCHK_ASSERT

    __lzo_init_done = 1;

    if (v == 0)
        return (LZO_E_ERROR);

    r = (s1 == -1 || s1 == (int) sizeof(short)) &&
        (s2 == -1 || s2 == (int) sizeof(int)) &&
        (s3 == -1 || s3 == (int) sizeof(long)) &&
        (s4 == -1 || s4 == (int) sizeof(lzo_uint32)) &&
        (s5 == -1 || s5 == (int) sizeof(lzo_uint)) &&
        (s6 == -1 || s6 == (int) lzo_sizeof_dict_t) &&
        (s7 == -1 || s7 == (int) sizeof(char *)) &&
        (s8 == -1 || s8 == (int) sizeof(lzo_voidp)) &&
        (s9 == -1 || s9 == (int) sizeof(lzo_callback_t));
    if (!r)
        return (LZO_E_ERROR);

    r = _lzo_config_check();
    if (r != LZO_E_OK)
        return (r);

    return (r);
}

#if !defined(__LZO_IN_MINILZO)

#if (LZO_OS_WIN16 && LZO_CC_WATCOMC) && defined(__SW_BD)

#if 0
BOOL FAR PASCAL LibMain (HANDLE hInstance, WORD wDataSegment,
                          WORD wHeapSize, LPSTR lpszCmdLine)
#else
int __far __pascal LibMain (int a, short b, short c, long d)
#endif
{
    LZO_UNUSED(a); LZO_UNUSED(b); LZO_UNUSED(c); LZO_UNUSED(d);
    return (1);
}

#endif

#endif

#define do_compress         _lzo1x_1_do_compress

#if !defined(MINILZO_CFG_SKIP_LZO1X_1_COMPRESS)

#define LZO_NEED_DICT_H
#define D_BITS          14
#define D_INDEX1(d,p)       d = DM(DMUL(0x21,DX3(p,5,5,6)) >> 5)
#define D_INDEX2(d,p)       d = (d & (D_MASK & 0x7ff)) ^ (D_HIGH | 0x1f)

#ifndef __LZO_CONFIG1X_H
#define __LZO_CONFIG1X_H

#if !defined(LZO1X) && !defined(LZO1Y) && !defined(LZO1Z)
#  define LZO1X
#endif

#if !defined(__LZO_IN_MINILZO)
#include "lzo/lzo1x.h"
#endif

#define LZO_EOF_CODE
#undef LZO_DETERMINISTIC

#define M1_MAX_OFFSET   0x0400
#ifndef M2_MAX_OFFSET
#define M2_MAX_OFFSET   0x0800
#endif
#define M3_MAX_OFFSET   0x4000
#define M4_MAX_OFFSET   0xbfff

#define MX_MAX_OFFSET   (M1_MAX_OFFSET + M2_MAX_OFFSET)

#define M1_MIN_LEN      2
#define M1_MAX_LEN      2
#define M2_MIN_LEN      3
#ifndef M2_MAX_LEN
#define M2_MAX_LEN      8
#endif
#define M3_MIN_LEN      3
#define M3_MAX_LEN      33
#define M4_MIN_LEN      3
#define M4_MAX_LEN      9

#define M1_MARKER       0
#define M2_MARKER       64
#define M3_MARKER       32
#define M4_MARKER       16

#ifndef MIN_LOOKAHEAD
#define MIN_LOOKAHEAD       (M2_MAX_LEN + 1)
#endif

#if defined(LZO_NEED_DICT_H)

#ifndef LZO_HASH
#define LZO_HASH            LZO_HASH_LZO_INCREMENTAL_B
#endif
#define DL_MIN_LEN          M2_MIN_LEN

#ifndef __LZO_DICT_H
#define __LZO_DICT_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(D_BITS) && defined(DBITS)
#  define D_BITS        DBITS
#endif
#if !defined(D_BITS)
#  error "D_BITS is not defined"
#endif
#if (D_BITS < 16)
#  define D_SIZE        LZO_SIZE(D_BITS)
#  define D_MASK        LZO_MASK(D_BITS)
#else
#  define D_SIZE        LZO_USIZE(D_BITS)
#  define D_MASK        LZO_UMASK(D_BITS)
#endif
#define D_HIGH          ((D_MASK >> 1) + 1)

#if !defined(DD_BITS)
#  define DD_BITS       0
#endif
#define DD_SIZE         LZO_SIZE(DD_BITS)
#define DD_MASK         LZO_MASK(DD_BITS)

#if !defined(DL_BITS)
#  define DL_BITS       (D_BITS - DD_BITS)
#endif
#if (DL_BITS < 16)
#  define DL_SIZE       LZO_SIZE(DL_BITS)
#  define DL_MASK       LZO_MASK(DL_BITS)
#else
#  define DL_SIZE       LZO_USIZE(DL_BITS)
#  define DL_MASK       LZO_UMASK(DL_BITS)
#endif

#if (D_BITS != DL_BITS + DD_BITS)
#  error "D_BITS does not match"
#endif
#if (D_BITS < 8 || D_BITS > 18)
#  error "invalid D_BITS"
#endif
#if (DL_BITS < 8 || DL_BITS > 20)
#  error "invalid DL_BITS"
#endif
#if (DD_BITS < 0 || DD_BITS > 6)
#  error "invalid DD_BITS"
#endif

#if !defined(DL_MIN_LEN)
#  define DL_MIN_LEN    3
#endif
#if !defined(DL_SHIFT)
#  define DL_SHIFT      ((DL_BITS + (DL_MIN_LEN - 1)) / DL_MIN_LEN)
#endif

#define LZO_HASH_GZIP                   1
#define LZO_HASH_GZIP_INCREMENTAL       2
#define LZO_HASH_LZO_INCREMENTAL_A      3
#define LZO_HASH_LZO_INCREMENTAL_B      4

#if !defined(LZO_HASH)
#  error "choose a hashing strategy"
#endif

#undef DM
#undef DX

#if (DL_MIN_LEN == 3)
#  define _DV2_A(p,shift1,shift2) \
        (((((lzo_xint)((p)[0]) << shift1) ^ (p)[1]) << shift2) ^ (p)[2])
#  define _DV2_B(p,shift1,shift2) \
        (((((lzo_xint)((p)[2]) << shift1) ^ (p)[1]) << shift2) ^ (p)[0])
#  define _DV3_B(p,shift1,shift2,shift3) \
        ((_DV2_B((p)+1,shift1,shift2) << (shift3)) ^ (p)[0])
#elif (DL_MIN_LEN == 2)
#  define _DV2_A(p,shift1,shift2) \
        (((lzo_xint)(p[0]) << shift1) ^ p[1])
#  define _DV2_B(p,shift1,shift2) \
        (((lzo_xint)(p[1]) << shift1) ^ p[2])
#else
#  error "invalid DL_MIN_LEN"
#endif
#define _DV_A(p,shift)      _DV2_A(p,shift,shift)
#define _DV_B(p,shift)      _DV2_B(p,shift,shift)
#define DA2(p,s1,s2) \
        (((((lzo_xint)((p)[2]) << (s2)) + (p)[1]) << (s1)) + (p)[0])
#define DS2(p,s1,s2) \
        (((((lzo_xint)((p)[2]) << (s2)) - (p)[1]) << (s1)) - (p)[0])
#define DX2(p,s1,s2) \
        (((((lzo_xint)((p)[2]) << (s2)) ^ (p)[1]) << (s1)) ^ (p)[0])
#define DA3(p,s1,s2,s3) ((DA2((p)+1,s2,s3) << (s1)) + (p)[0])
#define DS3(p,s1,s2,s3) ((DS2((p)+1,s2,s3) << (s1)) - (p)[0])
#define DX3(p,s1,s2,s3) ((DX2((p)+1,s2,s3) << (s1)) ^ (p)[0])
#define DMS(v,s)        ((lzo_uint) (((v) & (D_MASK >> (s))) << (s)))
#define DM(v)           DMS(v,0)

#if (LZO_HASH == LZO_HASH_GZIP)
#  define _DINDEX(dv,p)     (_DV_A((p),DL_SHIFT))

#elif (LZO_HASH == LZO_HASH_GZIP_INCREMENTAL)
#  define __LZO_HASH_INCREMENTAL
#  define DVAL_FIRST(dv,p)  dv = _DV_A((p),DL_SHIFT)
#  define DVAL_NEXT(dv,p)   dv = (((dv) << DL_SHIFT) ^ p[2])
#  define _DINDEX(dv,p)     (dv)
#  define DVAL_LOOKAHEAD    DL_MIN_LEN

#elif (LZO_HASH == LZO_HASH_LZO_INCREMENTAL_A)
#  define __LZO_HASH_INCREMENTAL
#  define DVAL_FIRST(dv,p)  dv = _DV_A((p),5)
#  define DVAL_NEXT(dv,p) \
                dv ^= (lzo_xint)(p[-1]) << (2*5); dv = (((dv) << 5) ^ p[2])
#  define _DINDEX(dv,p)     ((DMUL(0x9f5f,dv)) >> 5)
#  define DVAL_LOOKAHEAD    DL_MIN_LEN

#elif (LZO_HASH == LZO_HASH_LZO_INCREMENTAL_B)
#  define __LZO_HASH_INCREMENTAL
#  define DVAL_FIRST(dv,p)  dv = _DV_B((p),5)
#  define DVAL_NEXT(dv,p) \
                dv ^= p[-1]; dv = (((dv) >> 5) ^ ((lzo_xint)(p[2]) << (2*5)))
#  define _DINDEX(dv,p)     ((DMUL(0x9f5f,dv)) >> 5)
#  define DVAL_LOOKAHEAD    DL_MIN_LEN

#else
#  error "choose a hashing strategy"
#endif

#ifndef DINDEX
#define DINDEX(dv,p)        ((lzo_uint)((_DINDEX(dv,p)) & DL_MASK) << DD_BITS)
#endif
#if !defined(DINDEX1) && defined(D_INDEX1)
#define DINDEX1             D_INDEX1
#endif
#if !defined(DINDEX2) && defined(D_INDEX2)
#define DINDEX2             D_INDEX2
#endif

#if !defined(__LZO_HASH_INCREMENTAL)
#  define DVAL_FIRST(dv,p)  ((void) 0)
#  define DVAL_NEXT(dv,p)   ((void) 0)
#  define DVAL_LOOKAHEAD    0
#endif

#if !defined(DVAL_ASSERT)
#if defined(__LZO_HASH_INCREMENTAL) && !defined(NDEBUG)
static void DVAL_ASSERT(lzo_xint dv, const lzo_bytep p)
{
    lzo_xint df;
    DVAL_FIRST(df,(p));
    assert(DINDEX(dv,p) == DINDEX(df,p));
}
#else
#  define DVAL_ASSERT(dv,p) ((void) 0)
#endif
#endif

#if defined(LZO_DICT_USE_PTR)
#  define DENTRY(p,in)                          (p)
#  define GINDEX(m_pos,m_off,dict,dindex,in)    m_pos = dict[dindex]
#else
#  define DENTRY(p,in)                          ((lzo_uint) ((p)-(in)))
#  define GINDEX(m_pos,m_off,dict,dindex,in)    m_off = dict[dindex]
#endif

#if (DD_BITS == 0)

#  define UPDATE_D(dict,drun,dv,p,in)       dict[ DINDEX(dv,p) ] = DENTRY(p,in)
#  define UPDATE_I(dict,drun,index,p,in)    dict[index] = DENTRY(p,in)
#  define UPDATE_P(ptr,drun,p,in)           (ptr)[0] = DENTRY(p,in)

#else

#  define UPDATE_D(dict,drun,dv,p,in)   \
        dict[ DINDEX(dv,p) + drun++ ] = DENTRY(p,in); drun &= DD_MASK
#  define UPDATE_I(dict,drun,index,p,in)    \
        dict[ (index) + drun++ ] = DENTRY(p,in); drun &= DD_MASK
#  define UPDATE_P(ptr,drun,p,in)   \
        (ptr) [ drun++ ] = DENTRY(p,in); drun &= DD_MASK

#endif

#if defined(LZO_DICT_USE_PTR)

#define LZO_CHECK_MPOS_DET(m_pos,m_off,in,ip,max_offset) \
        (m_pos == NULL || (m_off = pd(ip, m_pos)) > max_offset)

#define LZO_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,max_offset) \
    (BOUNDS_CHECKING_OFF_IN_EXPR((\
        m_pos = ip - (lzo_uint) PTR_DIFF(ip,m_pos), \
        PTR_LT(m_pos,in) || \
        (m_off = (lzo_uint) PTR_DIFF(ip,m_pos)) <= 0 || \
         m_off > max_offset)))

#else

#define LZO_CHECK_MPOS_DET(m_pos,m_off,in,ip,max_offset) \
        (m_off == 0 || \
         ((m_off = pd(ip, in) - m_off) > max_offset) || \
         (m_pos = (ip) - (m_off), 0))

#define LZO_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,max_offset) \
        (pd(ip, in) <= m_off || \
         ((m_off = pd(ip, in) - m_off) > max_offset) || \
         (m_pos = (ip) - (m_off), 0))

#endif

#if defined(LZO_DETERMINISTIC)
#  define LZO_CHECK_MPOS    LZO_CHECK_MPOS_DET
#else
#  define LZO_CHECK_MPOS    LZO_CHECK_MPOS_NON_DET
#endif

#ifdef __cplusplus
}
#endif

#endif

#endif

#endif

#define DO_COMPRESS     lzo1x_1_compress

#include "Common.h"
#include "Uart.h"
#include "RemoteCommon.h"

// Holds the hash-type library (64K)
lzo_align_t __LZO_MMODEL compressionWorkMemory[LZO1X_1_MEM_COMPRESS];

// Holds temporary partially compressed code before being sent
unsigned char compressedDataCacheBuffer[2];

// Maintains the compressed data length
unsigned long compressedDataLen;

// Handles incrementing the data length, sending data and managing the temp compressed data buffer
#define SEND_COMPRESSED_DATA()	compressedDataLen++; if (compressedDataLen > 2) WriteCompressedData(compressedDataCacheBuffer[0], outMode); compressedDataCacheBuffer[0] = compressedDataCacheBuffer[1];

// Compression subroutine (main algorithm)
static __lzo_noinline lzo_uint
do_compress (const lzo_bytep in, lzo_uint in_len, lzo_uint outMode)
{
    register const lzo_bytep ip;
    const lzo_bytep const in_end = in + in_len;
    const lzo_bytep const ip_end = in + in_len - M2_MAX_LEN - 5;
    const lzo_bytep ii;
    lzo_dict_p const dict = (lzo_dict_p)&compressionWorkMemory[0];

    ip = in;
    ii = ip;

    ip += 4;
    for (;;)
    {
        register const lzo_bytep m_pos;
        lzo_uint m_off;
        lzo_uint m_len;
        lzo_uint dindex;

		// Every so often, handle system events
		if ((compressedDataLen > 255) && ((compressedDataLen % 256) == 0))
		{
			if (outMode == OUT_SERIAL)
			{
				HandleSystemEvents();
			}
		}

        DINDEX1(dindex, ip);
        GINDEX(m_pos, m_off, dict, dindex, in);

        if (LZO_CHECK_MPOS_NON_DET(m_pos, m_off, in, ip, M4_MAX_OFFSET))
            goto literal;

        if (m_off <= M2_MAX_OFFSET || m_pos[3] == ip[3])
            goto try_match;
            
        DINDEX2(dindex, ip);
        GINDEX(m_pos, m_off, dict, dindex, in);
        
        if (LZO_CHECK_MPOS_NON_DET(m_pos, m_off, in, ip, M4_MAX_OFFSET))
            goto literal;
            
        if (m_off <= M2_MAX_OFFSET || m_pos[3] == ip[3])
            goto try_match;
            
        goto literal;

try_match:
        if (m_pos[0] != ip[0] || m_pos[1] != ip[1])
        {
			// Do nothing
        }
        else
        {
            if __lzo_likely(m_pos[2] == ip[2])
            {
				goto match;
            }
        }

literal:
		UPDATE_I(dict, 0, dindex, ip, in);
        ++ip;

        if __lzo_unlikely(ip >= ip_end)
            break;
        continue;

match:
        UPDATE_I(dict, 0, dindex, ip, in);

        if (pd(ip, ii) > 0)
        {
            register lzo_uint t = pd(ip, ii);

            if (t <= 3)
            {
                compressedDataCacheBuffer[0] |= LZO_BYTE(t);
            }
            else if (t <= 18)
            {
				SEND_COMPRESSED_DATA();
				compressedDataCacheBuffer[1] = LZO_BYTE(t - 3);
            }
            else
            {
                register lzo_uint tt = t - 18;

				SEND_COMPRESSED_DATA();
				compressedDataCacheBuffer[1] = 0;
				
                while (tt > 255)
                {
                    tt -= 255;

					SEND_COMPRESSED_DATA();
					compressedDataCacheBuffer[1] = 0;
                }
                
                if (tt <= 0) { debugErr("MiniLZO compression Assert Failed! (tt <= 0)\n"); }

				SEND_COMPRESSED_DATA();
				compressedDataCacheBuffer[1] = LZO_BYTE(tt);
            }
            do
            {
				SEND_COMPRESSED_DATA();
				compressedDataCacheBuffer[1] = *ii++;
            } while (--t > 0);
        }

        if (ii != ip) { debugErr("MiniLZO compression Assert Failed! (ii != ip)\n"); }

        ip += 3;

        if (m_pos[3] != *ip++ || m_pos[4] != *ip++ || m_pos[5] != *ip++ ||
            m_pos[6] != *ip++ || m_pos[7] != *ip++ || m_pos[8] != *ip++)
        {
            --ip;
            m_len = pd(ip, ii);

	        if (m_len < 3) { debugErr("MiniLZO compression Assert Failed! (m_len < 3)\n"); }
	        if (m_len > M2_MAX_LEN) { debugErr("MiniLZO compression Assert Failed! (m_len > M2_MAX_LEN)\n"); }

            if (m_off <= M2_MAX_OFFSET)
            {
                m_off -= 1;

				SEND_COMPRESSED_DATA();
				compressedDataCacheBuffer[1] = LZO_BYTE(((m_len - 1) << 5) | ((m_off & 7) << 2));

				SEND_COMPRESSED_DATA();
				compressedDataCacheBuffer[1] = LZO_BYTE(m_off >> 3);
            }
            else if (m_off <= M3_MAX_OFFSET)
            {
                m_off -= 1;
                
				SEND_COMPRESSED_DATA();
				compressedDataCacheBuffer[1] = LZO_BYTE(M3_MARKER | (m_len - 2));
                
                goto m3_m4_offset;
            }
            else
            {
                m_off -= 0x4000;

		        if (m_off <= 0) { debugErr("MiniLZO compression Assert Failed! (m_off <= 0)\n"); }
		        if (m_off > 0x7fff) { debugErr("MiniLZO compression Assert Failed! (m_off > 0x7fff)\n"); }

				SEND_COMPRESSED_DATA();
				compressedDataCacheBuffer[1] = LZO_BYTE(M4_MARKER | ((m_off & 0x4000) >> 11) | (m_len - 2));

                goto m3_m4_offset;
            }
        }
        else
        {
            {
                const lzo_bytep end = in_end;
                const lzo_bytep m = m_pos + M2_MAX_LEN + 1;

                while (ip < end && *m == *ip)
                    m++, ip++;

                m_len = pd(ip, ii);
            }

	        if (m_len <= M2_MAX_LEN) { debugErr("MiniLZO compression Assert Failed! (m_len <= M2_MAX_LEN)\n"); }

            if (m_off <= M3_MAX_OFFSET)
            {
                m_off -= 1;

                if (m_len <= 33)
                {
					SEND_COMPRESSED_DATA();
					compressedDataCacheBuffer[1] = LZO_BYTE(M3_MARKER | (m_len - 2));
                }
                else
                {
                    m_len -= 33;
	                
					SEND_COMPRESSED_DATA();
					compressedDataCacheBuffer[1] = M3_MARKER | 0;
                    
                    goto m3_m4_len;
                }
            }
            else
            {
                m_off -= 0x4000;

		        if (m_off <= 0) { debugErr("MiniLZO compression Assert Failed! (m_off <= 0)\n"); }
		        if (m_off > 0x7fff) { debugErr("MiniLZO compression Assert Failed! (m_off > 0x7fff)\n"); }

                if (m_len <= M4_MAX_LEN)
                {
					SEND_COMPRESSED_DATA();
					compressedDataCacheBuffer[1] = LZO_BYTE(M4_MARKER | ((m_off & 0x4000) >> 11) | (m_len - 2));
                }
                else
                {
                    m_len -= M4_MAX_LEN;
	                
					SEND_COMPRESSED_DATA();
					compressedDataCacheBuffer[1] = LZO_BYTE(M4_MARKER | ((m_off & 0x4000) >> 11));
m3_m4_len:
                    while (m_len > 255)
                    {
                        m_len -= 255;
		                
						SEND_COMPRESSED_DATA();
						compressedDataCacheBuffer[1] = 0;
                    }

		        	if (m_len <= 0) { debugErr("MiniLZO compression Assert Failed! (m_len <= 0)\n"); }
	                
					SEND_COMPRESSED_DATA();
					compressedDataCacheBuffer[1] = LZO_BYTE(m_len);
                }
            }

m3_m4_offset:
			SEND_COMPRESSED_DATA();
			compressedDataCacheBuffer[1] = LZO_BYTE((m_off & 63) << 2);
            
			SEND_COMPRESSED_DATA();
			compressedDataCacheBuffer[1] = LZO_BYTE(m_off >> 6);
        }

        ii = ip;
        if __lzo_unlikely(ip >= ip_end)
            break;
    }

    return (pd(in_end, ii));
}

// Compression Algorithm
LZO_PUBLIC(unsigned long int)
DO_COMPRESS      (const lzo_bytep in, lzo_uint in_len, lzo_uint outMode)
// int lzo1x_1_compress(const lzo_bytep in, lzo_uint in_len)
{
    lzo_uint t;

	// Reset compressedDataLen
	compressedDataLen = 0;

    if __lzo_unlikely(in_len <= M2_MAX_LEN + 5)
        t = in_len;
    else
    {
        t = do_compress(in, in_len, outMode);
    }

    if (t > 0)
    {
        const lzo_bytep ii = in + in_len - t;

        if ((compressedDataLen == 0) && (t <= 238))
        {
			SEND_COMPRESSED_DATA();
			compressedDataCacheBuffer[1] = LZO_BYTE(17 + t);
        }
        else if (t <= 3)
        {
			compressedDataCacheBuffer[0] |= LZO_BYTE(t);
		}
        else if (t <= 18)
        {
			SEND_COMPRESSED_DATA();
			compressedDataCacheBuffer[1] = LZO_BYTE(t - 3);
        }
        else
        {
            lzo_uint tt = t - 18;

			SEND_COMPRESSED_DATA();
			compressedDataCacheBuffer[1] = 0;

            while (tt > 255)
            {
                tt -= 255;
                
				SEND_COMPRESSED_DATA();
				compressedDataCacheBuffer[1] = 0;
            }

		    if (tt <= 0) { debugErr("MiniLZO compression Assert Failed! (tt <= 0)\n"); }

			SEND_COMPRESSED_DATA();
			compressedDataCacheBuffer[1] = LZO_BYTE(tt);
        }

        do
        { 
			SEND_COMPRESSED_DATA();
			compressedDataCacheBuffer[1] = *ii++;
        } while (--t > 0);
    }

	SEND_COMPRESSED_DATA();
	compressedDataCacheBuffer[1] = M4_MARKER | 1;

	SEND_COMPRESSED_DATA();
	compressedDataCacheBuffer[1] = 0;

	SEND_COMPRESSED_DATA();
	compressedDataCacheBuffer[1] = 0;

	SEND_COMPRESSED_DATA();

	SEND_COMPRESSED_DATA();

	compressedDataLen -= 2;

    return (compressedDataLen);
}

#endif

#undef do_compress
#undef DO_COMPRESS
#undef LZO_HASH

#undef LZO_TEST_OVERRUN
#undef DO_DECOMPRESS
#define DO_DECOMPRESS       lzo1x_decompress

#if !defined(MINILZO_CFG_SKIP_LZO1X_DECOMPRESS)

#if defined(LZO_TEST_OVERRUN)
#  if !defined(LZO_TEST_OVERRUN_INPUT)
#    define LZO_TEST_OVERRUN_INPUT       2
#  endif
#  if !defined(LZO_TEST_OVERRUN_OUTPUT)
#    define LZO_TEST_OVERRUN_OUTPUT      2
#  endif
#  if !defined(LZO_TEST_OVERRUN_LOOKBEHIND)
#    define LZO_TEST_OVERRUN_LOOKBEHIND
#  endif
#endif

#undef TEST_IP
#undef TEST_OP
#undef TEST_LB
#undef TEST_LBO
#undef NEED_IP
#undef NEED_OP
#undef HAVE_TEST_IP
#undef HAVE_TEST_OP
#undef HAVE_NEED_IP
#undef HAVE_NEED_OP
#undef HAVE_ANY_IP
#undef HAVE_ANY_OP

#if defined(LZO_TEST_OVERRUN_INPUT)
#  if (LZO_TEST_OVERRUN_INPUT >= 1)
#    define TEST_IP             (ip < ip_end)
#  endif
#  if (LZO_TEST_OVERRUN_INPUT >= 2)
#    define NEED_IP(x) \
            if ((lzo_uint)(ip_end - ip) < (lzo_uint)(x))  goto input_overrun
#  endif
#endif

#if defined(LZO_TEST_OVERRUN_OUTPUT)
#  if (LZO_TEST_OVERRUN_OUTPUT >= 1)
#    define TEST_OP             (op <= op_end)
#  endif
#  if (LZO_TEST_OVERRUN_OUTPUT >= 2)
#    undef TEST_OP
#    define NEED_OP(x) \
            if ((lzo_uint)(op_end - op) < (lzo_uint)(x))  goto output_overrun
#  endif
#endif

#if defined(LZO_TEST_OVERRUN_LOOKBEHIND)
#  define TEST_LB(m_pos)        if (m_pos < out || m_pos >= op) goto lookbehind_overrun
#  define TEST_LBO(m_pos,o)     if (m_pos < out || m_pos >= op - (o)) goto lookbehind_overrun
#else
#  define TEST_LB(m_pos)        ((void) 0)
#  define TEST_LBO(m_pos,o)     ((void) 0)
#endif

#if !defined(LZO_EOF_CODE) && !defined(TEST_IP)
#  define TEST_IP               (ip < ip_end)
#endif

#if defined(TEST_IP)
#  define HAVE_TEST_IP
#else
#  define TEST_IP               1
#endif
#if defined(TEST_OP)
#  define HAVE_TEST_OP
#else
#  define TEST_OP               1
#endif

#if defined(NEED_IP)
#  define HAVE_NEED_IP
#else
#  define NEED_IP(x)            ((void) 0)
#endif
#if defined(NEED_OP)
#  define HAVE_NEED_OP
#else
#  define NEED_OP(x)            ((void) 0)
#endif

#if defined(HAVE_TEST_IP) || defined(HAVE_NEED_IP)
#  define HAVE_ANY_IP
#endif
#if defined(HAVE_TEST_OP) || defined(HAVE_NEED_OP)
#  define HAVE_ANY_OP
#endif

#undef __COPY4
#define __COPY4(dst,src)    * (lzo_uint32p)(dst) = * (const lzo_uint32p)(src)

#undef COPY4
#if defined(LZO_UNALIGNED_OK_4)
#  define COPY4(dst,src)    __COPY4(dst,src)
#elif defined(LZO_ALIGNED_OK_4)
#  define COPY4(dst,src)    __COPY4((lzo_uintptr_t)(dst),(lzo_uintptr_t)(src))
#endif

#if defined(DO_DECOMPRESS)
LZO_PUBLIC(int)
DO_DECOMPRESS  (const lzo_bytep in , lzo_uint  in_len,
                       lzo_bytep out, lzo_uintp out_len,
                       lzo_voidp wrkmem)
#endif
{
    register lzo_bytep op;
    register const lzo_bytep ip;
    register lzo_uint t;
#if defined(COPY_DICT)
    lzo_uint m_off;
    const lzo_bytep dict_end;
#else
    register const lzo_bytep m_pos;
#endif

    const lzo_bytep const ip_end = in + in_len;
#if defined(HAVE_ANY_OP)
    lzo_bytep const op_end = out + *out_len;
#endif
#if defined(LZO1Z)
    lzo_uint last_m_off = 0;
#endif

    LZO_UNUSED(wrkmem);

#if defined(COPY_DICT)
    if (dict)
    {
        if (dict_len > M4_MAX_OFFSET)
        {
            dict += dict_len - M4_MAX_OFFSET;
            dict_len = M4_MAX_OFFSET;
        }
        dict_end = dict + dict_len;
    }
    else
    {
        dict_len = 0;
        dict_end = NULL;
    }
#endif

    *out_len = 0;

    op = out;
    ip = in;

    if (*ip > 17)
    {
        t = (lzo_uint)(*ip++ - 17);

        if (t < 4)
            goto match_next;

        assert(t > 0);
        
        NEED_OP(t); NEED_IP(t+1);

        do *op++ = *ip++; while (--t > 0);

        goto first_literal_run;
    }

    while (TEST_IP && TEST_OP)
    {
        t = *ip++;
        if (t >= 16)
            goto match;
        if (t == 0)
        {
            NEED_IP(1);
            while (*ip == 0)
            {
                t += 255;
                ip++;
                NEED_IP(1);
            }
            t += 15 + *ip++;
        }
        
        assert(t > 0);
        
        NEED_OP(t+3); NEED_IP(t+4);
#if defined(LZO_UNALIGNED_OK_4) || defined(LZO_ALIGNED_OK_4)
#if !defined(LZO_UNALIGNED_OK_4)
        if (PTR_ALIGNED2_4(op,ip))
        {
#endif
        COPY4(op,ip);
        op += 4; ip += 4;
        if (--t > 0)
        {
            if (t >= 4)
            {
                do {
                    COPY4(op,ip);
                    op += 4; ip += 4; t -= 4;
                } while (t >= 4);
                if (t > 0) do *op++ = *ip++; while (--t > 0);
            }
            else
                do *op++ = *ip++; while (--t > 0);
        }
#if !defined(LZO_UNALIGNED_OK_4)
        }
        else
#endif
#endif
#if !defined(LZO_UNALIGNED_OK_4)
        {
            *op++ = *ip++; *op++ = *ip++; *op++ = *ip++;
            do *op++ = *ip++; while (--t > 0);
        }
#endif

first_literal_run:

        t = *ip++;
        if (t >= 16)
            goto match;
#if defined(COPY_DICT)
#if defined(LZO1Z)
        m_off = (1 + M2_MAX_OFFSET) + (t << 6) + (*ip++ >> 2);
        last_m_off = m_off;
#else
        m_off = (1 + M2_MAX_OFFSET) + (t >> 2) + (*ip++ << 2);
#endif
        NEED_OP(3);
        t = 3; COPY_DICT(t,m_off)
#else
#if defined(LZO1Z)
        t = (1 + M2_MAX_OFFSET) + (t << 6) + (*ip++ >> 2);
        m_pos = op - t;
        last_m_off = t;
#else
        m_pos = op - (1 + M2_MAX_OFFSET);
        m_pos -= t >> 2;
        m_pos -= *ip++ << 2;
#endif
        TEST_LB(m_pos); NEED_OP(3);
        *op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos;
#endif
        goto match_done;

        do {
match:
            if (t >= 64)
            {
#if defined(COPY_DICT)
#if defined(LZO1X)
                m_off = 1 + ((t >> 2) & 7) + (*ip++ << 3);
                t = (t >> 5) - 1;
#elif defined(LZO1Y)
                m_off = 1 + ((t >> 2) & 3) + (*ip++ << 2);
                t = (t >> 4) - 3;
#elif defined(LZO1Z)
                m_off = t & 0x1f;
                if (m_off >= 0x1c)
                    m_off = last_m_off;
                else
                {
                    m_off = 1 + (m_off << 6) + (*ip++ >> 2);
                    last_m_off = m_off;
                }
                t = (t >> 5) - 1;
#endif
#else
#if defined(LZO1X)
                m_pos = op - 1;
                m_pos -= (t >> 2) & 7;
                m_pos -= *ip++ << 3;
                t = (t >> 5) - 1;
#elif defined(LZO1Y)
                m_pos = op - 1;
                m_pos -= (t >> 2) & 3;
                m_pos -= *ip++ << 2;
                t = (t >> 4) - 3;
#elif defined(LZO1Z)
                {
                    lzo_uint off = t & 0x1f;
                    m_pos = op;
                    if (off >= 0x1c)
                    {
                        assert(last_m_off > 0);
                        
                        m_pos -= last_m_off;
                    }
                    else
                    {
                        off = 1 + (off << 6) + (*ip++ >> 2);
                        m_pos -= off;
                        last_m_off = off;
                    }
                }
                t = (t >> 5) - 1;
#endif
                TEST_LB(m_pos);
                
                assert(t > 0);
                
                NEED_OP(t+3-1);
                
                goto copy_match;
#endif
            }
            else if (t >= 32)
            {
                t &= 31;
                if (t == 0)
                {
                    NEED_IP(1);
                    while (*ip == 0)
                    {
                        t += 255;
                        ip++;
                        NEED_IP(1);
                    }
                    t += 31 + *ip++;
                }
#if defined(COPY_DICT)
#if defined(LZO1Z)
                m_off = 1 + (ip[0] << 6) + (ip[1] >> 2);
                last_m_off = m_off;
#else
                m_off = 1 + (ip[0] >> 2) + (ip[1] << 6);
#endif
#else
#if defined(LZO1Z)
                {
                    lzo_uint off = 1 + (ip[0] << 6) + (ip[1] >> 2);
                    m_pos = op - off;
                    last_m_off = off;
                }
#elif defined(LZO_UNALIGNED_OK_2) && defined(LZO_ABI_LITTLE_ENDIAN)
                m_pos = op - 1;
                m_pos -= (* (const lzo_ushortp) ip) >> 2;
#else
                m_pos = op - 1;
                m_pos -= (ip[0] >> 2) + (ip[1] << 6);
#endif
#endif
                ip += 2;
            }
            else if (t >= 16)
            {
#if defined(COPY_DICT)
                m_off = (t & 8) << 11;
#else
                m_pos = op;
                m_pos -= (t & 8) << 11;
#endif
                t &= 7;
                if (t == 0)
                {
                    NEED_IP(1);
                    while (*ip == 0)
                    {
                        t += 255;
                        ip++;
                        NEED_IP(1);
                    }
                    t += 7 + *ip++;
                }
#if defined(COPY_DICT)
#if defined(LZO1Z)
                m_off += (ip[0] << 6) + (ip[1] >> 2);
#else
                m_off += (ip[0] >> 2) + (ip[1] << 6);
#endif
                ip += 2;
                if (m_off == 0)
                    goto eof_found;
                m_off += 0x4000;
#if defined(LZO1Z)
                last_m_off = m_off;
#endif
#else
#if defined(LZO1Z)
                m_pos -= (ip[0] << 6) + (ip[1] >> 2);
#elif defined(LZO_UNALIGNED_OK_2) && defined(LZO_ABI_LITTLE_ENDIAN)
                m_pos -= (* (const lzo_ushortp) ip) >> 2;
#else
                m_pos -= (ip[0] >> 2) + (ip[1] << 6);
#endif
                ip += 2;
                if (m_pos == op)
                    goto eof_found;
                m_pos -= 0x4000;
#if defined(LZO1Z)
                last_m_off = pd((const lzo_bytep)op, m_pos);
#endif
#endif
            }
            else
            {
#if defined(COPY_DICT)
#if defined(LZO1Z)
                m_off = 1 + (t << 6) + (*ip++ >> 2);
                last_m_off = m_off;
#else
                m_off = 1 + (t >> 2) + (*ip++ << 2);
#endif
                NEED_OP(2);
                t = 2; COPY_DICT(t,m_off)
#else
#if defined(LZO1Z)
                t = 1 + (t << 6) + (*ip++ >> 2);
                m_pos = op - t;
                last_m_off = t;
#else
                m_pos = op - 1;
                m_pos -= t >> 2;
                m_pos -= *ip++ << 2;
#endif
                TEST_LB(m_pos); NEED_OP(2);
                *op++ = *m_pos++; *op++ = *m_pos;
#endif
                goto match_done;
            }

#if defined(COPY_DICT)

            NEED_OP(t+3-1);
            t += 3-1; COPY_DICT(t,m_off)

#else

            TEST_LB(m_pos);
            
            assert(t > 0);
            
            NEED_OP(t+3-1);
#if defined(LZO_UNALIGNED_OK_4) || defined(LZO_ALIGNED_OK_4)
#if !defined(LZO_UNALIGNED_OK_4)
            if (t >= 2 * 4 - (3 - 1) && PTR_ALIGNED2_4(op,m_pos))
            {
                assert((op - m_pos) >= 4);
#else
            if (t >= 2 * 4 - (3 - 1) && (op - m_pos) >= 4)
            {
#endif
                COPY4(op,m_pos);
                op += 4; m_pos += 4; t -= 4 - (3 - 1);
                do {
                    COPY4(op,m_pos);
                    op += 4; m_pos += 4; t -= 4;
                } while (t >= 4);
                if (t > 0) do *op++ = *m_pos++; while (--t > 0);
            }
            else
#endif
            {
copy_match:
                *op++ = *m_pos++; *op++ = *m_pos++;
                do *op++ = *m_pos++; while (--t > 0);
            }

#endif

match_done:
#if defined(LZO1Z)
            t = ip[-1] & 3;
#else
            t = (lzo_uint)(ip[-2] & 3);
#endif
            if (t == 0)
                break;

match_next:
            assert(t > 0); assert(t < 4); NEED_OP(t); NEED_IP(t+1);
#if 0
            do *op++ = *ip++; while (--t > 0);
#else
            *op++ = *ip++;
            if (t > 1) { *op++ = *ip++; if (t > 2) { *op++ = *ip++; } }
#endif
            t = *ip++;
        } while (TEST_IP && TEST_OP);
    }

#if defined(HAVE_TEST_IP) || defined(HAVE_TEST_OP)
    *out_len = pd(op, out);
    return (LZO_E_EOF_NOT_FOUND);
#endif

eof_found:
    assert(t == 1);
    *out_len = pd(op, out);
    return (ip == ip_end ? LZO_E_OK :
           (ip < ip_end  ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));

#if defined(HAVE_NEED_IP)
input_overrun:
    *out_len = pd(op, out);
    return (LZO_E_INPUT_OVERRUN);
#endif

#if defined(HAVE_NEED_OP)
output_overrun:
    *out_len = pd(op, out);
    return (LZO_E_OUTPUT_OVERRUN);
#endif

#if defined(LZO_TEST_OVERRUN_LOOKBEHIND)
lookbehind_overrun:
    *out_len = pd(op, out);
    return (LZO_E_LOOKBEHIND_OVERRUN);
#endif
}

#endif

#define LZO_TEST_OVERRUN
#undef DO_DECOMPRESS
#define DO_DECOMPRESS       lzo1x_decompress_safe

#if !defined(MINILZO_CFG_SKIP_LZO1X_DECOMPRESS_SAFE)

#if defined(LZO_TEST_OVERRUN)
#  if !defined(LZO_TEST_OVERRUN_INPUT)
#    define LZO_TEST_OVERRUN_INPUT       2
#  endif
#  if !defined(LZO_TEST_OVERRUN_OUTPUT)
#    define LZO_TEST_OVERRUN_OUTPUT      2
#  endif
#  if !defined(LZO_TEST_OVERRUN_LOOKBEHIND)
#    define LZO_TEST_OVERRUN_LOOKBEHIND
#  endif
#endif

#undef TEST_IP
#undef TEST_OP
#undef TEST_LB
#undef TEST_LBO
#undef NEED_IP
#undef NEED_OP
#undef HAVE_TEST_IP
#undef HAVE_TEST_OP
#undef HAVE_NEED_IP
#undef HAVE_NEED_OP
#undef HAVE_ANY_IP
#undef HAVE_ANY_OP

#if defined(LZO_TEST_OVERRUN_INPUT)
#  if (LZO_TEST_OVERRUN_INPUT >= 1)
#    define TEST_IP             (ip < ip_end)
#  endif
#  if (LZO_TEST_OVERRUN_INPUT >= 2)
#    define NEED_IP(x) \
            if ((lzo_uint)(ip_end - ip) < (lzo_uint)(x))  goto input_overrun
#  endif
#endif

#if defined(LZO_TEST_OVERRUN_OUTPUT)
#  if (LZO_TEST_OVERRUN_OUTPUT >= 1)
#    define TEST_OP             (op <= op_end)
#  endif
#  if (LZO_TEST_OVERRUN_OUTPUT >= 2)
#    undef TEST_OP
#    define NEED_OP(x) \
            if ((lzo_uint)(op_end - op) < (lzo_uint)(x))  goto output_overrun
#  endif
#endif

#if defined(LZO_TEST_OVERRUN_LOOKBEHIND)
#  define TEST_LB(m_pos)        if (m_pos < out || m_pos >= op) goto lookbehind_overrun
#  define TEST_LBO(m_pos,o)     if (m_pos < out || m_pos >= op - (o)) goto lookbehind_overrun
#else
#  define TEST_LB(m_pos)        ((void) 0)
#  define TEST_LBO(m_pos,o)     ((void) 0)
#endif

#if !defined(LZO_EOF_CODE) && !defined(TEST_IP)
#  define TEST_IP               (ip < ip_end)
#endif

#if defined(TEST_IP)
#  define HAVE_TEST_IP
#else
#  define TEST_IP               1
#endif
#if defined(TEST_OP)
#  define HAVE_TEST_OP
#else
#  define TEST_OP               1
#endif

#if defined(NEED_IP)
#  define HAVE_NEED_IP
#else
#  define NEED_IP(x)            ((void) 0)
#endif
#if defined(NEED_OP)
#  define HAVE_NEED_OP
#else
#  define NEED_OP(x)            ((void) 0)
#endif

#if defined(HAVE_TEST_IP) || defined(HAVE_NEED_IP)
#  define HAVE_ANY_IP
#endif
#if defined(HAVE_TEST_OP) || defined(HAVE_NEED_OP)
#  define HAVE_ANY_OP
#endif

#undef __COPY4
#define __COPY4(dst,src)    * (lzo_uint32p)(dst) = * (const lzo_uint32p)(src)

#undef COPY4
#if defined(LZO_UNALIGNED_OK_4)
#  define COPY4(dst,src)    __COPY4(dst,src)
#elif defined(LZO_ALIGNED_OK_4)
#  define COPY4(dst,src)    __COPY4((lzo_uintptr_t)(dst),(lzo_uintptr_t)(src))
#endif

#if defined(DO_DECOMPRESS)
LZO_PUBLIC(int)
DO_DECOMPRESS  (const lzo_bytep in , lzo_uint  in_len,
                       lzo_bytep out, lzo_uintp out_len,
                       lzo_voidp wrkmem)
#endif
{
    register lzo_bytep op;
    register const lzo_bytep ip;
    register lzo_uint t;
#if defined(COPY_DICT)
    lzo_uint m_off;
    const lzo_bytep dict_end;
#else
    register const lzo_bytep m_pos;
#endif

    const lzo_bytep const ip_end = in + in_len;
#if defined(HAVE_ANY_OP)
    lzo_bytep const op_end = out + *out_len;
#endif
#if defined(LZO1Z)
    lzo_uint last_m_off = 0;
#endif

    LZO_UNUSED(wrkmem);

#if defined(COPY_DICT)
    if (dict)
    {
        if (dict_len > M4_MAX_OFFSET)
        {
            dict += dict_len - M4_MAX_OFFSET;
            dict_len = M4_MAX_OFFSET;
        }
        dict_end = dict + dict_len;
    }
    else
    {
        dict_len = 0;
        dict_end = NULL;
    }
#endif

    *out_len = 0;

    op = out;
    ip = in;

    if (*ip > 17)
    {
        t = (lzo_uint)(*ip++ - 17);
        if (t < 4)
            goto match_next;
        assert(t > 0); NEED_OP(t); NEED_IP(t+1);
        do *op++ = *ip++; while (--t > 0);
        goto first_literal_run;
    }

    while (TEST_IP && TEST_OP)
    {
        t = *ip++;
        if (t >= 16)
            goto match;
        if (t == 0)
        {
            NEED_IP(1);
            while (*ip == 0)
            {
                t += 255;
                ip++;
                NEED_IP(1);
            }
            t += 15 + *ip++;
        }
        assert(t > 0); NEED_OP(t+3); NEED_IP(t+4);
#if defined(LZO_UNALIGNED_OK_4) || defined(LZO_ALIGNED_OK_4)
#if !defined(LZO_UNALIGNED_OK_4)
        if (PTR_ALIGNED2_4(op,ip))
        {
#endif
        COPY4(op,ip);
        op += 4; ip += 4;
        if (--t > 0)
        {
            if (t >= 4)
            {
                do {
                    COPY4(op,ip);
                    op += 4; ip += 4; t -= 4;
                } while (t >= 4);
                if (t > 0) do *op++ = *ip++; while (--t > 0);
            }
            else
                do *op++ = *ip++; while (--t > 0);
        }
#if !defined(LZO_UNALIGNED_OK_4)
        }
        else
#endif
#endif
#if !defined(LZO_UNALIGNED_OK_4)
        {
            *op++ = *ip++; *op++ = *ip++; *op++ = *ip++;
            do *op++ = *ip++; while (--t > 0);
        }
#endif

first_literal_run:

        t = *ip++;
        if (t >= 16)
            goto match;
#if defined(COPY_DICT)
#if defined(LZO1Z)
        m_off = (1 + M2_MAX_OFFSET) + (t << 6) + (*ip++ >> 2);
        last_m_off = m_off;
#else
        m_off = (1 + M2_MAX_OFFSET) + (t >> 2) + (*ip++ << 2);
#endif
        NEED_OP(3);
        t = 3; COPY_DICT(t,m_off)
#else
#if defined(LZO1Z)
        t = (1 + M2_MAX_OFFSET) + (t << 6) + (*ip++ >> 2);
        m_pos = op - t;
        last_m_off = t;
#else
        m_pos = op - (1 + M2_MAX_OFFSET);
        m_pos -= t >> 2;
        m_pos -= *ip++ << 2;
#endif
        TEST_LB(m_pos); NEED_OP(3);
        *op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos;
#endif
        goto match_done;

        do {
match:
            if (t >= 64)
            {
#if defined(COPY_DICT)
#if defined(LZO1X)
                m_off = 1 + ((t >> 2) & 7) + (*ip++ << 3);
                t = (t >> 5) - 1;
#elif defined(LZO1Y)
                m_off = 1 + ((t >> 2) & 3) + (*ip++ << 2);
                t = (t >> 4) - 3;
#elif defined(LZO1Z)
                m_off = t & 0x1f;
                if (m_off >= 0x1c)
                    m_off = last_m_off;
                else
                {
                    m_off = 1 + (m_off << 6) + (*ip++ >> 2);
                    last_m_off = m_off;
                }
                t = (t >> 5) - 1;
#endif
#else
#if defined(LZO1X)
                m_pos = op - 1;
                m_pos -= (t >> 2) & 7;
                m_pos -= *ip++ << 3;
                t = (t >> 5) - 1;
#elif defined(LZO1Y)
                m_pos = op - 1;
                m_pos -= (t >> 2) & 3;
                m_pos -= *ip++ << 2;
                t = (t >> 4) - 3;
#elif defined(LZO1Z)
                {
                    lzo_uint off = t & 0x1f;
                    m_pos = op;
                    if (off >= 0x1c)
                    {
                        assert(last_m_off > 0);
                        m_pos -= last_m_off;
                    }
                    else
                    {
                        off = 1 + (off << 6) + (*ip++ >> 2);
                        m_pos -= off;
                        last_m_off = off;
                    }
                }
                t = (t >> 5) - 1;
#endif
                TEST_LB(m_pos); assert(t > 0); NEED_OP(t+3-1);
                goto copy_match;
#endif
            }
            else if (t >= 32)
            {
                t &= 31;
                if (t == 0)
                {
                    NEED_IP(1);
                    while (*ip == 0)
                    {
                        t += 255;
                        ip++;
                        NEED_IP(1);
                    }
                    t += 31 + *ip++;
                }
#if defined(COPY_DICT)
#if defined(LZO1Z)
                m_off = 1 + (ip[0] << 6) + (ip[1] >> 2);
                last_m_off = m_off;
#else
                m_off = 1 + (ip[0] >> 2) + (ip[1] << 6);
#endif
#else
#if defined(LZO1Z)
                {
                    lzo_uint off = 1 + (ip[0] << 6) + (ip[1] >> 2);
                    m_pos = op - off;
                    last_m_off = off;
                }
#elif defined(LZO_UNALIGNED_OK_2) && defined(LZO_ABI_LITTLE_ENDIAN)
                m_pos = op - 1;
                m_pos -= (* (const lzo_ushortp) ip) >> 2;
#else
                m_pos = op - 1;
                m_pos -= (ip[0] >> 2) + (ip[1] << 6);
#endif
#endif
                ip += 2;
            }
            else if (t >= 16)
            {
#if defined(COPY_DICT)
                m_off = (t & 8) << 11;
#else
                m_pos = op;
                m_pos -= (t & 8) << 11;
#endif
                t &= 7;
                if (t == 0)
                {
                    NEED_IP(1);
                    while (*ip == 0)
                    {
                        t += 255;
                        ip++;
                        NEED_IP(1);
                    }
                    t += 7 + *ip++;
                }
#if defined(COPY_DICT)
#if defined(LZO1Z)
                m_off += (ip[0] << 6) + (ip[1] >> 2);
#else
                m_off += (ip[0] >> 2) + (ip[1] << 6);
#endif
                ip += 2;
                if (m_off == 0)
                    goto eof_found;
                m_off += 0x4000;
#if defined(LZO1Z)
                last_m_off = m_off;
#endif
#else
#if defined(LZO1Z)
                m_pos -= (ip[0] << 6) + (ip[1] >> 2);
#elif defined(LZO_UNALIGNED_OK_2) && defined(LZO_ABI_LITTLE_ENDIAN)
                m_pos -= (* (const lzo_ushortp) ip) >> 2;
#else
                m_pos -= (ip[0] >> 2) + (ip[1] << 6);
#endif
                ip += 2;
                if (m_pos == op)
                    goto eof_found;
                m_pos -= 0x4000;
#if defined(LZO1Z)
                last_m_off = pd((const lzo_bytep)op, m_pos);
#endif
#endif
            }
            else
            {
#if defined(COPY_DICT)
#if defined(LZO1Z)
                m_off = 1 + (t << 6) + (*ip++ >> 2);
                last_m_off = m_off;
#else
                m_off = 1 + (t >> 2) + (*ip++ << 2);
#endif
                NEED_OP(2);
                t = 2; COPY_DICT(t,m_off)
#else
#if defined(LZO1Z)
                t = 1 + (t << 6) + (*ip++ >> 2);
                m_pos = op - t;
                last_m_off = t;
#else
                m_pos = op - 1;
                m_pos -= t >> 2;
                m_pos -= *ip++ << 2;
#endif
                TEST_LB(m_pos); NEED_OP(2);
                *op++ = *m_pos++; *op++ = *m_pos;
#endif
                goto match_done;
            }

#if defined(COPY_DICT)

            NEED_OP(t+3-1);
            t += 3-1; COPY_DICT(t,m_off)

#else

            TEST_LB(m_pos); assert(t > 0); NEED_OP(t+3-1);
#if defined(LZO_UNALIGNED_OK_4) || defined(LZO_ALIGNED_OK_4)
#if !defined(LZO_UNALIGNED_OK_4)
            if (t >= 2 * 4 - (3 - 1) && PTR_ALIGNED2_4(op,m_pos))
            {
                assert((op - m_pos) >= 4);
#else
            if (t >= 2 * 4 - (3 - 1) && (op - m_pos) >= 4)
            {
#endif
                COPY4(op,m_pos);
                op += 4; m_pos += 4; t -= 4 - (3 - 1);
                do {
                    COPY4(op,m_pos);
                    op += 4; m_pos += 4; t -= 4;
                } while (t >= 4);
                if (t > 0) do *op++ = *m_pos++; while (--t > 0);
            }
            else
#endif
            {
copy_match:
                *op++ = *m_pos++; *op++ = *m_pos++;
                do *op++ = *m_pos++; while (--t > 0);
            }

#endif

match_done:
#if defined(LZO1Z)
            t = ip[-1] & 3;
#else
            t = (lzo_uint)(ip[-2] & 3);
#endif
            if (t == 0)
                break;

match_next:
            assert(t > 0); assert(t < 4); NEED_OP(t); NEED_IP(t+1);
#if 0
            do *op++ = *ip++; while (--t > 0);
#else
            *op++ = *ip++;
            if (t > 1) { *op++ = *ip++; if (t > 2) { *op++ = *ip++; } }
#endif
            t = *ip++;
        } while (TEST_IP && TEST_OP);
    }

#if defined(HAVE_TEST_IP) || defined(HAVE_TEST_OP)
    *out_len = pd(op, out);
    return (LZO_E_EOF_NOT_FOUND);
#endif

eof_found:
    assert(t == 1);
    *out_len = pd(op, out);
    return (ip == ip_end ? LZO_E_OK :
           (ip < ip_end  ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));

#if defined(HAVE_NEED_IP)
input_overrun:
    *out_len = pd(op, out);
    return (LZO_E_INPUT_OVERRUN);
#endif

#if defined(HAVE_NEED_OP)
output_overrun:
    *out_len = pd(op, out);
    return (LZO_E_OUTPUT_OVERRUN);
#endif

#if defined(LZO_TEST_OVERRUN_LOOKBEHIND)
lookbehind_overrun:
    *out_len = pd(op, out);
    return (LZO_E_LOOKBEHIND_OVERRUN);
#endif
}

#endif

/***** End of minilzo.c *****/

