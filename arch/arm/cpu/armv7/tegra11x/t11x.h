/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_T11X_H
#define INCLUDED_T11X_H

#include <asm/arch/t11x/nvcommon.h>
#include <asm/arch/t11x/nv_drf.h>
#include <asm/arch/t11x/nv_hardware_access.h>

#include <asm/arch/t11x/nvbl_memmap_nvap.h>
#include <asm/arch/t11x/nvbl_arm_cpsr.h>
#include <asm/arch/t11x/nvbl_arm_cp15.h>

#include <asm/arch/t11x/arpg.h>
#include <asm/arch/t11x/arflow_ctlr.h>
#include <asm/arch/t11x/arapbpm.h>
#include <asm/arch/t11x/arapb_misc.h>
#include <asm/arch/t11x/arahb_arbc.h>
#include <asm/arch/t11x/ari2c.h>
#include <asm/arch/t11x/arcsite.h>
#include <asm/arch/t11x/arfuse.h>
#include <asm/arch/t11x/artimerus.h>
#include <asm/arch/t11x/arevp.h>
#include <asm/arch/t11x/nvboot_bit.h>
#include <asm/arch/t11x/nvboot_version.h>
#include <asm/arch/t11x/nvboot_pmc_scratch_map.h>
#include <asm/arch/t11x/nvboot_osc.h>
#include <asm/arch/t11x/nvboot_clocks.h>

#define PG_UP_PA_BASE       0x60000000  // Base address for arpg.h registers
#define TIMERUS_PA_BASE     0x60005010  // Base address for artimerus.h registers
#define CLK_RST_PA_BASE     0x60006000  // Base address for arclk_rst.h registers
#define FLOW_PA_BASE        0x60007000  // Base address for arflow_ctlr.h registers
#define AHB_PA_BASE         0x6000C000  // Base address for arahb_arbc.h registers
#define EVP_PA_BASE         0x6000F000  // Base address for arevp.h registers
#define MISC_PA_BASE        0x70000000  // Base address for arapb_misc.h registers
#define GEN1I2C_PA_BASE     0x7000c000  // Base address for ari2c.h registers
#define PWRI2C_PA_BASE	    0x7000D000	// Base address for ari2c.h registers
#define PMC_PA_BASE         0x7000E400  // Base address for arapbpm.h registers
#define EMC_PA_BASE         0x7000F400  // Base address for aremc.h registers
#define FUSE_PA_BASE        0x7000F800  // Base adderss for arfuse.h registers
#define MC_PA_BASE          0x70019000  // Base address for armc.h registers
#define CSITE_PA_BASE       0x70040000  // Base address for arcsite.h registers
#define USB2_PA_BASE        0x7D004000  // Base address for USB2 registers in a$
#define USB3_PA_BASE        0x7D008000  // Base address for USB3 registers in a$
#define PINMUX_BASE         MISC_PA_BASE

#define FUSE_SKU_DIRECT_CONFIG_0_NUM_DISABLED_CPUS_RANGE  4:3
#define FUSE_SKU_DIRECT_CONFIG_0_DISABLE_ALL_RANGE        5:5

#define NV_PMC_REGR(pCar, reg)          NV_READ32( (((NvUPtr)(pCar)) + APBDEV_PMC_##reg##_0))
#define NV_PMC_REGW(pCar, reg, val)     NV_WRITE32((((NvUPtr)(pCar)) + APBDEV_PMC_##reg##_0), (val))
#define NV_FLOW_REGR(pFlow, reg)        NV_READ32((((NvUPtr)(pFlow)) + FLOW_CTLR_##reg##_0))
#define NV_FLOW_REGW(pFlow, reg, val)   NV_WRITE32((((NvUPtr)(pFlow)) + FLOW_CTLR_##reg##_0), (val))
#define NV_EVP_REGR(pEvp, reg)          NV_READ32( (((NvUPtr)(pEvp)) + EVP_##reg##_0))
#define NV_EVP_REGW(pEvp, reg, val)     NV_WRITE32((((NvUPtr)(pEvp)) + EVP_##reg##_0), (val))

#define USE_PLLC_OUT1               0       // 0 ==> PLLP_OUT4, 1 ==> PLLC_OUT1
#define NVBL_PLL_BYPASS 0
#define NVBL_PLLP_SUBCLOCKS_FIXED   (1)

