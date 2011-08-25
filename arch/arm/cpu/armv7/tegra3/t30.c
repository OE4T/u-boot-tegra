/** @file
  C Entry point for the SEC. First C code after the reset vector.

  Copyright (c) 2008 - 2009, Nvidia Inc. All rights reserved.<BR>
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#if 0
#include <PiPei.h>

#include <artimerus.h>
#include <arclk_rst.h>
#include <artimerus.h>
#include <arcsite.h>
#include <arflow_ctlr.h>
#include <arapb_misc.h>
#include <arfuse.h>
#include <arapbpm.h>
#include <arcsite.h>
#include <arevp.h>
#include <ari2c.h>
#include <arscu.h>
#include <arapb_misc.h>
#include <NvMacro.h>
#include <NvRm/nvrm_drf.h>
#include <NvRm/reg_access.h>

#include "LzmaDecompress.h"
#include "LnvBootRomStruct.h"
#include "Library/NvTegraVersionLib.h"
#include "Library/NvTegraMpLib.h"
#else

#include <common.h>
#include "t30.h"

#define NV_ASSERT(p) \
{    if ((p) == 0) { \
        t30_WriteDebugString("t30_cold!\n"); \
    };               \
}

#endif

#define PMU_IGNORE_PWRREQ 1


UINT32 NvBlAvpQueryBootCpuFrequency( void )
{

    return 1000000;
}

VOID AvpStallUs(UINT32 MicroSec)
{
  UINT32           Reg;            // Flow controller register
  UINT32           Delay;          // Microsecond delay time
  UINT32           MaxUs;          // Maximum flow controller delay

  // Get the maxium delay per loop.
  MaxUs = NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, ZERO, 0xFFFFFFFF);

  //Bug 589922. Add 1 to requested value as the flow controller just decrements ZERO counter on
  //every usec timer tick; hence the resulting delay varies randomly between N-1 and N (N - is requested delay).

  MicroSec += 1;

  while (MicroSec)
  {
      Delay     = (MicroSec > MaxUs) ? MaxUs : MicroSec;
      MicroSec -= Delay;

      Reg = NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, ZERO, Delay)
          | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, uSEC, 1)
          | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, MODE, 2);

      NV_FLOW_REGW(FLOW_PA_BASE, HALT_COP_EVENTS, Reg);
  }
}

VOID AvpStallMs(UINT32 MilliSec)
{
  UINT32           Reg;            // Flow controller register
  UINT32           Delay;          // Millisecond delay time
  UINT32           MaxMs;          // Maximum flow controller delay

  // Get the maxium delay per loop.
  MaxMs = NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, ZERO, 0xFFFFFFFF);

  //Bug 589922. Add 1 to requested value as the flow controller just decrements ZERO counter on
  //every MSEC timer tick; hence the resulting delay varies randomly between N-1 and N (N - is requested delay).

  MilliSec += 1;

  while (MilliSec)
  {
      Delay     = (MilliSec > MaxMs) ? MaxMs : MilliSec;
      MilliSec -= Delay;

      Reg = NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, ZERO, Delay)
          | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, MSEC, 1)
          | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, MODE, 2);

      NV_FLOW_REGW(FLOW_PA_BASE, HALT_COP_EVENTS, Reg);
  }
}

VOID AvpEnableCpuPowerRail(void)
{
  UINT32   Reg;        // Scratch reg

  Reg = NV_PMC_REGR(PMC_PA_BASE, CNTRL);
  Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, CNTRL, CPUPWRREQ_OE, ENABLE, Reg);
  NV_PMC_REGW(PMC_PA_BASE, CNTRL, Reg);

#if PMU_IGNORE_PWRREQ
  Reg = 0x005A;
  NV_PWRI2C_REGW(PWRI2C_PA_BASE, I2C_CMD_ADDR0, Reg);
  Reg = 0x0002;
  NV_PWRI2C_REGW(PWRI2C_PA_BASE, I2C_CNFG, Reg);

  Reg = 0x2328;
  NV_PWRI2C_REGW(PWRI2C_PA_BASE, I2C_CMD_DATA1, Reg);
  Reg = 0x0A02;
  NV_PWRI2C_REGW(PWRI2C_PA_BASE, I2C_CNFG, Reg);
  AvpStallMs(1);

  Reg = 0x0127;
  NV_PWRI2C_REGW(PWRI2C_PA_BASE, I2C_CMD_DATA1, Reg);
  Reg = 0x0A02;
  NV_PWRI2C_REGW(PWRI2C_PA_BASE, I2C_CNFG, Reg);

  AvpStallMs(10);
#endif
}

VOID AvpHaltCpu(BOOLEAN halt)
{
  UINT32   Reg;

  if (halt)
  {
      Reg = NV_DRF_DEF(FLOW_CTLR, HALT_CPU_EVENTS, MODE, FLOW_MODE_STOP);
  }
  else
  {
      Reg = NV_DRF_DEF(FLOW_CTLR, HALT_CPU_EVENTS, MODE, FLOW_MODE_NONE);
  }

  NV_FLOW_REGW(FLOW_PA_BASE, HALT_CPU_EVENTS, Reg);
}

VOID AvpResetCpu(BOOLEAN reset)
{
  UINT32   Reg;    // Scratch reg
  UINT32   Cpu;    // Scratch reg

  //-------------------------------------------------------------------------
  // NOTE:  Regardless of whether the request is to hold the CPU in reset or
  //        take it out of reset, every processor in the CPU complex except
  //        the master (CPU 0) will be held in reset because the AVP only
  //        talks to the master. The AVP does not know, nor does it need to
  //        know, that there are multiple processors in the CPU complex.
  //-------------------------------------------------------------------------

  // Hold CPU 1-3 in reset.
  Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CPURESET1, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DBGRESET1, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DERESET1,  1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CPURESET2, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DBGRESET2, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DERESET2,  1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CPURESET3, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DBGRESET3, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DERESET3,  1);
  NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPU_CMPLX_SET, Cpu);

  if (reset)
  {
      // Place CPU0 into reset.
      Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CPURESET0, 1)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DBGRESET0, 1)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DERESET0,  1);
      NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPU_CMPLX_SET, Cpu);

      // Enable master CPU reset.
      Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_L_SET, SET_CLK_ENB_CPU, 1);
      NV_CAR_REGW(CLK_RST_PA_BASE, CLK_ENB_L_SET, Reg);
  }
  else
  {
      // Take CPU0 out of reset.
      Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_CPURESET0, 1)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_DBGRESET0, 1)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_DERESET0,  1);
      NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPU_CMPLX_CLR, Cpu);

      // Disable master CPU reset.
      Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_L_CLR, CLR_CPU_RST, 1);
      NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEV_L_CLR, Reg);
  }
}

VOID AvpEnableCpuClock(BOOLEAN enable)
{
  UINT32   Reg;        // Scratch reg
  UINT32   Clk;        // Scratch reg

  //-------------------------------------------------------------------------
  // NOTE:  Regardless of whether the request is to enable or disable the CPU
  //        clock, every processor in the CPU complex except the master (CPU
  //        0) will have it's clock stopped because the AVP only talks to the
  //        master. The AVP, it does not know, nor does it need to know that
  //        there are multiple processors in the CPU complex.
  //-------------------------------------------------------------------------

  // Always halt CPU 1-3 at the flow controller so that in uni-processor
  // configurations the low-power trigger conditions will work properly.
  Reg = NV_DRF_DEF(FLOW_CTLR, HALT_CPU1_EVENTS, MODE, FLOW_MODE_STOP);
  NV_FLOW_REGW(FLOW_PA_BASE, HALT_CPU1_EVENTS, Reg);
  NV_FLOW_REGW(FLOW_PA_BASE, HALT_CPU2_EVENTS, Reg);
  NV_FLOW_REGW(FLOW_PA_BASE, HALT_CPU3_EVENTS, Reg);

  // Enabling clock?
  if (enable)
  {

          // Wait for PLL-X to lock.
          do
          {
              Reg = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_BASE);
          } while (!NV_DRF_VAL(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, Reg));


      //*((volatile long *)0x60006020) = 0x20008888 ;// CCLK_BURST_POLICY
      Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_STATE, 0x2)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, COP_AUTO_CWAKEUP_FROM_FIQ, 0x0)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_AUTO_CWAKEUP_FROM_FIQ, 0x0)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, COP_AUTO_CWAKEUP_FROM_IRQ, 0x0)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_AUTO_CWAKEUP_FROM_IRQ, 0x0)
          | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_FIQ_SOURCE, PLLX_OUT0)
          | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_IRQ_SOURCE, PLLX_OUT0)
          | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_RUN_SOURCE, PLLX_OUT0)
          | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_IDLE_SOURCE, PLLX_OUT0);
      NV_CAR_REGW(CLK_RST_PA_BASE, CCLK_BURST_POLICY, Reg);
  }

  //*((volatile long *)0x60006024) = 0x80000000 ;// SUPER_CCLK_DIVIDER
  Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_ENB, 0x1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_COP_FIQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_CPU_FIQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_COP_IRQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_CPU_IRQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIVIDEND, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIVISOR, 0x0);
  NV_CAR_REGW(CLK_RST_PA_BASE, SUPER_CCLK_DIVIDER, Reg);

  // Stop the clock to all CPUs.
  Clk = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_SET, SET_CPU0_CLK_STP, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_SET, SET_CPU1_CLK_STP, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_SET, SET_CPU2_CLK_STP, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_SET, SET_CPU3_CLK_STP, 1);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_CPU_CMPLX_SET, Clk);

  // Enable the CPU0 clock if requested.
  if (enable)
  {
      Clk = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_CLR, CLR_CPU0_CLK_STP, 1);
      NV_CAR_REGW(CLK_RST_PA_BASE, CLK_CPU_CMPLX_CLR, Clk);
  }

  // Always enable the main CPU complex clock.
  Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_L_SET, SET_CLK_ENB_CPU, 1);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_ENB_L_SET, Reg);
}

VOID AvpClockEnableCorsight(BOOLEAN enable)
{
  UINT32   Reg;        // Scratch register

  if (enable)
  {
      // Put CoreSight on PLLP_OUT0 (216 MHz) and divide it down by 1.5
      // giving an effective frequency of 144MHz.

      {
          // Program the CoreSight clock source and divider (but not at the same time).
          // Note that CoreSight has a fractional divider (LSB == .5).
          Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_SOURCE_CSITE);
          Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_CSITE,
#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
		/*
		 * Clock divider request for 204MHz would setup CSITE clock as
		 * 144MHz for PLLP base 216MHz, and 204MHz for PLLP base 408MHz
		 */
		CSITE_CLK_DIVISOR, CLK_DIVIDER(NVBL_PLLP_KHZ, 204000), Reg);
