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

#ifndef INCLUDED_NVERROR_H
#define INCLUDED_NVERROR_H

/** 
 * @defgroup nverror Error Handling
 * 
 * nverror.h contains our error code enumeration and helper macros.
 * 
 * @{
 */

/**
 * The NvError enumeration contains ALL return / error codes.  Error codes
 * are specifically explicit to make it easy to identify where an error
 * came from.  
 * 
 * All error codes are derived from the macros in nverrval.h.
 * @ingroup nv_errors 
 */
typedef enum
{
#define NVERROR(_name_, _value_, _desc_) NvError_##_name_ = _value_,
    /* header included for macro expansion of error codes */
    #include "nverrval.h"
#undef NVERROR

    // An alias for success
    NvSuccess = NvError_Success,

    NvError_Force32 = 0x7FFFFFFF
} NvError;

/**
 * A helper macro to check a function's error return code and propagate any
 * errors upward.  This assumes that no cleanup is necessary in the event of
 * failure.  This macro does not locally define its own NvError variable out of
 * fear that this might burn too much stack space, particularly in debug builds
 * or with mediocre optimizing compilers.  The user of this macro is therefore
 * expected to provide their own local variable "NvError e;".
 */
#define NV_CHECK_ERROR(expr) \
    do \
    { \
        e = (expr); \
        if (e != NvSuccess) \
            return e; \
    } while (0)

/**
 * A helper macro to check a function's error return code and, if an error
 * occurs, jump to a label where cleanup can take place.  Like NV_CHECK_ERROR,
 * this macro does not locally define its own NvError variable.  (Even if we
 * wanted it to, this one can't, because the code at the "fail" label probably
 * needs to do a "return e;" to propagate the error upwards.)
 */
#define NV_CHECK_ERROR_CLEANUP(expr) \
    do \
    { \
        e = (expr); \
        if (e != NvSuccess) \
            goto fail; \
    } while (0)

/** @} */
#endif // INCLUDED_NVERROR_H
