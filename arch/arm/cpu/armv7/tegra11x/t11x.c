/*
 * (C) Copyright 2008-2011 NVIDIA Corporation <www.nvidia.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include "t11x.h"
#include "t11x_uart.h"

#if 0			//FIXME: Remove this if code in start_cpu_t11x() is never used
NvBool s_IsCortexA9      = 0xa5;  // set to non-zero so this won't be in .bss
NvBool s_UsePL310L2Cache = 0xa5;  // set to non-zero so this won't be in .bss
#endif

#define NV_ASSERT(p) \
{    if ((p) == 0)	\
	uart_post('?');	\
}

#define EEPROM_I2C_SLAVE_ADDRESS  0xAC
#define EEPROM_BOARDID_LSB_ADDRESS 0x4
#define EEPROM_BOARDID_MSB_ADDRESS 0x5

static NvBlCpuClusterId AvpQueryFlowControllerClusterId(void);
static void NvBlAvpEnableCpuPowerRail(void);

static NvU32 NvBlGetOscillatorDriveStrength(void)
{
	/*
	 * TODO: get this from the ODM
	 */
	/* set to default value */
	return 0x01;
}

static NvU32 NvBlAvpQueryBootCpuFrequency(void)
{
    NvU32   frequency = 350;

    // !!!FIXME!!! ADD SKU VARIANT FREQUENCIES
    if (AvpQueryFlowControllerClusterId() == NvBlCpuClusterId_Fast)
        frequency = 700;

    return frequency;
}

VOID NvBlAvpStallUs(UINT32 MicroSec)
{
  UINT32           Reg;            // Flow controller register
  UINT32           Delay;          // Microsecond delay time
  UINT32           MaxUs;          // Maximum flow controller delay

  // Get the maxium delay per loop.
  MaxUs = NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, ZERO, 0xFFFFFFFF);

  //Bug 589922. Add 1 to requested value as the flow controller just decrements ZERO counter on
  //every usec timer tick; hence the resulting delay varies randomly between N-1 and N (N - is requested delay).

  MicroSec += 1;

  while (MicroSec) {
      Delay     = (MicroSec > MaxUs) ? MaxUs : MicroSec;
      MicroSec -= Delay;

      Reg = NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, ZERO, Delay)
          | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, uSEC, 1)
          | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, MODE, 2);

      NV_FLOW_REGW(FLOW_PA_BASE, HALT_COP_EVENTS, Reg);
  }
}

VOID NvBlAvpStallMs(UINT32 MilliSec)
{
  UINT32           Reg;            // Flow controller register
  UINT32           Delay;          // Millisecond delay time
  UINT32           MaxMs;          // Maximum flow controller delay

  // Get the maxium delay per loop.
  MaxMs = NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, ZERO, 0xFFFFFFFF);

  //Bug 589922. Add 1 to requested value as the flow controller just decrements ZERO counter on
  //every MSEC timer tick; hence the resulting delay varies randomly between N-1 and N (N - is requested delay).

  MilliSec += 1;

  while (MilliSec) {
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
	NvBlAvpEnableCpuPowerRail();
}

VOID AvpHaltCpu(BOOLEAN halt)
{
  UINT32   Reg;

  if (halt)
      Reg = NV_DRF_DEF(FLOW_CTLR, HALT_CPU_EVENTS, MODE, FLOW_MODE_STOP);
  else
      Reg = NV_DRF_DEF(FLOW_CTLR, HALT_CPU_EVENTS, MODE, FLOW_MODE_NONE);

  NV_FLOW_REGW(FLOW_PA_BASE, HALT_CPU_EVENTS, Reg);
}

static void AvpEnableCpuClock(void)
{
  UINT32   Reg;        // Scratch reg
  UINT32   Clk;        // Scratch reg

  // Wait for PLL-X to lock.
  do {
      Reg = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_BASE);
  } while (!NV_DRF_VAL(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, Reg));

      // *((volatile long *)0x60006020) = 0x20008888 ;// CCLK_BURST_POLICY
	/*
	 * WARNING: documentation error
	 *
	 * kernel does expect clock source of PLLX_OUT0_LJ being for cpu,
	 * ie value of 0x8. However, PLLX_OUT0_LJ is defined as 0x0e in file
	 * t11x/arclk_rst.h. Instead, PLLX_OUT0 is defined as 0x08.
	 */
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

  // *((volatile long *)0x60006024) = 0x80000000 ;// SUPER_CCLK_DIVIDER
  Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_ENB, 0x1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_COP_FIQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_CPU_FIQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_COP_IRQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_CPU_IRQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIVIDEND, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIVISOR, 0x0);
  NV_CAR_REGW(CLK_RST_PA_BASE, SUPER_CCLK_DIVIDER, Reg);

  // Enable the clock to all CPUs.
  Clk = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_CLR, CLR_CPU0_CLK_STP, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_CLR, CLR_CPU1_CLK_STP, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_CLR, CLR_CPU2_CLK_STP, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_CLR, CLR_CPU3_CLK_STP, 1);
      NV_CAR_REGW(CLK_RST_PA_BASE, CLK_CPU_CMPLX_CLR, Clk);

  // Always enable the main CPU complex clock.
  Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_L_SET, SET_CLK_ENB_CPU, 1);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_ENB_L_SET, Reg);
  Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_V_SET, SET_CLK_ENB_CPULP, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_V_SET, SET_CLK_ENB_CPUG, 1);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_ENB_V_SET, Reg);
}

static void AvpRemoveCpuReset(void)
{
    NvU32   Reg;    // Scratch reg

    // Take the slow non-CPU partition out of reset.
    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPULP_CMPLX_CLR, CLR_NONCPURESET, 1);
    NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPULP_CMPLX_CLR, Reg);

    // Take the fast non-CPU partition out of reset.
    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_NONCPURESET, 1);
    NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPUG_CMPLX_CLR, Reg);

    // Clear software controlled reset of slow cluster
    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPULP_CMPLX_CLR, CLR_CPURESET0, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPULP_CMPLX_CLR, CLR_DBGRESET0, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPULP_CMPLX_CLR, CLR_CORERESET0,1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPULP_CMPLX_CLR, CLR_CXRESET0,  1);
    NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPULP_CMPLX_CLR, Reg);

    // Clear software controlled reset of fast cluster
    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CPURESET0, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_DBGRESET0, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CORERESET0,1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CXRESET0,  1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CPURESET1, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_DBGRESET1, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CORERESET1,1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CXRESET1,  1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CPURESET2, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_DBGRESET2, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CORERESET2,1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CXRESET2,  1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CPURESET3, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_DBGRESET3, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CORERESET3,1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CXRESET3,  1);

    NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPUG_CMPLX_CLR, Reg);
}

