/*
 * (C) Copyright 2010
 * NVIDIA Corporation <www.nvidia.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef INCLUDED_NVCOMMON_H
#define INCLUDED_NVCOMMON_H

// Include headers that provide NULL, size_t, offsetof, and [u]intptr_t.  In
// the event that the toolchain doesn't provide these, provide them ourselves.
#include <stddef.h>

#if defined(__cplusplus)
extern "C"
{
#endif

/** 
 * @defgroup nvcommon Common Declarations
 * 
 * nvcommon.h contains standard definitions used by various interfaces
 * 
 * @{
 */

/* Explicitly sized signed and unsigned ints */
typedef unsigned char      NvU8;  /* 0 to 255 */
typedef unsigned short     NvU16; /* 0 to 65535 */
typedef unsigned int       NvU32; /* 0 to 4294967295 */
typedef unsigned long long NvU64; /* 0 to 18446744073709551615 */
typedef signed char        NvS8;  /* -128 to 127 */
typedef signed short       NvS16; /* -32768 to 32767 */
typedef signed int         NvS32; /* -2147483648 to 2147483647 */
typedef signed long long   NvS64; /* 2^-63 to 2^63-1 */

/* Explicitly sized floats */
typedef float              NvF32; /* IEEE Single Precision (S1E8M23) */
typedef double             NvF64; /* IEEE Double Precision (S1E11M52) */

/* Min/Max values for NvF32 */
#define NV_MIN_F32  (1.1754944e-38f)
#define NV_MAX_F32  (3.4028234e+38f)

/* Boolean type */
enum { NV_FALSE = 0, NV_TRUE = 1 };
typedef NvU8 NvBool;

// Pointer-sized signed and unsigned ints
#if NVCPU_IS_64_BITS
typedef NvU64 NvUPtr;
typedef NvS64 NvSPtr;
#else
typedef NvU32 NvUPtr;
typedef NvS32 NvSPtr;
#endif

/* 
 * Function attributes are lumped in here too
 * INLINE - Make the function inline
 * NAKED - Create a function without a prologue or an epilogue.
 */
#if defined(__GNUC__)

#define NV_INLINE __inline__
#define NV_FORCE_INLINE __attribute__((always_inline)) __inline__
#define NV_NAKED __attribute__((naked))

#elif defined(__arm) // ARM RVDS compiler

#define NV_INLINE __inline
#define NV_FORCE_INLINE __forceinline
#define NV_NAKED __asm

#else
#error Unknown compiler
#endif

/* 
 * Symbol attributes.
 * ALIGN - Variable declaration to a particular # of bytes (should always be a
 *         power of two)
 * WEAK  - Define the symbol weakly so it can be overridden by the user.
 */
#if defined(__GNUC__)
#define NV_ALIGN(size) __attribute__ ((aligned (size)))
#define NV_WEAK __attribute__((weak))    
#elif defined(__arm)
#define NV_ALIGN(size) __align(size)
#define NV_WEAK __weak    
#else
#error Unknown compiler
#endif

/** Macro for determining the size of an array */
#define NV_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/** Macro for taking min or max of a pair of numbers */
#define NV_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define NV_MAX(a,b) (((a) > (b)) ? (a) : (b))

/**
 * By convention, we use this value to represent an infinite wait interval in
 * APIs that expect a timeout argument.  A value of zero should not be
 * interpreted as infinite -- it should be interpreted as "time out immediately
 * and simply check whether the event has already happened."
 */
#define NV_WAIT_INFINITE 0xFFFFFFFF

/**
 * Union that can be used to view a 32-bit word as your choice of a 32-bit
 * unsigned integer, a 32-bit signed integer, or an IEEE single-precision
 * float.  Here is an example of how you might use it to extract the (integer)
 * bitwise representation of a floating-point number:
 *   NvData32 data;
 *   data.f = 1.0f;
 *   printf("%x", data.u);
 */
typedef union NvData32Rec
{
    NvU32 u;
    NvS32 i;
    NvF32 f;
} NvData32;

/**
 * This structure is used to determine a location on a 2-dimensional object,
 * where the coordinate (0,0) is located at the top-left of the object.  The
 * values of x and y are in pixels.
 */
typedef struct NvPointRec
{
    /** horizontal location of the point */
    NvS32 x;

    /** vertical location of the point */
    NvS32 y;
} NvPoint;

/**
 * This structure is used to define a 2-dimensional rectangle where the
 * rectangle is bottom right exclusive (that is, the right most column, and the
 * bottom row of the rectangle is not included).
 */
typedef struct NvRectRec
{
    /** left column of a rectangle */
    NvS32 left;

    /** top row of a rectangle*/
    NvS32 top;

    /** right column of a rectangle */
    NvS32 right;

    /** bottom row of a rectangle */
    NvS32 bottom;        
} NvRect;

/**
 * This structure is used to define a 2-dimensional rectangle
 * relative to some containing rectangle.
 * Rectangle coordinates are normalized to [-1.0...+1.0] range
 */
typedef struct NvRectF32Rec
{
    NvF32 left;
    NvF32 top;
    NvF32 right;
    NvF32 bottom;        
} NvRectF32;

/**
 * This structure is used to define a 2-dimensional surface where the surface is
 * determined by it's height and width in pixels.
 */
typedef struct NvSizeRec
{
    /* width of the surface in pixels */
    NvS32 width;

    /* height of the surface in pixels */
    NvS32 height;
} NvSize;

/** @} */

#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_NVCOMMON_H