#else
		CSITE_CLK_DIVISOR, CLK_DIVIDER(NVBL_PLLP_KHZ, 144000), Reg);
#endif
          NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_CSITE, Reg);

          AvpStallUs(3);

          Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_CSITE,
                  CSITE_CLK_SRC, PLLP_OUT0, Reg);
          NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_CSITE, Reg);
      }

      // Enable CoreSight clock and take the module out of reset in case
      // the clock is not enabled or the module is being held in reset.
      Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_U_SET, SET_CLK_ENB_CSITE, 1);
      NV_CAR_REGW(CLK_RST_PA_BASE, CLK_ENB_U_SET, Reg);

      Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_U_CLR, CLR_CSITE_RST, 1);
      NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEV_U_CLR, Reg);

      // Unlock the CPU CoreSight interface for CPU0 be cause CPU0
      // is always present.
      NV_CSITE_REGW(CSITE_PA_BASE, CPUDBG0_LAR, 0xC5ACCE55);

      // Find out which CPU complex we're booting to.
      Reg = NV_FLOW_REGR(FLOW_PA_BASE, CLUSTER_CONTROL);
      Reg = NV_DRF_VAL(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, Reg);

      if (Reg == NV_DRF_DEF(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, G))
      {
          // Unlock all CPU CoreSight interfaces for CPU1-CPU3.
          NV_CSITE_REGW(CSITE_PA_BASE, CPUDBG1_LAR, 0xC5ACCE55);
          NV_CSITE_REGW(CSITE_PA_BASE, CPUDBG2_LAR, 0xC5ACCE55);
          NV_CSITE_REGW(CSITE_PA_BASE, CPUDBG3_LAR, 0xC5ACCE55);
      }
  }
  else
  {
      // Disable CoreSight clock and hold it in reset.
      Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_U_CLR, CLR_CLK_ENB_CSITE, 1);
      NV_CAR_REGW(CLK_RST_PA_BASE, CLK_ENB_U_CLR, Reg);

      Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_U_SET, SET_CSITE_RST, 1);
      NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEV_U_SET, Reg);
  }
}