static void AvpClockEnableCorsight(void)
{
  UINT32   Reg;        // Scratch register

      // Put CoreSight on PLLP_OUT0 (216 MHz) and divide it down by 1.5
      // giving an effective frequency of 144MHz.

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

      NvBlAvpStallUs(3);

      Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_CSITE,
          CSITE_CLK_SRC, PLLP_OUT0, Reg);
      NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_CSITE, Reg);

      // Enable CoreSight clock and take the module out of reset in case
      // the clock is not enabled or the module is being held in reset.
      Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_ENB_U_SET, SET_CLK_ENB_CSITE, 1);
      NV_CAR_REGW(CLK_RST_PA_BASE, CLK_ENB_U_SET, Reg);

      Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_U_CLR, CLR_CSITE_RST, 1);
      NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEV_U_CLR, Reg);
}

VOID AvpSetCpuResetVector(UINT32 reset)
{
	NvU32  imme, inst;

	inst = imme = reset & 0xFFFF;
	inst &= 0xFFF;
	inst |= ((imme >> 12) << 16);
	inst |= 0xE3000000;
	NV_WRITE32(0x4003FFF0, inst); /* MOV R0, #LSB16(ResetVector) */

	imme = (reset >> 16) & 0xFFFF;
	inst = imme & 0xFFF;
	inst |= ((imme >> 12) << 16);
	inst |= 0xE3400000;
	NV_WRITE32(0x4003FFF4, inst); /* MOVT R0, #MSB16(ResetVector) */

	NV_WRITE32(0x4003FFF8, 0xE12FFF10); /* BX R0 */

	inst = (NvU32)-20;
	inst >>= 2;
	inst &= 0x00FFFFFF;
	inst |= 0xEA000000;
	NV_WRITE32(0x4003FFFC, inst); /* B -12 */

	/* keep compatible to A01 */
	NV_EVP_REGW(EVP_PA_BASE, CPU_RESET_VECTOR, reset);
}

static NvBlCpuClusterId AvpQueryFlowControllerClusterId(void)
{
    NvBlCpuClusterId    ClusterId = NvBlCpuClusterId_G; // Active CPU id
    NvU32               Reg;                            // Scratch register

    Reg = NV_FLOW_REGR(FLOW_PA_BASE, CLUSTER_CONTROL);
    Reg = NV_DRF_VAL(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, Reg);

    if (Reg == NV_DRF_DEF(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, LP))
        ClusterId = NvBlCpuClusterId_LP;

    return ClusterId;
}

static NvBool AvpIsPartitionPowered(NvU32 Mask)
{
    // Get power gate status.
    NvU32   Reg = NV_PMC_REGR(PMC_PA_BASE, PWRGATE_STATUS);

    if ((Reg & Mask) == Mask)
        return NV_TRUE;

    return NV_FALSE;
}

static NvBool AvpIsClampRemoved(NvU32 Mask)
{
    // Get clamp status.
    NvU32   Reg = NV_PMC_REGR(PMC_PA_BASE, CLAMP_STATUS);

    if ((Reg & Mask) == Mask)
    {
        return NV_FALSE;
    }

    return NV_TRUE;
}

static void AvpPowerPartition(NvU32 status, NvU32 toggle)
{
    // Is the partition already on?
    if (!AvpIsPartitionPowered(status)) {
        // No, toggle the partition power state (OFF -> ON).
        NV_PMC_REGW(PMC_PA_BASE, PWRGATE_TOGGLE, toggle);

        // Wait for the power to come up.
        while (!AvpIsPartitionPowered(status))
		;            // Do nothing

        // Wait for the clamp status to be cleared.
        while (!AvpIsClampRemoved(status))
		;            // Do nothing
    }
}

#define POWER_PARTITION(x)  \
    AvpPowerPartition(PMC_PWRGATE_STATUS(x), PMC_PWRGATE_TOGGLE(x))

static void AvpPowerUpCpu(void)
{
    // Are we booting to the fast cluster?
    if (AvpQueryFlowControllerClusterId() == NvBlCpuClusterId_G) {
        // TODO: Set CPU Power Good Timer

        // Power up the fast cluster rail partition.
        POWER_PARTITION(CRAIL);

        // Power up the fast cluster non-CPU partition.
        POWER_PARTITION(C0NC);

        // Power up the fast cluster CPU0 partition.
        POWER_PARTITION(CE0);
    } else {
        // Power up the slow cluster non-CPU partition.
        POWER_PARTITION(C1NC);

        // Power up the slow cluster CPU partition.
        POWER_PARTITION(CELP);
    }
}

VOID AvpHaltAvp(void)
{
	UINT32   Reg;    // Scratch reg

	for (;;) {
		Reg = NV_DRF_DEF(FLOW_CTLR, HALT_COP_EVENTS, MODE, FLOW_MODE_STOP)
			| NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, JTAG, 1)
			| NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, LIC_IRQ, 1)
			| NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, LIC_FIQ, 1);
		NV_FLOW_REGW(FLOW_PA_BASE, HALT_COP_EVENTS, Reg);
	}
}

VOID InitPllP(NvBootClocksOscFreq OscFreq)
{
	UINT32   Base, Misc, Reg;
	Base = NV_CAR_REGR(CLK_RST_PA_BASE, PLLP_BASE);
#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
	UINT32 OutA, OutB;
#endif
	Misc = Base = 0;

#if !defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
	if ((Base & NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, ENABLE))) {
		Misc = NV_CAR_REGR(CLK_RST_PA_BASE, PLLP_MISC);
		Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_LOCK_ENABLE, ENABLE, Misc);
		NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_MISC, Misc);
		return;
	}