#define NV_CAR_REGR(pCar, reg)              NV_READ32((((NvUPtr)(pCar)) + CLK_RST_CONTROLLER_##reg##_0))
#define NV_CAR_REGW(pCar, reg, val)         NV_WRITE32((((NvUPtr)(pCar)) + CLK_RST_CONTROLLER_##reg##_0), (val))
#define NV_TIMERUS_REGR(pTimer, reg)        NV_READ32((((NvUPtr)(pTimer)) + TIMERUS_##reg##_0))
#define NV_TIMERUS_REGW(pTimer, reg, val)   NV_WRITE32((((NvUPtr)(pTimer)) + TIMERUS_##reg##_0), (val))

#define NV_MISC_REGR(pMisc, reg)                NV_READ32((((NvUPtr)(pMisc)) + APB_MISC_##reg##_0))
#define NV_MISC_REGW(pMisc, reg, val)           NV_WRITE32((((NvUPtr)(pMisc)) + APB_MISC_##reg##_0), (val))

#define NV_PWRI2C_REGR(pPwrI2c, reg)          	NV_READ32( (((NvUPtr)(pPwrI2c)) + I2C_##reg##_0))
#define NV_PWRI2C_REGW(pPwrI2c, reg, val)     	NV_WRITE32((((NvUPtr)(pPwrI2c)) + I2C_##reg##_0), (val))

#define NV_MC_REGR(pMc, reg)                    NV_READ32( (((NvUPtr)(pMc)) + MC_##reg##_0))
#define NV_MC_REGW(pMc, reg, val)               NV_WRITE32((((NvUPtr)(pMc)) + MC_##reg##_0), (val))

#define NV_EMC_REGR(pEmc, reg)                  NV_READ32( (((NvUPtr)(pEmc)) + EMC_##reg##_0))
#define NV_EMC_REGW(pEmc, reg, val)             NV_WRITE32((((NvUPtr)(pEmc)) + EMC_##reg##_0), (val))

#define NV_AHB_ARBC_REGR(pArb, reg)             NV_READ32( (((NvUPtr)(pArb)) + AHB_ARBITRATION_##reg##_0))
#define NV_AHB_ARBC_REGW(pArb, reg, val)        NV_WRITE32((((NvUPtr)(pArb)) + AHB_ARBITRATION_##reg##_0), (val))

#define NV_MEMC_REGR(pMc, reg)                  NV_READ32( (((NvUPtr)(pMc)) + MEMC_##reg##_0))
#define NV_MEMC_REGW(pMc, reg, val)             NV_WRITE32((((NvUPtr)(pMc)) + MEMC_##reg##_0), (val))

#define NV_AHB_GIZMO_REGR(pAhbGizmo, reg)       NV_READ32( (((NvUPtr)(pAhbGizmo)) + AHB_GIZMO_##reg##_0))
#define NV_AHB_GIZMO_REGW(pAhbGizmo, reg, val)  NV_WRITE32((((NvUPtr)(pAhbGizmo)) + AHB_GIZMO_##reg##_0), (val))

#define NV_CSITE_REGR(pCsite, reg)        NV_READ32( (((NvUPtr)(pCsite)) + CSITE_##reg##_0))
#define NV_CSITE_REGW(pCsite, reg, val)   NV_WRITE32((((NvUPtr)(pCsite)) + CSITE_##reg##_0), (val))

#define NV_FUSE_REGR(pFuse, reg)        NV_READ32( (((NvUPtr)(pFuse)) + FUSE_##reg##_0))

#define NV_CEIL(time, clock)     (((time) + (clock) - 1)/(clock))

#define NVBL_PLLC_KHZ               0
#define NVBL_PLLM_KHZ               167000
#define NVBL_PLLP_KHZ               408000
#define NVBL_USE_PLL_LOCK_BITS      1

// Calculate clock fractional divider value from reference and target frequencies
#define CLK_DIVIDER(REF, FREQ)  ((((REF) * 2) / FREQ) - 2)

// Calculate clock frequency value from reference and clock divider value
#define CLK_FREQUENCY(REF, REG)  (((REF) * 2) / (REG + 2))