VOID AvpSetCpuResetVector(UINT32 reset)
{
  NV_EVP_REGW(EVP_PA_BASE, CPU_RESET_VECTOR, reset);
}

BOOLEAN AvpIsCpuPowered(NvBlCpuClusterId ClusterId)
{
  UINT32   Reg;        // Scratch reg
  UINT32   Mask;       // Mask

  // Get power gate status.
  Reg = NV_PMC_REGR(PMC_PA_BASE, PWRGATE_STATUS);

  // WARNING: Do not change the way this is coded (BUG 626323)
  //          Otherwise, the compiler can generate invalid ARMv4i instructions
  Mask = (ClusterId == NvBlCpuClusterId_LP) ?
          NV_DRF_DEF(APBDEV_PMC, PWRGATE_STATUS, A9LP, ON) :
          NV_DRF_DEF(APBDEV_PMC, PWRGATE_STATUS, CPU, ON);

  if (Reg & Mask)
  {
      return TRUE;
  }

  return FALSE;
}

VOID AvpRemoveCpuIoClamps(void)
{
  UINT32   Reg;        // Scratch reg

  // Always remove the clamps on the LP CPU I/O signals.
  Reg = NV_DRF_DEF(APBDEV_PMC, REMOVE_CLAMPING_CMD, A9LP, ENABLE);
  NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);

  // If booting the G CPU cluster...
      // ... remove the clamps on the G CPU0 I/O signals as well.
      Reg = NV_DRF_DEF(APBDEV_PMC, REMOVE_CLAMPING_CMD, CPU, ENABLE);
      NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);

  // Give I/O signals time to stabilize.
  AvpStallUs(20);  // !!!FIXME!!! THIS TIME HAS NOT BEEN CHARACTERIZED
}

VOID AvpPowerUpCpu(void)
{
  UINT32   Reg;        // Scratch reg

  // Always power up the LP cluster
  if (!AvpIsCpuPowered(NvBlCpuClusterId_LP))
  {
      // Toggle the LP CPU power state (OFF -> ON).
      Reg = NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, PARTID, A9LP)
          | NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, START, ENABLE);
      NV_PMC_REGW(PMC_PA_BASE, PWRGATE_TOGGLE, Reg);

      // Wait for the power to come up.
      while (!AvpIsCpuPowered(NvBlCpuClusterId_LP))
      {
          // Do nothing
      }
  }

  // Are we booting to the G CPU cluster?
      if (!AvpIsCpuPowered(NvBlCpuClusterId_G))
      {
          // Toggle the G CPU0 power state (OFF -> ON).
          Reg = NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, PARTID, CP)
              | NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, START, ENABLE);
          NV_PMC_REGW(PMC_PA_BASE, PWRGATE_TOGGLE, Reg);
      }

      // Wait for the power to come up.
      while (!AvpIsCpuPowered(NvBlCpuClusterId_G))
      {
          // Do nothing
      }

  // Remove the I/O clamps from CPU power partition.
  AvpRemoveCpuIoClamps();
}

VOID AvpHaltAp20(void)
{
  UINT32   Reg;    // Scratch reg

  for (;;)
  {
      Reg = NV_DRF_DEF(FLOW_CTLR, HALT_COP_EVENTS, MODE, FLOW_MODE_STOP)
          | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, JTAG, 1)
          | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, IRQ_1, 1)
          | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, FIQ_1, 1);
      NV_FLOW_REGW(FLOW_PA_BASE, HALT_COP_EVENTS, Reg);
  }
}

VOID InitPllP(NvBootClocksOscFreq OscFreq)
{
	UINT32   Base, Misc;
#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
	UINT32 OutA, OutB;
#endif
	Misc = Base = 0;

	/*
	 * DIVP/DIVM/DIVN values taken from arclk_rst.h table for fixed
	 *  216MHz or 408MHz operation.
	 */
      switch (OscFreq)
      {
          case NvBootClocksOscFreq_13:
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
		| NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x0)
		| NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0x198)
#else
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0xD8)
#endif
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0xD);
              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLC_MISC, PLLC_CPCON, 8);
              break;

          case NvBootClocksOscFreq_38_4:
              // Fall through -- 38.4 MHz is identical to 19.2 MHz except
              // that the PLL reference divider has been previously set
              // to divide by 2 for 38.4 MHz.

          case NvBootClocksOscFreq_19_2:
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
		| NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x0)
		| NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0x154)