#endif

	/*
	 * DIVP/DIVM/DIVN values taken from arclk_rst.h table for fixed
	 *  216MHz or 408MHz operation.
	 */
	switch (OscFreq) {
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
              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 8);
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
              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 4);
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
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0x1B0)
#endif
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0x0C);
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
              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 8);
              break;

          case NvBootClocksOscFreq_16_8:
              Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                   | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
		| NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x0)
		| NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0x154)
#else
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0xB4)
#endif
                   | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0x14);
              Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 4);
              break;

          default:
              break;
	}

	NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_MISC, Misc);
	NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Base);

	/* Assert OUTx_RSTN for pllp_out1,2,3,4 before PLLP enable */
	Reg = NV_CAR_REGR(CLK_RST_PA_BASE,PLLP_OUTA);
	Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_RSTN,
		RESET_ENABLE,Reg);
	Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_RSTN,
		RESET_ENABLE,Reg);
	NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_OUTA, Reg);

	Reg = NV_CAR_REGR(CLK_RST_PA_BASE,PLLP_OUTB);
	Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_RSTN,
		RESET_ENABLE,Reg);
	Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_RSTN,
		RESET_ENABLE,Reg);
	NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_OUTB, Reg);

	// disable override
	Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, DISABLE, Base);
	NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Base);

	// reenable PLLP
	Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, ENABLE, Base);
	NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Base);

	// Give clocks time to stabilize.
	NvBlAvpStallUs(20);

	// disable bypass
	Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, DISABLE, Base);
	NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Base);

#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
	/* Set pllp_out1,2,3,4 to frequencies 9.6MHz, 48MHz, 102MHz, 102MHz */
	OutA = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_RATIO, 83) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_OVRRIDE, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_CLKEN, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_RSTN, RESET_DISABLE) |

	NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_RATIO, 15) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_OVRRIDE, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_CLKEN, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_RSTN, RESET_DISABLE);

	OutB = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_RATIO, 6) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_OVRRIDE, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_CLKEN, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_RSTN, RESET_DISABLE) |

	NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_RATIO, 6) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_OVRRIDE, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_CLKEN, ENABLE) |
	NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_RSTN, RESET_DISABLE);

	NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_OUTA, OutA);
	NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_OUTB, OutB);
#endif

	// lock enable
	Misc = NV_CAR_REGR(CLK_RST_PA_BASE, PLLP_MISC);
	Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_LOCK_ENABLE, ENABLE, Misc);
	NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_MISC, Misc);

	// Wait for PLL-P to lock.
	do {
		Base = NV_CAR_REGR(CLK_RST_PA_BASE, PLLP_BASE);
	} while (!NV_DRF_VAL(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, Base));
}