#define PMC_CLAMPING_CMD(x)   \
    (NV_DRF_DEF(APBDEV_PMC, REMOVE_CLAMPING_CMD, x, ENABLE))
#define PMC_PWRGATE_STATUS(x) \
    (NV_DRF_DEF(APBDEV_PMC, PWRGATE_STATUS, x, ON))
#define PMC_PWRGATE_TOGGLE(x) \
    (NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, PARTID, x) | \
     NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, START, ENABLE))

#define NV_GEN1I2C_REGR(pGen1I2c, reg)  NV_READ32( (((NvUPtr)(pGen1I2c)) + I2C_##reg##_0))
#define NV_GEN1I2C_REGW(pGen1I2c, reg, val) NV_WRITE32((((NvUPtr)(pGen1I2c)) + I2C_##reg##_0), (val))

#define MC_EMEM_ARB_OVERRIDE_0	0xe8

/**
 * @brief CPU complex id.
 */
typedef enum NvBlCpuClusterId_t {
	/// Unknown CPU complex
	NvBlCpuClusterId_Unknown = 0,

	/// CPU complex 0
	NvBlCpuClusterId_Fast,
	NvBlCpuClusterId_G = NvBlCpuClusterId_Fast,

	/// CPU complex 1
	NvBlCpuClusterId_Slow,
	NvBlCpuClusterId_LP = NvBlCpuClusterId_Slow,

	/// Ignore -- Forces compilers to make 32-bit enums.
	NvBlCpuClusterId__Force32 = 0x7FFFFFFF
} NvBlCpuClusterId;

void NvBlAvpStallUs(NvU32 MicroSec);
void NvBlAvpStallMs(NvU32 MilliSec);
void NvBlAvpSetCpuResetVector(NvU32 reset);
void NvBlStartCpu_T11x(NvU32 ResetVector);

#define UINT32	NvU32
#define VOID	void
#define BOOLEAN	NvBool
#define TRUE	NV_TRUE
#define FALSE	NV_FALSE
#if defined(__arm__)	// ARM GNU compiler

// Define the standard ARM coprocessor register names because the ARM compiler requires
// that we use the names and the GNU compiler requires that we use the numbers.
#define p14 	14
#define p15 	15
#define c0		0
#define c1		1
#define c2		2
#define c3		3
#define c4		4
#define c5		5
#define c6		6
#define c7		7
#define c8		8
#define c9		9
#define c10 	10
#define c11 	11
#define c12 	12
#define c13 	13
#define c14 	14
#define c15 	15

// Macro used in C code.compiler specific
/*
  *  @brief Macro to abstract writing of a ARM coprocessor register via the MCR instruction.
  *  @param cp is the coprocessor name (e.g., p15)
  *  @param op1 is a coprocessor-specific operation code (must be a manifest constant).
  *  @param Rd is a variable that will receive the value read from the coprocessor register.
  *  @param CRn is the destination coprocessor register (e.g., c7).
  *  @param CRm is an additional destination coprocessor register (e.g., c2).
  *  @param op2 is a coprocessor-specific operation code (must be a manifest constant).
 */
 #define MCR(cp,op1,Rd,CRn,CRm,op2)  asm(" MCR " #cp",%1,%2,"#CRn","#CRm ",%5" \
	 : : "i" (cp), "i" (op1), "r" (Rd), "i" (CRn), "i" (CRm), "i" (op2))

 /*
  *  @brief Macro to abstract reading of a ARM coprocessor register via the MRC instruction.
  *  @param cp is the coprocessor name (e.g., p15)
  *  @param op1 is a coprocessor-specific operation code (must be a manifest constant).
  *  @param Rd is a variable that will receive the value read from the coprocessor register.
  *  @param CRn is the source coprocessor register (e.g., c7).
  *  @param CRm is an additional source coprocessor register (e.g., c2).
  *  @param op2 is a coprocessor-specific operation code (must be a manifest constant).
 */
 #define MRC(cp,op1,Rd,CRn,CRm,op2)  asm( " MRC " #cp",%2,%0," #CRn","#CRm",%5" \
	 : "=r" (Rd) : "i" (cp), "i" (op1), "i" (CRn), "i" (CRm), "i" (op2))
#endif
void start_cpu_t11x(u32 reset_vector);
#endif // INCLUDED_T11X_H