#else
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0xB4)
#endif
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0x16);
              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLC_MISC, PLLC_CPCON, 4);
              break;

          case NvBootClocksOscFreq_12:
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
		| NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x0)
		| NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0x198)
#else
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
//                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0xD8)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0x1B0)
#endif
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0x0C);
//              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLC_MISC, PLLC_CPCON, 8);
      Misc = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 8, Misc);
  Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_LOCK_ENABLE, DISABLE, Misc);
              break;

          case NvBootClocksOscFreq_26:
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
		| NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x0)
		| NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0x198)
#else
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0xD8)
#endif
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0x1A);
              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLC_MISC, PLLC_CPCON, 8);
              break;

          case NvBootClocksOscFreq_16_8:
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLC_BASE, PLLC_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLC_BASE, PLLC_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLC_BASE, PLLC_REF_DIS, REF_ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
		| NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x0)
		| NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0x154)
#else
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0xB4)
#endif
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0x14);
              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLC_MISC, PLLC_CPCON, 4);
              break;

          default:
              break;
      }

  NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_MISC, Misc);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Base);

  // disable override
  Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, DISABLE, Base);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Base);

  // reenable PLLP
  Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, ENABLE, Base);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Base);

  // Give clocks time to stabilize.
  AvpStallUs(20);

  // disable bypass
  Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, DISABLE, Base);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Base);

#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
	/* Set pllp_out1,2,3,4 to frequencies 9.6MHz, 48MHz, 102MHz, 204MHz */
	OutA = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_RATIO, 83) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_OVRRIDE, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_CLKEN, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_RSTN,
		RESET_DISABLE) |

	NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_RATIO, 15) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_OVRRIDE, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_CLKEN, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_RSTN,
		RESET_DISABLE);

	OutB = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_RATIO, 6) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_OVRRIDE, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_CLKEN, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_RSTN,
		RESET_DISABLE) |

	NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_RATIO, 2) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_OVRRIDE, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_CLKEN, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_RSTN,
		RESET_DISABLE);

	NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_OUTA, OutA);
	NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_OUTA, OutB);
#endif

  // lock enable
  Misc = NV_CAR_REGR(CLK_RST_PA_BASE, PLLP_MISC);
  Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_LOCK_ENABLE, ENABLE, Misc);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_MISC, Misc);

  // Wait for PLL-P to lock.
  do
  {
      Base = NV_CAR_REGR(CLK_RST_PA_BASE, PLLP_BASE);
  } while (!NV_DRF_VAL(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, Base));
}

VOID InitPllX(NvBootClocksOscFreq OscFreq)
{
  UINT32               Base;       // PLL Base
  UINT32               Misc;       // PLL Misc
  UINT32               Divm;       // Reference
  UINT32               Divn;       // Multiplier
  UINT32               Divp;       // Divider == Divp ^ 2

  // Is PLL-X already running?
  Base = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_BASE);
  Base = NV_DRF_VAL(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, Base);
  if (Base == NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, ENABLE))
  {
      return;
  }

  Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_MISC, PLLX_CPCON, 1);

  {
      Divm = 1;
      Divp = 0;
      Divn = NvBlAvpQueryBootCpuFrequency() / 1000;

      // Above 600 MHz, set DCCON in the PLLX_MISC register.
      if (Divn > 600)
      {
          Misc |= NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_MISC, PLLX_DCCON, 1);
      }

      // Operating below the 50% point of the divider's range?
      if (Divn <= (NV_DRF_VAL(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, ~0)/2))
      {
          // Yes, double the post divider and the feedback divider.
          Divp = 1;
          Divn <<= Divp;
      }
      // Operating above the range of the feedback divider?
      else if (Divn > NV_DRF_VAL(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, ~0))
      {
          // Yes, double the input divider and halve the feedback divider.
          Divn >>= 1;
          Divm = 2;
      }

      // Program PLL-X.
      switch (OscFreq)
      {
          case NvBootClocksOscFreq_13:
              Divm = (Divm == 1) ? 13 : (13 / Divm);
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_REF_DIS, REF_ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, 0x0)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVP, Divp)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, Divn)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVM, Divm);
              break;

          case NvBootClocksOscFreq_38_4:
              // Fall through -- 38.4 MHz is identical to 19.2 MHz except
              // that the PLL reference divider has been previously set
              // to divide by 2 for 38.4 MHz.

          case NvBootClocksOscFreq_19_2:
              // NOTE: With a 19.2 or 38.4 MHz oscillator, the PLL will run
              //       1.05% faster than the target frequency because we cheat
              //       on the math to make this simple.
              Divm = (Divm == 1) ? 19 : (19 / Divm);
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_REF_DIS, REF_ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, 0x0)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVP, Divp)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, Divn)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVM, Divm);
              break;

          case NvBootClocksOscFreq_48:
              // Fall through -- 48 MHz is identical to 12 MHz except
              // that the PLL reference divider has been previously set
              // to divide by 4 for 48 MHz.

          case NvBootClocksOscFreq_12:
              Divm = (Divm == 1) ? 12 : (12 / Divm);
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_REF_DIS, REF_ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, 0x0)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVP, Divp)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, Divn)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVM, Divm);
              break;

          case NvBootClocksOscFreq_26:
              Divm = (Divm == 1) ? 26 : (26 / Divm);
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_REF_DIS, REF_ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, 0x0)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVP, Divp)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, Divn)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVM, Divm);
              break;

          case NvBootClocksOscFreq_16_8:
              // NOTE: With a 16.8 MHz oscillator, the PLL will run 1.18%
              //       slower than the target frequency because we cheat
              //       on the math to make this simple.
              Divm = (Divm == 1) ? 17 : (17 / Divm);
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_REF_DIS, REF_ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, 0x0)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVP, Divp)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, Divn)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVM, Divm);
              break;

          default:
              break;
      }
  }

  NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_MISC, Misc);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Base);

  Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, ENABLE, Base);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Base);

  Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, DISABLE, Base);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Base);

  Misc = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_MISC);
  Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_MISC, PLLX_LOCK_ENABLE, ENABLE, Misc);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_MISC, Misc);
}