//----------------------------------------------------------------------------------------------
static void InitPllX(NvBootClocksOscFreq OscFreq)
{
    NvU32               Base;       // PLL Base
    NvU32               Divm;       // Reference
    NvU32               Divn;       // Multiplier
    NvU32               Divp;       // Divider == Divp ^ 2
    NvU32               p, Misc;
    NvU32               Val;
    NvU32               OscFreqKhz, RateInKhz, VcoMin = 700000;

    // Is PLL-X already running?
    Base = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_BASE);
    Base = NV_DRF_VAL(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, Base);
    if (Base == NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, ENABLE))
        return;

    switch(OscFreq) {
        case NvBootClocksOscFreq_13:
            OscFreqKhz = 13000;
            break;
        case NvBootClocksOscFreq_38_4:
            OscFreqKhz = 38400;
            break;
        case NvBootClocksOscFreq_19_2:
            OscFreqKhz = 19200;
            break;
        case NvBootClocksOscFreq_12:
            OscFreqKhz = 12000;
            break;
        case NvBootClocksOscFreq_26:
            OscFreqKhz = 26000;
            break;
        case NvBootClocksOscFreq_16_8:
            OscFreqKhz = 16800;
            break;
        default:
            NV_ASSERT(!OscFreq); // Invalid frequency
	    OscFreqKhz = 12000;
    }

    RateInKhz = NvBlAvpQueryBootCpuFrequency() * 1000;

    /* Clip vco_min to exact multiple of OscFreq to avoid
       crossover by rounding */
    VcoMin = ((VcoMin + OscFreqKhz- 1) / OscFreqKhz)*OscFreqKhz;

    p = (VcoMin + RateInKhz - 1)/RateInKhz; // Rounding p
    Divp = p - 1;
    Divm = (OscFreqKhz> 19200)? 2 : 1; // cf_max = 19200 Khz
    Divn = (RateInKhz * p * Divm)/OscFreqKhz;

    Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, ENABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, DISABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_REF_DIS, REF_ENABLE)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVP, Divp)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, Divn)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVM, Divm);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Base);

        Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, DISABLE, Base);
        NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Base);

        Misc = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_MISC);
        Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_MISC, PLLX_LOCK_ENABLE, ENABLE, Misc);
        Misc &= 0xFFFC0000;  	/* clear bit 17:0 */
        NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_MISC, Misc);

    // Disable IDDQ
    Val = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_MISC_3);
    Val = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, PLLX_MISC_3, PLLX_IDDQ, 0x0, Val);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_MISC_3, Val);

    // Wait 2 us
    NvBlAvpStallUs(2);

    Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, ENABLE, Base);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Base);
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

  if (isEnabled && !isBypassed) {
      // Make sure lock enable is set.
      Misc = NV_CAR_REGR(CLK_RST_PA_BASE, PLLU_MISC);
      Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLU_MISC, PLLU_LOCK_ENABLE, ENABLE, Misc);
      NV_CAR_REGW(CLK_RST_PA_BASE, PLLU_MISC, Misc);
      return;
  }

      // DIVP/DIVM/DIVN values taken from arclk_rst.h table
      switch (OscFreq) {
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

static NvBlCpuClusterId NvBlAvpQueryBootCpuClusterId(void)
{
    NvU32   Reg;            // Scratch register
    NvU32   Fuse;           // Fuse register

    // Read the fuse register containing the CPU capabilities.
    Fuse = NV_FUSE_REGR(FUSE_PA_BASE, SKU_DIRECT_CONFIG);

    // Read the bond-out register containing the CPU capabilities.
    Reg = NV_CAR_REGR(CLK_RST_PA_BASE, BOND_OUT_V);

    // Are there any CPUs present in the fast CPU complex?
    if ((!NV_DRF_VAL(CLK_RST_CONTROLLER, BOND_OUT_V, BOND_OUT_CPUG, Reg))
     && (!NV_DRF_VAL(FUSE, SKU_DIRECT_CONFIG, DISABLE_ALL, Fuse))) {
        return NvBlCpuClusterId_G;	//_Fast
    }

    // Is there a CPU present in the slow CPU complex?
    if (!NV_DRF_VAL(CLK_RST_CONTROLLER, BOND_OUT_V, BOND_OUT_CPULP, Reg))
        return NvBlCpuClusterId_LP;	//_Slow

    return NvBlCpuClusterId_Unknown;
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
	NvBlAvpStallUs(3);
}
#endif

VOID ClockInitT11x(void)
{
  NvBootClocksOscFreq     OscFreq;        // Oscillator frequency
  UINT32                  Reg;            // Temporary register
  NvU32                   OscStrength;    // Oscillator Drive Strength
  NvBlCpuClusterId        CpuClusterId;   // Boot CPU cluster id

  // Get the oscillator frequency.
    Reg = NV_CAR_REGR(CLK_RST_PA_BASE, OSC_CTRL);
    OscFreq = (NvBootClocksOscFreq)NV_DRF_VAL(CLK_RST_CONTROLLER, OSC_CTRL, OSC_FREQ, Reg);

  // Find out which CPU cluster we're booting to.
  CpuClusterId = NvBlAvpQueryBootCpuClusterId();

  Reg = NV_FLOW_REGR(FLOW_PA_BASE, CLUSTER_CONTROL);
  // Are we booting to LP CPU complex?
  if (CpuClusterId == NvBlCpuClusterId_LP) {
      // Set active CPU cluster to LP.
      Reg = NV_FLD_SET_DRF_DEF(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, LP, Reg);
  } else {
      // Set active CPU cluster to G.
      Reg = NV_FLD_SET_DRF_DEF(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, G, Reg);
  }

  NV_FLOW_REGW(FLOW_PA_BASE, CLUSTER_CONTROL, Reg);

    // Enable the PPSB_STOPCLK feature to allow SCLK to be run at higher
    // frequencies. See bug 811773.
    Reg = NV_CAR_REGR(CLK_RST_PA_BASE, MISC_CLK_ENB);
    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, MISC_CLK_ENB,
            EN_PPSB_STOPCLK, ENABLE, Reg);
    NV_CAR_REGW(CLK_RST_PA_BASE, MISC_CLK_ENB, Reg);

    Reg = NV_AHB_ARBC_REGR(AHB_PA_BASE, XBAR_CTRL);
    Reg = NV_FLD_SET_DRF_DEF(AHB_ARBITRATION, XBAR_CTRL,
            PPSB_STOPCLK_ENABLE, ENABLE, Reg);
    NV_AHB_ARBC_REGW(AHB_PA_BASE, XBAR_CTRL, Reg);

    // Get Oscillator Drive Strength Setting
    OscStrength = NvBlGetOscillatorDriveStrength();

    // Change the oscillator drive strength
    Reg = NV_CAR_REGR(CLK_RST_PA_BASE, OSC_CTRL);
    Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOFS, OscStrength, Reg);
    NV_CAR_REGW(CLK_RST_PA_BASE, OSC_CTRL, Reg);
    // Update same value in PMC_OSC_EDPD_OVER XOFS field for warmboot
    Reg = NV_PMC_REGR(PMC_PA_BASE, OSC_EDPD_OVER);
    Reg = NV_FLD_SET_DRF_NUM(APBDEV_PMC, OSC_EDPD_OVER, XOFS, OscStrength, Reg);
    NV_PMC_REGW(PMC_PA_BASE, OSC_EDPD_OVER, Reg);

#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
	/* Move AVP clock to CLKM temporarily. Reset to PLLP_OUT4 later */
	set_avp_clock_to_clkm();
