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

#ifndef INCLUDED_NV_HARDWARE_ACCESS_H
#define INCLUDED_NV_HARDWARE_ACCESS_H

#include "nvcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

/**
 * NV_WRITE[8|16|32|64] - Writes N data bits to hardware.
 *
 *   @param a The address to write.
 *   @param d The data to write.
 */

/**
 * NV_READ[8|16|32|64] - Reads N bits from hardware.
 *
 * @param a The address to read.
 */

#define NV_WRITE08(a,d)     *((volatile NvU8 *)(a)) = (d)
#define NV_WRITE16(a,d)     *((volatile NvU16 *)(a)) = (d)
#define NV_WRITE32(a,d)     *((volatile NvU32 *)(a)) = (d)
#define NV_WRITE64(a,d)     *((volatile NvU64 *)(a)) = (d)
#define NV_READ8(a)         *((const volatile NvU8 *)(a))
#define NV_READ16(a)        *((const volatile NvU16 *)(a))
#define NV_READ32(a)        *((const volatile NvU32 *)(a))
#define NV_READ64(a)        *((const volatile NvU64 *)(a))
#define NV_WRITE(dst, src, len)  NvOsMemcpy(dst, src, len)
#define NV_READ(dst, src, len)   NvOsMemcpy(dst, src, len)

#ifndef NVB_DEBUG
#define NV_READ32_(a, d) \
   { \
       d = *((const volatile NvU32 *)(a)); \
   }

#define NV_WRITE32_(a,d) \
   { \
      *((volatile NvU32 *)(a)) = (d); \
   }

#else
#define NV_READ32_(a, d) \
   {   \
       d = *((const volatile NvU32 *)(a)); \
       printf(" R reg: 0x%08x, val: 0x%08x\n", \
                (NvU32)a, (NvU32)d);          \
   }

#define NV_WRITE32_(a,d)    \
   { \
      *((volatile NvU32 *)(a)) = (d); \
       printf(" W reg: 0x%08x, val: 0x%08x\n", \
                (NvU32)a, (NvU32)d);          \
   }
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // INCLUDED_NV_HARDWARE_ACCESS_H