VOID InitPllU(NvBootClocksOscFreq OscFreq)
{
  UINT32   Base;
  UINT32   Misc = 0;
  UINT32   isEnabled;
  UINT32   isBypassed;

  Base = NV_CAR_REGR(CLK_RST_PA_BASE, PLLU_BASE);
  isEnabled  = NV_DRF_VAL(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, Base);
  isBypassed = NV_DRF_VAL(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, Base);

  if (isEnabled && !isBypassed )
  {
      // Make sure lock enable is set.
      Misc = NV_CAR_REGR(CLK_RST_PA_BASE, PLLU_MISC);
      Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLU_MISC, PLLU_LOCK_ENABLE, ENABLE, Misc);
      NV_CAR_REGW(CLK_RST_PA_BASE, PLLU_MISC, Misc);
      return;
  }

  {
      // DIVP/DIVM/DIVN values taken from arclk_rst.h table
      switch (OscFreq)
      {
          case NvBootClocksOscFreq_13:
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_REF_DIS, REF_ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_LOCK, 0x0)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_OVERRIDE, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_ICUSB, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_HSIC, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_USB, DEFAULT)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_VCO_FREQ, 0)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVN, 0x3C0)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVM, 0xD);
              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 0xC)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_LFCON, 0x1);
              break;

          case NvBootClocksOscFreq_38_4:
              // Fall through -- 38.4 MHz is identical to 19.2 MHz except
              // that the PLL reference divider has been previously set
              // to divide by 2 for 38.4 MHz.

          case NvBootClocksOscFreq_19_2:
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_REF_DIS, REF_ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_LOCK, 0x0)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_OVERRIDE, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_ICUSB, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_HSIC, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_USB, DEFAULT)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_VCO_FREQ, 0)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVN, 0xC8)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVM, 0x4);
              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 0x3)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_LFCON, 0x0);
              break;

          case NvBootClocksOscFreq_48:
              // Fall through -- 48 MHz is identical to 12 MHz except
              // that the PLL reference divider has been previously set
              // to divide by 4 for 48 MHz.

          case NvBootClocksOscFreq_12:
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_REF_DIS, REF_ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_LOCK, 0x0)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_OVERRIDE, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_ICUSB, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_HSIC, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_USB, DEFAULT)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_VCO_FREQ, 0)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVN, 0x3C0)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVM, 0xC);
              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 0xC)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_LFCON, 0x1);
              break;

          case NvBootClocksOscFreq_26:
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_REF_DIS, REF_ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_LOCK, 0x0)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_OVERRIDE, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_ICUSB, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_HSIC, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_USB, DEFAULT)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_VCO_FREQ, 0)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVN, 0x3C0)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVM, 0x1A);
              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 0xC)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_LFCON, 0x1);
              break;

          case NvBootClocksOscFreq_16_8:
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_REF_DIS, REF_ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_LOCK, 0x0)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_OVERRIDE, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_ICUSB, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_HSIC, DEFAULT)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_CLKENABLE_USB, DEFAULT)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_VCO_FREQ, 0)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVN, 0x190)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_DIVM, 0x7);
              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 0x5)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_LFCON, 0x0);
              break;

          default:
              break;
      }
  }

  NV_CAR_REGW(CLK_RST_PA_BASE, PLLU_MISC, Misc);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLU_BASE, Base);

  Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_ENABLE, ENABLE, Base);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLU_BASE, Base);

  Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLU_BASE, PLLU_BYPASS, DISABLE, Base);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLU_BASE, Base);

  Misc = NV_CAR_REGR(CLK_RST_PA_BASE, PLLU_MISC);
  Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLU_MISC, PLLU_LOCK_ENABLE, ENABLE, Misc);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLU_MISC, Misc);

}

NvBlCpuClusterId NvBlAvpQueryBootCpuClusterId(void)
{
  return NvBlCpuClusterId_G;
}

#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
void set_avp_clock_to_clkm(void)
{
	UINT32 Reg;

	/* Use CLKM as AVP clock source */
	Reg = NV_CAR_REGR(CLK_RST_PA_BASE, SCLK_BURST_POLICY);
	Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY,
		SYS_STATE, RUN, Reg);
	Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY,
		SWAKEUP_RUN_SOURCE, CLKM, Reg);
	NV_CAR_REGW(CLK_RST_PA_BASE, SCLK_BURST_POLICY, Reg);
	AvpStallUs(3);
}
#endif