#endif

  // Initialize PLL-P.
  InitPllP(OscFreq);

  //-------------------------------------------------------------------------
  // Switch system clock to PLLP_out 4 (108 MHz) MHz, AVP will now run at 108 MHz.
  // This is glitch free as only the source is changed, no special precaution needed.
  //-------------------------------------------------------------------------

  // *((volatile long *)0x60006028) = 0x20002222 ;// SCLK_BURST_POLICY
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

  // *((volatile long *)0x6000602C) = 0x80000000 ;// SUPER_SCLK_DIVIDER
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

  // *((volatile long *)0x60006030) = 0x00000010 ;// CLK_SYSTEM_RATE
  Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, HCLK_DIS, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, AHB_RATE, 0x1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, PCLK_DIS, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, APB_RATE, 0x0);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SYSTEM_RATE, Reg);

  //-------------------------------------------------------------------------
  // If the boot ROM hasn't already enabled the clocks to the memory
  // controller we have to do it here.
  //-------------------------------------------------------------------------

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
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_UARTA, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_TMR, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_RTC, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, CLK_ENB_CPU, DISABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_L, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_H);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_BSEV, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_BSEA, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_VDE, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_MPE, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_USB3, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_USB2, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_EMC, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_MIPI_CAL, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_UARTC, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_I2C2, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_CSI, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_HDMI, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_MIPI, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_DSI, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_I2C5, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_SBC3, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_XIO, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_SBC2, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_NOR, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_SNOR, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_SBC1, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_SLC1, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_KFUSE, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_FUSE, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_PMC, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_STAT_MON, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_KBC, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_APBDMA, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_AHBDMA, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_MEM, ENABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_H, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_U);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_XUSB_DEV, ENABLE, Reg)
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_DEV1_OUT, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_DEV2_OUT, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_SUS_OUT, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_MSENC, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_M_DOUBLER_ENB, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_CRAM2, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_IRAMD, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_IRAMC, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_IRAMB, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_IRAMA, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_TSEC, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_DSIB, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_I2C_SLOW, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_NAND_SPEED, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_DTV, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_SOC_THERM, ENABLE, Reg);
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
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_U, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_V);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_SE, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_TZRAM, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_HDA, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_SATA, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_SATA_OOB, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_EXTPERIPH3, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_EXTPERIPH2, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_ACTMON, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_SPDIF_DOUBLER, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_I2S4_DOUBLER, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_I2S3_DOUBLER, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_I2S2_DOUBLER, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_I2S1_DOUBLER, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_HDA2CODEC_2X, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_DAM2, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_DAM1, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_DAM0, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_APBIF, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_AUDIO, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_SBC6, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_SBC5, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_I2C4, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_I2S4, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_I2S3, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_TSENSOR, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_MSELECT, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_3D2, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_CPULP, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_CPUG, ENABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_V, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_W);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_EMC1, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_MC1, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_DP2, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_DDS, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_ENTROPY, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_DISB_LP, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_DISA_LP, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_CILE, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_CILCD, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_CILAB, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_XUSB, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_MIPI_IOBIST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_SATA_IOBIST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_HDMI_IOBIST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_EMC_IOBIST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_PCIE2_IOBIST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_CEC, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_PCIERX5, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_PCIERX4, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_PCIERX3, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_PCIERX2, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_PCIERX1, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_PCIERX0, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_HDA2HDMICODEC, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_W, CLK_ENB_DVFS, ENABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_W, Reg);

  // Switch MSELECT clock to PLLP
  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_SOURCE_MSELECT);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_MSELECT,
          MSELECT_CLK_SRC, PLLP_OUT0, Reg);

    // Clock divider request for 102MHz would setup MSELECT clock as 108MHz for PLLP base 216MHz
    // and 102MHz for PLLP base 408MHz.
    Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_MSELECT,
            MSELECT_CLK_DIVISOR, CLK_DIVIDER(NVBL_PLLP_KHZ, 102000), Reg);
    NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_MSELECT, Reg);

  /* Give clocks time to stabilize */
  NvBlAvpStallMs(1);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_SOURCE_I2C5);
  Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_I2C5,
          I2C5_CLK_DIVISOR, 0x10, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_I2C5, Reg);

  // Give clocks time to stabilize.
  NvBlAvpStallMs(1);

  // Take required peripherals out of reset.
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
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_UARTA_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_TMR_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_COP_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L, SWR_CPU_RST, DISABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_L, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_H);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_BSEV_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_BSEA_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_VDE_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_MPE_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_USB3_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_USB2_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_EMC_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_MIPI_CAL_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_UARTC_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_I2C2_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_CSI_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_HDMI_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_HSI_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_DSI_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_I2C5_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_SBC3_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_XIO_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_SBC2_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_NOR_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_SNOR_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_SBC1_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_KFUSE_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_FUSE_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_STAT_MON_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_APBDMA_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_AHBDMA_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H, SWR_MEM_RST, DISABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_H, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_U);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_EMUCIF_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_MSENC_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_TSEC_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_DSIB_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_I2C_SLOW_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_NAND_SPEED_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_DTV_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_SOC_THERM_RST, DISABLE, Reg);
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
  NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_U, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_V);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_SE_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_TZRAM_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_HDA_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_SATA_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_SATA_OOB_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_EXTPERIPH3_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_EXTPERIPH2_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_EXTPERIPH1_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_ACTMON_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_ATOMICS_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_HDA2CODEC_2X_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_DAM2_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_DAM1_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_DAM0_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_APBIF_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_AUDIO_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_SBC6_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_SBC5_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_I2C4_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_I2S4_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_I2S3_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_TSENSOR_RST, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_MSELECT_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_3D2_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_CPULP_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_CPUG_RST, ENABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_V, Reg);

  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_W);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_W, SWR_EMC1_RST, DISABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_W, SWR_MC1_RST, DISABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_W, SWR_DP2_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_W, SWR_DDS_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_W, SWR_ENTROPY_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_W, SWR_XUSB_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_W, SWR_CEC_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_W, SWR_SATACOLD_RST, ENABLE, Reg);
  // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_W, SWR_HDA2HDMICODEC_RST, ENABLE, Reg);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_W, SWR_DVFS_RST, DISABLE, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_W, Reg);

  // t11x_WriteDebugString("\nAVP will halt, and CPU should start!\n");
}

VOID NvBlStartCpu_T11x(UINT32 ResetVector)
{
  // Enable VDD_CPU
  AvpEnableCpuPowerRail();

  // Enable the CPU clock.
  AvpEnableCpuClock();

  // Enable CoreSight.
  AvpClockEnableCorsight();

  // Remove all software overrides
  AvpRemoveCpuReset();

  // Set the entry point for CPU execution from reset, if it's a non-zero value.
  if (ResetVector)
      AvpSetCpuResetVector(ResetVector);

  // Power up the CPU.
  AvpPowerUpCpu();

  AvpHaltAvp();
}

