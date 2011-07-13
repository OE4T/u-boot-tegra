/*
 * Copyright (c) 2009-2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_T30_H
#define INCLUDED_T30_H

#include <asm/arch/t30/nvcommon.h>
#include <asm/arch/t30/nv_drf.h>
#include <asm/arch/t30/nv_hardware_access.h>

#include <asm/arch/t30/nvbl_memmap_nvap.h>
#include <asm/arch/t30/nvbl_arm_cpsr.h>
#include <asm/arch/t30/nvbl_arm_cp15.h>

#include <asm/arch/t30/arpg.h>
#include <asm/arch/t30/arflow_ctlr.h>
#include <asm/arch/t30/arapbpm.h>
#include <asm/arch/t30/ari2c.h>
#include <asm/arch/t30/arcsite.h>
#include <asm/arch/t30/artimerus.h>
#include <asm/arch/t30/arevp.h>
#include <asm/arch/t30/nvboot_bit.h>
#include <asm/arch/t30/nvboot_version.h>
#include <asm/arch/t30/nvboot_pmc_scratch_map.h>
#include <asm/arch/t30/nvboot_osc.h>
#include <asm/arch/t30/nvboot_clocks.h>

#define PG_UP_PA_BASE       0x60000000  // Base address for arpg.h registers
#define PMC_PA_BASE         0x7000E400  // Base address for arapbpm.h registers
#define CLK_RST_PA_BASE     0x60006000  // Base address for arclk_rst.h registers
#define TIMERUS_PA_BASE     0x60005010  // Base address for artimerus.h registers
#define FLOW_PA_BASE        0x60007000  // Base address for arflow_ctlr.h registers
#define EMC_PA_BASE         0x7000f400  // Base address for aremc.h registers
#define MC_PA_BASE          0x7000f000  // Base address for armc.h registers
#define MISC_PA_BASE        0x70000000  // Base address for arapb_misc.h registers
#define AHB_PA_BASE         0x6000C000  // Base address for arahb_arbc.h registers
#define EVP_PA_BASE         0x6000F000  // Base address for arevp.h registers
#define CSITE_PA_BASE       0x70040000  // Base address for arcsite.h registers
#define FUSE_PA_BASE        0x7000F800  // Base adderss for arfuse.h registers
#define PWRI2C_PA_BASE	    0x7000D000	// Base address for ari2c.h registers

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

#define NV_CAR_REGR(pCar, reg)              NV_READ32( (((NvUPtr)(pCar)) + CLK_RST_CONTROLLER_##reg##_0))
#define NV_CAR_REGW(pCar, reg, val)         NV_WRITE32((((NvUPtr)(pCar)) + CLK_RST_CONTROLLER_##reg##_0), (val))
#define NV_TIMERUS_REGR(pTimer, reg)        NV_READ32((((NvUPtr)(pTimer)) + TIMERUS_##reg##_0))
#define NV_TIMERUS_REGW(pTimer, reg, val)   NV_WRITE32((((NvUPtr)(pTimer)) + TIMERUS_##reg##_0), (val))

#define NV_MISC_REGR(pMisc, reg)                NV_READ32( (((NvUPtr)(pMisc)) + APB_MISC_##reg##_0))
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
#define NVBL_PLLP_KHZ               216000
#define NVBL_USE_PLL_LOCK_BITS      1

/// Calculate clock fractional divider value from reference and target frequencies
#define CLK_DIVIDER(REF, FREQ)  ((((REF) * 2) / FREQ) - 2)

/// Calculate clock frequency value from reference and clock divider value
#define CLK_FREQUENCY(REF, REG)  (((REF) * 2) / (REG + 2))

//------------------------------------------------------------------------------
// Provide missing enumerators for spec files.
//------------------------------------------------------------------------------

#define NV_BIT_ADDRESS 0x40000000
#define NV3P_SIGNATURE 0x5AFEADD8

#if 0 // jz
extern NvBool s_FpgaEmulation;
extern NvU32 s_Netlist;
extern NvU32 s_NetlistPatch;
#endif

/**
 * @brief CPU complex id.
 */
typedef enum NvBlCpuClusterId_t
{
    /// Unknown CPU complex
    NvBlCpuClusterId_Unknown = 0,

    /// Unknown CPU complex
    NvBlCpuClusterId_G,

    /// Unknown CPU complex
    NvBlCpuClusterId_LP,

    /// Ignore -- Forces compilers to make 32-bit enums.
    NvBlCpuClusterId__Force32 = 0x7FFFFFFF

} NvBlCpuClusterId;


#if 0 // jz
static NvBool NvBlIsValidBootRomSignature(const NvBootInfoTable*  pBootInfo);
static NvBool NvBlIsChipInitialized( void );
static NvBool NvBlIsChipInRecovery( void );
#endif 
void NvBlAvpStallUs(NvU32 MicroSec);
void NvBlAvpStallMs(NvU32 MilliSec);
//static NvU32 NvBlGetOscillatorDriveStrength( void );
//static NvU32  NvBlAvpQueryOscillatorFrequency( void );

//static void NvBlAvpEnableCpuClock(NvBool enable);
#if 0
static void NvBlClockInit_T30(NvBool IsChipInitialized);
static void InitPllM(NvBootClocksOscFreq OscFreq);
#endif 
//static void InitPllP(NvBootClocksOscFreq OscFreq);
//static void InitPllU(NvBootClocksOscFreq OscFreq);
//static void NvBlMemoryControllerInit(NvBool IsChipInitialized);
void NvBlAvpSetCpuResetVector(NvU32 reset);
//static NvBool NvBlAvpIsCpuPowered(NvBlCpuClusterId ClusterId);
//static void NvBlAvpRemoveCpuIoClamps(void);
//static void  NvBlAvpPowerUpCpu(void);
//static void NvBlAvpResetCpu(NvBool reset);
#if 0
static void NvBlAvpUnHaltCpu(void);
#endif // jz
//static NvBlCpuClusterId NvBlAvpQueryFlowControllerClusterId(void);

NvBool NvBlAvpInit_T30(NvBool IsRunningFromSdram);
void NvBlStartCpu_T30(NvU32 ResetVector);
//void NvBlAvpHalt(void);
//NvBool NvBlQueryGetNv3pServerFlag_T30(void);

//NV_NAKED void NvBlStartUpAvp_T30( void );
//NV_NAKED void ColdBoot_T30( void );

#define UINT32	NvU32
#define VOID	void
#define BOOLEAN	NvBool
#define TRUE	NV_TRUE
#define FALSE	NV_FALSE

void start_cpu_t30(u32 reset_vector);
#endif // INCLUDED_T30_H