//VOID ClockInitT30( const NvRmChipId* ChipId)
VOID ClockInitT30(void)
{
  const NvBootInfoTable*  pBootInfo;      // Boot Information Table pointer
  NvBootClocksOscFreq     OscFreq;        // Oscillator frequency
  UINT32                   Reg;            // Temporary register
//  UINT32                   OscCtrl;        // Oscillator control register
//  UINT32                   OscStrength;    // Oscillator Drive Strength
  UINT32                   UsecCfg;        // Usec timer configuration register
  UINT32                   PllRefDiv;      // PLL reference divider
  NvBlCpuClusterId        CpuClusterId;   // Boot CPU cluster id

  // Get a pointer to the Boot Information Table.
  pBootInfo = (const NvBootInfoTable*)(T30_BASE_PA_BOOT_INFO);

  // Get the oscillator frequency.
  OscFreq = pBootInfo->OscFrequency;

  // For most oscillator frequencies, the PLL reference divider is 1.
  // Frequencies that require a different reference divider will set
  // it below.
  PllRefDiv = CLK_RST_CONTROLLER_OSC_CTRL_0_PLL_REF_DIV_DIV1;

  // Set up the oscillator dependent registers.
  // NOTE: Don't try to replace this switch statement with an array lookup.
  //       Can't use global arrays here because the data segment isn't valid yet.
  switch (OscFreq)
  {
      default:
          // Fall Through -- this is what the boot ROM does.
      case NvBootClocksOscFreq_13:
          UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (1-1))
                  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (13-1));
          break;

      case NvBootClocksOscFreq_19_2:
          UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (5-1))
                  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (96-1));
          break;

      case NvBootClocksOscFreq_12:
          UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (1-1))
                  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (12-1));
          break;

      case NvBootClocksOscFreq_26:
          UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (1-1))
                  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (26-1));
          break;

      case NvBootClocksOscFreq_16_8:
          UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (5-1))
                  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (84-1));
          break;

      case NvBootClocksOscFreq_38_4:
          UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (5-1))
                  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (192-1));
          PllRefDiv = CLK_RST_CONTROLLER_OSC_CTRL_0_PLL_REF_DIV_DIV2;
          break;

      case NvBootClocksOscFreq_48:
          UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (1-1))
                  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (48-1));
          PllRefDiv = CLK_RST_CONTROLLER_OSC_CTRL_0_PLL_REF_DIV_DIV4;
          break;
  }

  // Find out which CPU cluster we're booting to.
  CpuClusterId = NvBlAvpQueryBootCpuClusterId();

  Reg = NV_FLOW_REGR(FLOW_PA_BASE, CLUSTER_CONTROL);
  // Are we booting to LP CPU complex?
  if (CpuClusterId == NvBlCpuClusterId_LP)
  {
      // Set active CPU cluster to LP.
      Reg = NV_FLD_SET_DRF_DEF(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, LP, Reg);
  }
  else
  {
      // Set active CPU cluster to G.
      Reg = NV_FLD_SET_DRF_DEF(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, G, Reg);
  }
  NV_FLOW_REGW(FLOW_PA_BASE, CLUSTER_CONTROL, Reg);

  //-------------------------------------------------------------------------
  // If the boot ROM hasn't performed the PLLM initialization, we have to
  // do it here and always reconfigure PLLP.
  //-------------------------------------------------------------------------

//  if (!IsChipInitialized)
//  {
//      // Program the microsecond scaler.
//      NV_TIMERUS_REGW(TIMERUS_PA_BASE, USEC_CFG, UsecCfg);
//
//      // Get Oscillator Drive Strength Setting
//      OscStrength = NvBlGetOscillatorDriveStrength();
//
//      // Program the oscillator control register.
//      OscCtrl = NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, OSC_FREQ, (int)OscFreq)
//              | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, PLL_REF_DIV, PllRefDiv)
//              | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, OSCFI_SPARE, 0)
//              | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XODS, 0)
//              | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOFS, OscStrength)
//              | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOBP, 0)
//              | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOE, 1);
//      NV_CAR_REGW(CLK_RST_PA_BASE, OSC_CTRL, OscCtrl);
//  }

//uart_post('9'); uart_post('.'); uart_post(' ');
//NvBlPrintU32(OscFreq);
//debug_pause();

#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
	/* Move AVP clock to CLM temporarily. Reset to PLLP_OUT4 later */
	set_avp_clock_to_clkm();
#endif

  // Initialize PLL-P.
  InitPllP(OscFreq);

//  if (!IsChipInitialized)
//  {
//      // Initialize PLL-M.
//      InitPllM(OscFreq);
//  }

#if 0
  // Wait for PLL-P to lock.
  do
  {
      Reg = NV_CAR_REGR(CLK_RST_PA_BASE, PLLP_BASE);
  } while (!NV_DRF_VAL(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, Reg));
NvBlPrintU32(Reg);
uart_post('8'); uart_post('.'); uart_post(' ');
#endif

//  if (!IsChipInitialized)
//  {
//      // Wait for PLL-M to lock.
//      do
//      {
//          Reg = NV_CAR_REGR(CLK_RST_PA_BASE, PLLM_BASE);
//      } while (!NV_DRF_VAL(CLK_RST_CONTROLLER, PLLM_BASE, PLLM_LOCK, Reg));
//  }


//  if (!IsChipInitialized)
//  {
//      Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLM_OUT, PLLM_OUT1_RATIO, 0x2)
//          | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLM_OUT, PLLM_OUT1_CLKEN, ENABLE)
//          | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLM_OUT, PLLM_OUT1_RSTN, RESET_DISABLE);
//      NV_CAR_REGW(CLK_RST_PA_BASE, PLLM_OUT, Reg);
//  }

  //-------------------------------------------------------------------------
  // Switch system clock to PLLP_out 4 (108 MHz) MHz, AVP will now run at 108 MHz.
  // This is glitch free as only the source is changed, no special precaution needed.
  //-------------------------------------------------------------------------

  //*((volatile long *)0x60006028) = 0x20002222 ;// SCLK_BURST_POLICY
  Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, SYS_STATE, RUN)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, COP_AUTO_SWAKEUP_FROM_FIQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, CPU_AUTO_SWAKEUP_FROM_FIQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, COP_AUTO_SWAKEUP_FROM_IRQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, CPU_AUTO_SWAKEUP_FROM_IRQ, 0x0)
      | NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, SWAKEUP_FIQ_SOURCE, PLLP_OUT4)
      | NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, SWAKEUP_IRQ_SOURCE, PLLP_OUT4)
      | NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, SWAKEUP_RUN_SOURCE, PLLP_OUT4)
      | NV_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY, SWAKEUP_IDLE_SOURCE, PLLP_OUT4);
  NV_CAR_REGW(CLK_RST_PA_BASE, SCLK_BURST_POLICY, Reg);

  //*((volatile long *)0x6000602C) = 0x80000000 ;// SUPER_SCLK_DIVIDER
  Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_ENB, 0x1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIS_FROM_COP_FIQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIS_FROM_CPU_FIQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIS_FROM_COP_IRQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIS_FROM_CPU_IRQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIVIDEND, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIVISOR, 0x0);
  NV_CAR_REGW(CLK_RST_PA_BASE, SUPER_SCLK_DIVIDER, Reg);

  // Initialize PLL-X.
  InitPllX(OscFreq);