static void NvBlMemoryControllerInit(NvBool IsChipInitialized)
{
    NvU32   Reg;

    //-------------------------------------------------------------------------
    // Memory Aperture Configuration
    //-------------------------------------------------------------------------

    // Get the default aperture configuration.
    Reg = NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, IROM_LOVEC, MMIO)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, PCIE_A1, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, PCIE_A2, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, PCIE_A3, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, IRAM_RSVD, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, NOR_A1, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, NOR_A2, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, NOR_A3, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, VERIF_RSVD, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, GFX_HOST_RSVD, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, GART_GPU, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, PPSB_RSVD, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, EXTIO_RSVD, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, APB_RSVD, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, AHB_A1, MMIO)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, AHB_A1_RSVD, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, AHB_A2_RSVD, DRAM)
        | NV_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, IROM_HIVEC, MMIO);

    NV_PMC_REGW(PMC_PA_BASE, GLB_AMAP_CFG, Reg);

    // Disable any further writes to the address map configuration register.
    Reg = NV_PMC_REGR(PMC_PA_BASE, SEC_DISABLE);
    Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, SEC_DISABLE, AMAP_WRITE, ON, Reg);
    NV_PMC_REGW(PMC_PA_BASE, SEC_DISABLE, Reg);

    //-------------------------------------------------------------------------
    // Memory Controller Tuning
    //-------------------------------------------------------------------------

    // Set up the AHB Mem Gizmo
    Reg = NV_AHB_GIZMO_REGR(AHB_PA_BASE, AHB_MEM);
    Reg = NV_FLD_SET_DRF_NUM(AHB_GIZMO, AHB_MEM, ENABLE_SPLIT, 1, Reg);
    Reg = NV_FLD_SET_DRF_NUM(AHB_GIZMO, AHB_MEM, DONT_SPLIT_AHB_WR, 0, Reg);
    /// Added for USB Controller
    Reg = NV_FLD_SET_DRF_NUM(AHB_GIZMO, AHB_MEM, ENB_FAST_REARBITRATE, 1, Reg);
    NV_AHB_GIZMO_REGW(AHB_PA_BASE, AHB_MEM, Reg);

    // Enable GIZMO settings for USB controller.
    Reg = NV_AHB_GIZMO_REGR(AHB_PA_BASE, USB);
    Reg = NV_FLD_SET_DRF_NUM(AHB_GIZMO, USB, IMMEDIATE, 1, Reg);
    NV_AHB_GIZMO_REGW(AHB_PA_BASE, USB, Reg);

    // Make sure the debug registers are clear.
    Reg = 0;
    NV_MISC_REGW(MISC_PA_BASE, GP_ASDBGREG, Reg);
    NV_MISC_REGW(MISC_PA_BASE, GP_ASDBGREG2, Reg);

    //Set Weak Bias
    NV_PMC_REGW(PMC_PA_BASE, WEAK_BIAS, 0x3ff);

    // Enable errata for 1157520
    Reg = NV_MC_REGR(MC_PA_BASE, EMEM_ARB_OVERRIDE);
    Reg &= ~0x2;
    NV_MC_REGW(MC_PA_BASE, EMEM_ARB_OVERRIDE, Reg);
}

void start_cpu_t11x(u32 reset_vector)
{
	UINT32 Reg;
#if 0	//FIXME: need it for A15 remove tmp dependency ??

	UINT32 tmp;
	s_IsCortexA9 = NV_TRUE;
	s_UsePL310L2Cache = NV_FALSE;

	// Is this a Cortex-A9? */
	MRC(p15, 0, tmp, c0, c0, 0);
	if ((tmp & 0x0000FFF0) != 0x0000C090)
	{
		// No
		s_IsCortexA9 = NV_FALSE;
		s_UsePL310L2Cache = NV_FALSE;
	}

	if (s_IsCortexA9 == NV_FALSE)
	{
		MRC(p15, 1, tmp, c9, c0, 2);
		tmp &= ~7;
		tmp |= 2;
		MCR(p15, 1, tmp, c9, c0, 2);
	}
#endif	//0
	ClockInitT11x();

	/* Initialize memory controller */
	NvBlMemoryControllerInit(1);

	/* Set power gating timer multiplier -- see bug 966323 */
	Reg = NV_PMC_REGR(PMC_PA_BASE, PWRGATE_TIMER_MULT);
	Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, PWRGATE_TIMER_MULT, MULT, EIGHT, Reg);
	NV_PMC_REGW(PMC_PA_BASE, PWRGATE_TIMER_MULT, Reg);

	NvBlStartCpu_T11x(reset_vector);
}

static void NvBlGen1I2cStatusPoll(void)
{
    NvU32 status = 0;
    NvU32 timeout = 0;

#define I2C_POLL_TIMEOUT_MSEC   1000
#define I2C_POLL_STEP_MSEC  1

    while (timeout < I2C_POLL_TIMEOUT_MSEC) {
        status = NV_GEN1I2C_REGR(GEN1I2C_PA_BASE, I2C_STATUS);
        if (status)
            NvBlAvpStallMs(I2C_POLL_STEP_MSEC);
        else
            return;

        timeout += I2C_POLL_STEP_MSEC;
    }
    NV_ASSERT(0);
#undef  I2C_POLL_STEP_MSEC
#undef  I2C_POLL_TIMEOUT_MSEC
}

static NvU32 NvBlAvpI2cReadEeprom(NvU32 Addr, NvU32 Reg)
{
    NvU32 Ret = 0;

    //Read the Board Id
    NV_GEN1I2C_REGW(GEN1I2C_PA_BASE, I2C_CMD_ADDR0, (Addr & 0xFE));
    NV_GEN1I2C_REGW(GEN1I2C_PA_BASE, I2C_CMD_ADDR1, (Addr | 0x1));
    NV_GEN1I2C_REGW(GEN1I2C_PA_BASE, I2C_CMD_DATA1, Reg);
    NV_GEN1I2C_REGW(GEN1I2C_PA_BASE, I2C_CNFG, 0x290);
    NvBlGen1I2cStatusPoll();
    Ret = NV_GEN1I2C_REGR(GEN1I2C_PA_BASE, I2C_CMD_DATA2);

    return Ret;
}

static NvU32 NvBlAvpI2cReadBoardId(NvU32 Addr)
{
    NvU32 Lsb = 0;
    NvU32 Msb = 0;
    NvU32 Ret = 0;
    static NvBool gen1initialised = NV_FALSE;
    NvU32 Reg = 0;

    if (!gen1initialised) {
        //Is the pinmux initialisation required
        //Enable SCL and SDA
        NV_WRITE32(PINMUX_BASE + PINMUX_AUX_GEN1_I2C_SDA_0, 0x60);
        NV_WRITE32(PINMUX_BASE + PINMUX_AUX_GEN1_I2C_SCL_0, 0x60);

        //Enable clock
        Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_L);
        Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_L,
                     CLK_ENB_I2C1, 1, Reg);
        NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_L, Reg);

        //Reset the controller
        Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_L);
        Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_L,
                     SWR_I2C1_RST, 1, Reg);
        NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_L, Reg);
        NvBlAvpStallMs(2);
        Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_L);
        Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_L,
                     SWR_I2C1_RST, 0, Reg);
        NV_CAR_REGW(CLK_RST_PA_BASE,RST_DEVICES_L, Reg);
    }

    Lsb = NvBlAvpI2cReadEeprom(Addr, EEPROM_BOARDID_LSB_ADDRESS);
    Msb = NvBlAvpI2cReadEeprom(Addr, EEPROM_BOARDID_MSB_ADDRESS);

    Ret = Lsb | (Msb << 8);
    return Ret;
}