//uart_post('7'); uart_post('.'); uart_post(' ');
//debug_pause();

  // Initialize PLL-U.
  InitPllU(OscFreq);
//uart_post('6'); uart_post('.'); uart_post(' ');

  //*((volatile long *)0x60006030) = 0x00000010 ;// CLK_SYSTEM_RATE
  Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, HCLK_DIS, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, AHB_RATE, 0x1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, PCLK_DIS, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, APB_RATE, 0x0);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SYSTEM_RATE, Reg);

  //-------------------------------------------------------------------------
  // If the boot ROM hasn't already enabled the clocks to the memory
  // controller we have to do it here.
  //-------------------------------------------------------------------------

//  if (!IsChipInitialized)
//  {
//      //*((volatile long *)0x6000619C) = 0x03000000 ;//  CLK_SOURCE_EMC
//      Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_EMC, EMC_2X_CLK_SRC, 0x0)
//          | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_EMC, EMC_2X_CLK_DIVISOR, 0x0);
//      NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_EMC, Reg);
//  }

  //-------------------------------------------------------------------------
  // Enable clocks to required peripherals.
  //-------------------------------------------------------------------------

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_L);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_CACHE2, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_I2S0, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_VCP, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_HOST1X, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_DISP1, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_DISP2, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_3D, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_ISP, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_USBD, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_2D, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_VI, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_EPP, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_I2S2, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_PWM, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_TWC, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_SDMMC4, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_SDMMC1, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_NDFLASH, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_I2C1, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_I2S1, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_SPDIF, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_SDMMC2, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_GPIO, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_UARTB, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_UARTA, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_TMR, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_RTC, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_CPU, ENABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_L, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_H);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_BSEV, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_BSEA, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_VDE, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_MPE, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_EMC, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_UARTC, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_I2C2, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_TVDAC, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_CSI, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_HDMI, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_MIPI, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_TVO, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_DSI, ENABLE, Reg);
#if PMU_IGNORE_PWRREQ
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_DVC_I2C, ENABLE, Reg);
#else
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_DVC_I2C, ENABLE, Reg);
#endif
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_SBC3, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_XIO, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_SBC2, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_NOR, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_SBC1, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_SLC1, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_FUSE, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_PMC, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_STAT_MON, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_KBC, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_APBDMA, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_MEM, ENABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_H, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_U);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_DEV1_OUT, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_DEV2_OUT, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_SUS_OUT, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_M_DOUBLER_ENB, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, SYNC_CLK_DOUBLER_ENB, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_CRAM2, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_IRAMD, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_IRAMC, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_IRAMB, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_IRAMA, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_DTV, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_LA, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_AVPUCQ, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_CSITE, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_AFI, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_OWR, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_PCIE, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_SDMMC3, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_SBC4, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_I2C3, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_UARTE, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_UARTD, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_SPEEDO, ENABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_U, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_V);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_SE, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_TZRAM, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_HDA, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_SATA, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_SATA_OOB, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_SATA_FPCI, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_VDE_CBC, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_DAM2, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_DAM1, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_DAM0, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_APBIF, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_AUDIO, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_SBC6, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_SBC5, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_I2S4, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_I2S3, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_TSENSOR, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_MSELECT, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_3D2, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_CPULP, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_CPUG, ENABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_V, Reg);

  // Switch MSELECT clock to PLLP
  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_SOURCE_MSELECT);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_MSELECT,
          MSELECT_CLK_SRC, PLLP_OUT0, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_MSELECT, Reg);

#if PMU_IGNORE_PWRREQ
  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_SOURCE_DVC_I2C);
  Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_DVC_I2C,
          DVC_I2C_CLK_DIVISOR, 0x10, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_DVC_I2C, Reg);
#endif

  // Give clocks time to stabilize.
  AvpStallMs(1);

  // Take requried peripherals out of reset.
  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_L);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_CACHE2_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_I2S0_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_VCP_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_HOST1X_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_DISP1_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_DISP2_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_3D_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_ISP_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_USBD_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_2D_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_VI_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_EPP_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_I2S2_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_PWM_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_TWC_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_SDMMC4_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_SDMMC1_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_NDFLASH_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_I2C1_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_I2S1_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_SPDIF_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_SDMMC2_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_GPIO_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_UARTB_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_UARTA_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_TMR_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_TRIG_SYS_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_COP_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_CPU_RST, DISABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_L, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_H);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_BSEV_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_BSEA_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_VDE_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_MPE_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_EMC_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_UARTC_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_I2C2_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_TVDAC_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_CSI_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_HDMI_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_MIPI_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_TVO_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_DSI_RST, DISABLE, Reg);
#if PMU_IGNORE_PWRREQ
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_DVC_I2C_RST, DISABLE, Reg);
#else
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_DVC_I2C_RST, DISABLE, Reg);
#endif
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_SBC3_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_XIO_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_SBC2_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_NOR_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_SBC1_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_FUSE_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_STAT_MON_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_APBDMA_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_MEM_RST, DISABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_H, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_U);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_DTV_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_LA_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_AVPUCQ_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_PCIEXCLK_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_CSITE_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_AFI_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_OWR_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_PCIE_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_SDMMC3_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_SBC4_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_I2C3_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_UARTE_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_UARTD_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_SPEEDO_RST, DISABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_U, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_V);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_SE_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_TZRAM_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_HDA_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_SATA_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_SATA_OOB_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_SATA_FPCI_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_VDE_CBC_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_DAM2_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_DAM1_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_DAM0_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_APBIF_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_AUDIO_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_SBC6_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_SBC5_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_I2S4_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_I2S3_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_TSENSOR_RST, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_MSELECT_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_3D2_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_CPULP_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_CPUG_RST, ENABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_V, Reg);

}

#if 0
//------------------------------------------------------------------------------
void NvBlAvpReadChipId(NvRmChipId* id, volatile void* pMisc, volatile void* pFuse)
{
    UINT32   reg;

    // Read the chip id and parse it.
    reg = NV_MISC_REGR( pMisc, GP_HIDREV );
    id->Id = (UINT16)NV_DRF_VAL( APB_MISC_GP, HIDREV, CHIPID, reg );
    id->Major = (UINT8)NV_DRF_VAL( APB_MISC_GP, HIDREV, MAJORREV, reg );
    id->Minor = (UINT8)NV_DRF_VAL( APB_MISC_GP, HIDREV, MINORREV, reg );
    id->Family = NvRmChipFamily_HandheldSoc;

    // Read the revision id and parse it.
    reg = NV_MISC_REGR( pMisc, GP_EMU_REVID );
    id->Netlist = (UINT16)NV_DRF_VAL( APB_MISC_GP, EMU_REVID, NETLIST, reg );
    id->Patch = (UINT16)NV_DRF_VAL( APB_MISC_GP, EMU_REVID, PATCH, reg );

    // Read the SKU from the fuses.
    reg = NV_FUSE_REGR(pFuse, SKU_INFO);
    id->SKU = (UINT16)reg;

}

void AvpGetChipId(NvRmChipId* id)
{
    NvBlAvpReadChipId(id, (volatile void*)MISC_PA_BASE, (volatile void*)FUSE_PA_BASE);
}
#endif

#if 0
//BOOLEAN AvpInitAP20(BOOLEAN IsRunningFromSdram)
BOOLEAN NvBlAvpInit_T30(BOOLEAN IsRunningFromSdram)
{
#if 0
  NvRmChipId          ChipId;

  // Get the chip id.
  AvpGetChipId(&ChipId);

  // Enable the boot clocks.
  ClockInitT30(&ChipId);
#else
  // Enable the boot clocks.
  ClockInitT30();
#endif
  return TRUE;
}
#endif

VOID
//AvpStartCpuAp20 (UINT32 ResetVector)
NvBlStartCpu_T30 (UINT32 ResetVector)
{
//PostCc('A');
//debug_pause();

  // Enable VDD_CPU
  AvpEnableCpuPowerRail();

  // Halt the CPU (it should not be running).
  AvpHaltCpu(TRUE);

  // Hold the CPUs in reset.
  AvpResetCpu(TRUE);

  // Disable the CPU clock.
  AvpEnableCpuClock(FALSE);

  // Enable CoreSight.
  AvpClockEnableCorsight(TRUE);

//PostCc('C');

  // Set the entry point for CPU execution from reset, if it's a non-zero value.
  if (ResetVector)
  {
      AvpSetCpuResetVector(ResetVector);
  }

  // Enable the CPU clock.
  AvpEnableCpuClock(TRUE);

  // Power up the CPU.
  AvpPowerUpCpu();

  // Take the CPU out of reset.
  AvpResetCpu(FALSE);

  // Unhalt the CPU.
  AvpHaltCpu(FALSE);

  /* post code 'Dd' */
//  PostCc('G');
//debug_pause();

}

void start_cpu_t30(u32 reset_vector)
{
	ClockInitT30();

	NvBlStartCpu_T30(reset_vector);
}


#if 0 // jz
volatile UINT32 s_bFirstBoot = 1;

VOID bootloader()
{
    volatile UINT32 *jtagReg = (UINT32*)0x70000024;

    // enable JTAG
    *jtagReg = 192;

    if( s_bFirstBoot )
    {
        /* need to set this before cold-booting, otherwise we'll end up in
         * an infinite loop.
         */
        s_bFirstBoot = 0;
        ColdBoot();
    }

    return;
}

UINT32 GetChipRev()
{
  return DevReadField (APB_MISC, GP_HIDREV, CHIPID);
}

UINT32 GetMinorRev()
{
  return DevReadField (APB_MISC, GP_HIDREV, MINORREV);
}

UINT32 GetMajorRev()
{
  return DevReadField (APB_MISC, GP_HIDREV, MAJORREV);
}

VOID enableScu()
{
  UINT32 reg;

  reg = *((const volatile UINT32 *)(0x50040000 + 0x0));//NV_SCU_REGR(CONTROL);
  if (NV_DRF_VAL(SCU, CONTROL, SCU_ENABLE, reg) == 1)
  {
      /* SCU already enabled, return */
      return;
  }

  /* Invalidate all ways for all processors */
  *((volatile UINT32 *)((0x50040000 + 0xc))) = ((0xffff));//NV_SCU_REGW(INVALID_ALL, 0xffff);

  /* Enable SCU - bit 0 */
  reg = *((const volatile UINT32 *)(0x50040000 + 0x0));//NV_SCU_REGR(CONTROL);
  reg |= 0x1;
  *((volatile UINT32 *)((0x50040000 + 0x0))) = ((reg));//NV_SCU_REGW(CONTROL, reg);

  return;
}
#endif