#define NV_PWRI2C_REGR(pPwrI2c, reg)        NV_READ32( (((NvUPtr)(pPwrI2c)) + I2C_##reg##_0))
#define NV_PWRI2C_REGW(pPwrI2c, reg, val)   NV_WRITE32((((NvUPtr)(pPwrI2c)) + I2C_##reg##_0), (val))

static void NvBlAvpI2cStatusPoll(void)
{
    NvU32 status = 0;
    NvU32 timeout = 0;

#define I2C_POLL_TIMEOUT_MSEC   1000
#define I2C_POLL_STEP_MSEC  1

    while (timeout < I2C_POLL_TIMEOUT_MSEC) {
        status = NV_PWRI2C_REGR(PWRI2C_PA_BASE, I2C_STATUS);
        if (status)
            NvBlAvpStallMs(I2C_POLL_STEP_MSEC);
        else
            return;

        timeout += I2C_POLL_STEP_MSEC;
    }

#undef  I2C_POLL_STEP_MSEC
#undef  I2C_POLL_TIMEOUT_MSEC
}

static void NvBlAvpI2cWrite(NvU32 Addr, NvU32 Data)
{
    NV_PWRI2C_REGW(PWRI2C_PA_BASE, I2C_CMD_ADDR0, Addr);
    NV_PWRI2C_REGW(PWRI2C_PA_BASE, I2C_CMD_DATA1, Data);
    NV_PWRI2C_REGW(PWRI2C_PA_BASE, I2C_CNFG, 0xA02);
    NvBlAvpI2cStatusPoll();
}

static void NvBlAvpEnableCpuPowerRail(void)
{
    NvU32   Reg;        // Scratch reg
    NvU32 BoardId;

#define DALMORE_E1613_BOARDID 0x64D
#define DALMORE_E1611_BOARDID 0x64B
#define PLUTO_E1580_BOARDID   0x62C

	BoardId = NvBlAvpI2cReadBoardId(EEPROM_I2C_SLAVE_ADDRESS);

    /*
     * Program PMIC TPS51632
     *
     * slave address 7b 1000011 (3 LSB selected by SLEWA which is 0.8v on
     * Dalmore E1613.
     *
     * The cold boot voltage is selected by BRAMP  which is 0v on Dalmore,
     * so we need to program VSR to 1.10v
     */

#define DALMORE_TPS51632_I2C_ADDR   0x86
#define PLUTO_TPS65913_I2C_ADDR     0xB0
#define PLUTO_CPUPWRREQ_EN          1

	/* set pinmux for PWR_I2C SCL and SDA */
	NV_WRITE32(PINMUX_BASE + PINMUX_AUX_PWR_I2C_SCL_0, 0x60);
	NV_WRITE32(PINMUX_BASE + PINMUX_AUX_PWR_I2C_SDA_0, 0x60);

	if ((BoardId == DALMORE_E1611_BOARDID) ||
		(BoardId == DALMORE_E1613_BOARDID)) {
		NvBlAvpI2cWrite(DALMORE_TPS51632_I2C_ADDR, 0x5500);
	}
        else if (BoardId == PLUTO_E1580_BOARDID)
        {
#ifdef PLUTO_CPUPWRREQ_EN
            NvBlAvpI2cWrite(PLUTO_TPS65913_I2C_ADDR, 0x0120);
            NvBlAvpI2cWrite(PLUTO_TPS65913_I2C_ADDR, 0x4023);
            NvBlAvpI2cWrite(PLUTO_TPS65913_I2C_ADDR, 0x0124);
#else
            NvBlAvpI2cWrite(PLUTO_TPS65913_I2C_ADDR, 0x0520);
            NvBlAvpI2cWrite(PLUTO_TPS65913_I2C_ADDR, 0x4023);
            NvBlAvpI2cWrite(PLUTO_TPS65913_I2C_ADDR, 0x0524);
#endif
        }

	/* FIXME:
	 * Hack for enabling necessary rails for display, sdmmc3, usb, etc.
	 * Need to be removed once PWR I2C driver is fixed for CPU side.
	 */
	if (BoardId == DALMORE_E1611_BOARDID) {
		/*
		 * Voltage for SDMMC3: VDDIO_SDMMC3_AP
		 *   LDO9_VOLTAGE = 3.3v (TPS65913)
		 *   LDO9_CTRL = Active
		 *
		 * Voltage for SD Card Socket: VDD_3V3_SD
		 *   VOUT6 (FET6) = 3.3v (TPS65090)
		 */
		NvBlAvpI2cWrite(0xB0, 0x3161);
		NvBlAvpI2cWrite(0xB0, 0x0160);
		NvBlAvpI2cWrite(0x90, 0x0314);

		/*
		 * Enable USB voltage: AVDD_USB_HDMI for AVDD_USB_AP
		 *			and AVDD_HDMI_AP
		 *   LDOUSB_VOLTAGE = 3.3v
		 *   LDOUSB_CTRL = Active
		 */
		NvBlAvpI2cWrite(0xB0, 0x3165);
		NvBlAvpI2cWrite(0xB0, 0x0164);

		/*
		 * Enable HVDD_USB3 voltage: HVDD_USB3_AP
		 *   LDOLN_VOLTAGE = 3.3v
		 *   LDOLN_CTRL = Active
		 */
		NvBlAvpI2cWrite(0xB0, 0x3163);
		NvBlAvpI2cWrite(0xB0, 0x0162);

		/*
		 * Enable additional VDD_1V1_CORE
		 *
		 *   SMPS7_CTRL: enable active: auto
		 *
		 *   VDD_CORE is provided by SMPS4_SW, 5 and 7 where
		 *   4 and 5 are enabled after power on.
		 */
		NvBlAvpI2cWrite(0xB0, 0x0530);	/* SMPS7_SW: active: auto */

		/*
		 * Set and enable AVDD_2V8_CAM1
		 *   LDO1_VOLTAGE = 2.8v
		 *   LDO1_CTRL = Active
		 */
		NvBlAvpI2cWrite(0xB0, 0x2751);
		NvBlAvpI2cWrite(0xB0, 0x0150);

		/*
		 * Set and enable AVDD_2V8_CAM2
		 *   LDO2_VOLTAGE = 2.8v
		 *   LDO2_CTRL = Active
		 */
		NvBlAvpI2cWrite(0xB0, 0x2753);
		NvBlAvpI2cWrite(0xB0, 0x0152);

		/*
		 * Set and enable AVDD_1V2 for VDDIO_HSIC_AP and AVDD_DSI_CSI_AP
		 *   LDO3_VOLTAGE = 1.2v
		 *   LDO3_CTRL = Active
		 */
		NvBlAvpI2cWrite(0xB0, 0x0755);
		NvBlAvpI2cWrite(0xB0, 0x0154);

		/*
		 * Set and enable VPP_FUSE_APP
		 *   LDO4_VOLTAGE = 1.8v
		 *   LDO4_CTRL = Active
		 */
		NvBlAvpI2cWrite(0xB0, 0x1357);
		NvBlAvpI2cWrite(0xB0, 0x0156);

		/*
		 * Set and enable VDD_1V2_LCD
		 *   LDO5_VOLTAGE = 1.2v (TPS65913)
		 *   LDO5_CTRL = Active
		 *
		 * Enable VDD_LCD_BL
		 *   VOUT1 (FET1) (TPS65090): auto discharge and enable
		 *
		 * Enable AVDD_LCD
		 *   VOUT4 (FET4) (TPS65090): auto discharge and enable
		 *
		 * Enable VDD_LVDS
		 *   VOUT5 (FET5) (TPS65090): auto discharge and enable
		 */
		NvBlAvpI2cWrite(0xB0, 0x0759);	/* LDO5_VOLTAGE */
		NvBlAvpI2cWrite(0xB0, 0x0158);	/* LD05_CTRL */
		NvBlAvpI2cWrite(0x90, 0x030F);	/* VOUT1 (FET1) */
		NvBlAvpI2cWrite(0x90, 0x0312);	/* VOUT4 (FET4) */
		NvBlAvpI2cWrite(0x90, 0x0313);	/* VOUT5 (FET5) */

		/*
		 * Set and enable VDD_SENSOR
		 *   LDO6_VOLTAGE = 2.85v
		 *   LDO6_CTRL = Active
		 */
		NvBlAvpI2cWrite(0xB0, 0x285B);
		NvBlAvpI2cWrite(0xB0, 0x015A);

		/*
		 * Set and enable AVDD_2V8_CAM_AF1
		 *   LDO7_VOLTAGE = 2.8v
		 *   LDO7_CTRL = Active
		 */
		NvBlAvpI2cWrite(0xB0, 0x275D);
		NvBlAvpI2cWrite(0xB0, 0x015C);

		/*
		 * Enable VDD_3V3_COM
		 *   VOUT7 (FET7) (TPS65090): auto discharge and enable
		 */
		NvBlAvpI2cWrite(0x90, 0x0315);
	}

    if (BoardId == DALMORE_E1613_BOARDID || BoardId == PLUTO_E1580_BOARDID) {
        /* Use GMI_CS1_N as CPUPWRGOOD -- is this really the SOC_THERM_OC2 ??? */
        Reg = NV_PMC_REGR(PMC_PA_BASE, CNTRL);
        Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, CNTRL, CPUPWRGOOD_SEL, SOC_THERM_OC2, Reg);
        NV_PMC_REGW(PMC_PA_BASE, CNTRL, Reg);

        /* set pinmux for GMI_CS1_N */
        Reg = NV_READ32(MISC_PA_BASE + PINMUX_AUX_GMI_CS1_N_0);
        Reg = NV_FLD_SET_DRF_DEF(PINMUX, AUX_GMI_CS1_N, PM, SOC, Reg);
        Reg = NV_FLD_SET_DRF_DEF(PINMUX, AUX_GMI_CS1_N, TRISTATE, NORMAL, Reg);
        Reg = NV_FLD_SET_DRF_DEF(PINMUX, AUX_GMI_CS1_N, E_INPUT, ENABLE, Reg);
        NV_WRITE32(MISC_PA_BASE + PINMUX_AUX_GMI_CS1_N_0, Reg);
    }

    /* set CPUPWRGOOD_TIMER -- APB clock is 1/2 of SCLK, 102M, 0x26E8F0 is 25ms */
    Reg = 0x26E8F0;
    NV_PMC_REGW(PMC_PA_BASE, CPUPWRGOOD_TIMER, Reg);

    /* enable CPUPWRGOOD feedback */
    Reg = NV_PMC_REGR(PMC_PA_BASE, CNTRL);
    Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, CNTRL, CPUPWRGOOD_EN, ENABLE, Reg);
    // test test test !!! NV_PMC_REGW(PMC_PA_BASE, CNTRL, Reg);

    /* polarity set to 0 (normal) */
    Reg = NV_PMC_REGR(PMC_PA_BASE, CNTRL);
    Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, CNTRL, CPUPWRREQ_POLARITY, NORMAL, Reg);
    NV_PMC_REGW(PMC_PA_BASE, CNTRL, Reg);

    /* CPUPWRREQ_OE set to 1 (enable) */
    Reg = NV_PMC_REGR(PMC_PA_BASE, CNTRL);
    Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, CNTRL, CPUPWRREQ_OE, ENABLE, Reg);
    NV_PMC_REGW(PMC_PA_BASE, CNTRL, Reg);

   /*
    * Set CLK_RST_CONTROLLER_CPU_SOFTRST_CTRL2_0_CAR2PMC_CPU_ACK_WIDTH to 408
    * to satisfy the requirement of having at least 16 CPU clock cycles before
    * clamp removal.
    */
    Reg = NV_CAR_REGR(CLK_RST_PA_BASE,CPU_SOFTRST_CTRL2);
    Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CPU_SOFTRST_CTRL2, CAR2PMC_CPU_ACK_WIDTH, 408, Reg);
    Reg = NV_CAR_REGW(CLK_RST_PA_BASE,CPU_SOFTRST_CTRL2, Reg);
}

