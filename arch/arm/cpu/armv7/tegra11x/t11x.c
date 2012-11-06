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

extern NvBool s_FpgaEmulation;
NvBool s_IsCortexA9      = 0xa5;  // set to non-zero so this won't be in .bss
NvBool s_UsePL310L2Cache = 0xa5;  // set to non-zero so this won't be in .bss
NvBool s_UseSCU          = 0xa5;  // set to non-zero so this won't be in .bss
NvBool s_UseLPAE =0xa5;

#define NV_ASSERT(p) \
{    if ((p) == 0)	\
	uart_post('?');	\
}

#define EEPROM_I2C_SLAVE_ADDRESS  0xAC
#define EEPROM_BOARDID_LSB_ADDRESS 0x4
#define EEPROM_BOARDID_MSB_ADDRESS 0x5

#define PMU_IGNORE_PWRREQ 1
static NvBlCpuClusterId AvpQueryFlowControllerClusterId(void);

UINT32 NvBlAvpQueryBootCpuFrequency( void )
{

    return 1000000;
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

void NvBlAvpEnableCpuPowerRail(void);

VOID AvpEnableCpuPowerRail(void)
{
//  UINT32   Reg;        // Scratch reg

//  Reg = NV_PMC_REGR(PMC_PA_BASE, CNTRL);
//  Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, CNTRL, CPUPWRREQ_OE, ENABLE, Reg);
//  NV_PMC_REGW(PMC_PA_BASE, CNTRL, Reg);
	uart_post('N'); uart_post('v'); uart_post('E'); uart_post('C'); uart_post('P'); uart_post('R');
	uart_post(0x0d);uart_post(0x0a);

	NvBlAvpEnableCpuPowerRail();
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
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_WDRESET1,  1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CORERESET1,1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CXRESET1,  1)

      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CPURESET2, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DBGRESET2, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DERESET2,  1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_WDRESET2,  1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CORERESET2,1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CXRESET2,  1)

      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CPURESET3, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DBGRESET3, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DERESET3,  1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_WDRESET3,  1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CORERESET3,1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CXRESET3,  1);
  NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPU_CMPLX_SET, Cpu);

  if (reset)
  {
      // Place CPU0  and the non-CPU partition in to reset.
      Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CPURESET0, 1)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DBGRESET0, 1)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DERESET0,  1)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CORERESET0,1)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CXRESET0,  1)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_NONCPURESET, 1);

      NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPU_CMPLX_SET, Cpu);

      // Enable master CPU reset.
       Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_L_SET, SET_CPU_RST, 1);
       NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEV_L_SET, Reg);

  }
  else
  {
       // Disable master CPU reset.
       Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_L_CLR, CLR_CPU_RST, 1);
       NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEV_L_CLR, Reg);

        // Take the non-CPU partition out of reset.
        Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_NONCPURESET, 1);
        NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPU_CMPLX_CLR, Cpu);
// Wait until non-CPU partition is out of reset.
do
{
	Cpu = NV_CAR_REGR(CLK_RST_PA_BASE, CPU_CMPLX_STATUS);
} while (NV_DRF_VAL(CLK_RST_CONTROLLER, CPU_CMPLX_STATUS, NONCPURESET, Cpu));


// make SOFTRST_CTRL2 equal to SOFTRST_CTRL1, this gurantees C0/C1NC is
// always removed out of reset earlier than CE0/CELP
Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CPU_SOFTRST_CTRL2);
NV_CAR_REGW(CLK_RST_PA_BASE, CPU_SOFTRST_CTRL1, Reg);

if (AvpQueryFlowControllerClusterId() == NvBlCpuClusterId_Fast)
{
	// Clear CPULP reset
	Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, RST_DEV_V_CLR, CLR_CPULP_RST, ENABLE);
	NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEV_V_CLR, Reg);

	// Clear NONCPU reset of S-cluster
	Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, RST_CPULP_CMPLX_CLR, CLR_NONCPURESET, ENABLE);
	NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPULP_CMPLX_CLR, Reg);
}

      // Take CPU0 out of reset.
      Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_CPURESET0, 1)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_DBGRESET0, 1)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_DERESET0,  1)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_CORERESET0,1)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_CXRESET0,  1);

      NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPU_CMPLX_CLR, Cpu);

  }
}

VOID AvpEnableCpuClock(BOOLEAN enable)
{
  UINT32   Reg;        // Scratch reg
  UINT32   Clk;        // Scratch reg

uart_post(0x0d);uart_post(0x0a);
  //-------------------------------------------------------------------------
  // NOTE:  Regardless of whether the request is to enable or disable the CPU
  //        clock, every processor in the CPU complex except the master (CPU
  //        0) will have it's clock stopped because the AVP only talks to the
  //        master. The AVP, it does not know, nor does it need to know that
  //        there are multiple processors in the CPU complex.
  //-------------------------------------------------------------------------

  // Always halt CPU 1-3 at the flow controller so that in uni-processor
  // configurations the low-power trigger conditions will work properly.
  uart_post('-'); uart_post('F'); uart_post('L'); uart_post('O'); uart_post('0');
  Reg = NV_DRF_DEF(FLOW_CTLR, HALT_CPU1_EVENTS, MODE, FLOW_MODE_STOP);
  NV_FLOW_REGW(FLOW_PA_BASE, HALT_CPU1_EVENTS, Reg);
  NV_FLOW_REGW(FLOW_PA_BASE, HALT_CPU2_EVENTS, Reg);
  NV_FLOW_REGW(FLOW_PA_BASE, HALT_CPU3_EVENTS, Reg);

  // Enabling clock?
  if (enable)
  {
  uart_post('-'); uart_post('e'); uart_post('n'); uart_post('a'); uart_post('b');

          // Wait for PLL-X to lock. T114 Dalmore hangs here!!! 6:30pm Tues
//tcw          do
//          {
//  uart_post('-'); uart_post('P'); uart_post('L'); uart_post('L'); uart_post('x');
//              Reg = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_BASE);
//tcw          } while (!NV_DRF_VAL(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, Reg));
  NvBlAvpStallUs(320);  //tcw !!!FIXME!!! Stall instead of waiting on lock

  uart_post('+'); uart_post('L'); uart_post('o'); uart_post('c'); uart_post('k');

      // *((volatile long *)0x60006020) = 0x20008888 ;// CCLK_BURST_POLICY
  uart_post('-'); uart_post('B'); uart_post('R'); uart_post('S'); uart_post('T');
      Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_STATE, 0x2)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, COP_AUTO_CWAKEUP_FROM_FIQ, 0x0)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_AUTO_CWAKEUP_FROM_FIQ, 0x0)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, COP_AUTO_CWAKEUP_FROM_IRQ, 0x0)
          | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_AUTO_CWAKEUP_FROM_IRQ, 0x0)
          | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_FIQ_SOURCE, PLLX_OUT0_LJ)
          | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_IRQ_SOURCE, PLLX_OUT0_LJ)
          | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_RUN_SOURCE, PLLX_OUT0_LJ)
          | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_IDLE_SOURCE, PLLX_OUT0_LJ);
      NV_CAR_REGW(CLK_RST_PA_BASE, CCLK_BURST_POLICY, Reg);
  }

  // *((volatile long *)0x60006024) = 0x80000000 ;// SUPER_CCLK_DIVIDER
  uart_post('-'); uart_post('S'); uart_post('U'); uart_post('P'); uart_post('R');
  Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_ENB, 0x1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_COP_FIQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_CPU_FIQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_COP_IRQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIS_FROM_CPU_IRQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIVIDEND, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_CCLK_DIVIDER, SUPER_CDIV_DIVISOR, 0x0);
  NV_CAR_REGW(CLK_RST_PA_BASE, SUPER_CCLK_DIVIDER, Reg);

  // Stop the clock to all CPUs.
  uart_post('-'); uart_post('S'); uart_post('T'); uart_post('O'); uart_post('P');
  Clk = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_SET, SET_CPU0_CLK_STP, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_SET, SET_CPU1_CLK_STP, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_SET, SET_CPU2_CLK_STP, 1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_SET, SET_CPU3_CLK_STP, 1);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_CPU_CMPLX_SET, Clk);

  // Enable the CPU0 clock if requested.
  if (enable)
  {
  uart_post('-'); uart_post('C'); uart_post('P'); uart_post('U'); uart_post('0'); uart_post('+');
      Clk = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_CPU_CMPLX_CLR, CLR_CPU0_CLK_STP, 1);
      NV_CAR_REGW(CLK_RST_PA_BASE, CLK_CPU_CMPLX_CLR, Clk);
  }

  // Always enable the main CPU complex clock.
  uart_post('-'); uart_post('C'); uart_post('P'); uart_post('U'); uart_post('M');
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
     // Is this an FPGA?
     if (s_FpgaEmulation) {
          // Leave CoreSight on the default clock source (oscillator).
     }
     else
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

          NvBlAvpStallUs(3);

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
#if 1//t11x_port
static NvBlCpuClusterId AvpQueryFlowControllerClusterId(void)
{
    NvBlCpuClusterId    ClusterId = NvBlCpuClusterId_G; // Active CPU id
    NvU32               Reg;                            // Scratch register

    Reg = NV_FLOW_REGR(FLOW_PA_BASE, CLUSTER_CONTROL);
    Reg = NV_DRF_VAL(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, Reg);

    if (Reg == NV_DRF_DEF(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, LP))
    {
        ClusterId = NvBlCpuClusterId_LP;
    }

    return ClusterId;
}

static NvBool AvpIsPartitionPowered(NvU32 Mask)
{
    // Get power gate status.
    NvU32   Reg = NV_PMC_REGR(PMC_PA_BASE, PWRGATE_STATUS);

    if ((Reg & Mask) == Mask)
    {
        return NV_TRUE;
    }

    return NV_FALSE;
}

static void AvpPowerPartition(NvU32 status, NvU32 toggle)
{
    // Is the partition already on?
    if (!AvpIsPartitionPowered(status))
    {
        // No, toggle the partition power state (OFF -> ON).
        NV_PMC_REGW(PMC_PA_BASE, PWRGATE_TOGGLE, toggle);

        // Wait for the power to come up.
        while (!AvpIsPartitionPowered(status))
        {
            // Do nothing
        }
    }
}

static void AvpRemoveCpuIoClamps(void)
{
    NvU32               Reg;        // Scratch reg

    // If booting to the fast cluster ...
    if (AvpQueryFlowControllerClusterId() == NvBlCpuClusterId_G)
    {
        // ... remove the clamps on the fast rail partition I/O signals ...
        Reg = PMC_CLAMPING_CMD(CRAIL);
        NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);

        // ... and the fast non-CPU I/O signals ...
        Reg = PMC_CLAMPING_CMD(C0NC);
        NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);

        // ...and the fast CPU0 I/O signals.
        Reg = PMC_CLAMPING_CMD(CE0);
        NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);
    }
    else
    {
        // Remove the clamps on the slow non-CPU I/O signals ...
        Reg = PMC_CLAMPING_CMD(C1NC);
        NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);

        // ...and the CPU0 I/O signals.
        Reg = PMC_CLAMPING_CMD(CELP);
        NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);
    }

    // Give I/O signals time to stabilize.
    NvBlAvpStallUs(20);  // !!!FIXME!!! THIS TIME HAS NOT BEEN CHARACTERIZED
}

#define POWER_PARTITION(x)  \
    AvpPowerPartition(PMC_PWRGATE_STATUS(x), PMC_PWRGATE_TOGGLE(x))

static void  AvpRamRepair(void)
{
//tcw hangs the system for some reason. Really needed??
//    NvU32   Reg;        // Scratch reg
//
//    // Can only be performed on fast cluster
//    NV_ASSERT(AvpQueryFlowControllerClusterId() == NvBlCpuClusterId_G);

    // Request RAM repair
//    Reg = NV_DRF_DEF(FLOW_CTLR, RAM_REPAIR, REQ, ENABLE);
//    NV_FLOW_REGW(FLOW_PA_BASE, RAM_REPAIR, Reg);

    // Wait for completion
//    do
//    {
//        Reg = NV_FLOW_REGR(FLOW_PA_BASE, RAM_REPAIR);
//    } while (!NV_DRF_VAL(FLOW_CTLR, RAM_REPAIR, STS, Reg));
}

static void  AvpPowerUpCpu(void)
{
    // Are we booting to the fast cluster?
    if (AvpQueryFlowControllerClusterId() == NvBlCpuClusterId_G)
    {
        // Power up the fast cluster rail partition.
        POWER_PARTITION(CRAIL);

        // Do RAM repair.
       AvpRamRepair();

        // Power up the fast cluster non-CPU partition.
        POWER_PARTITION(C0NC);

        // Power up the fast cluster CPU0 partition.
        POWER_PARTITION(CE0);
    }
    else
    {
        // Power up the slow cluster non-CPU partition.
        POWER_PARTITION(C1NC);

        // Power up the slow cluster CPU partition.
        POWER_PARTITION(CELP);
    }

    // Remove the I/O clamps from CPU power partition.
    AvpRemoveCpuIoClamps();
}



#else
//BOOLEAN AvpIsCpuPowered(NvBlCpuClusterId ClusterId)
//{
//  UINT32   Reg;        // Scratch reg
//  UINT32   Mask;       // Mask
//
//  // Get power gate status.
//  Reg = NV_PMC_REGR(PMC_PA_BASE, PWRGATE_STATUS);
//
//  // WARNING: Do not change the way this is coded (BUG 626323)
//  //          Otherwise, the compiler can generate invalid ARMv4i instructions
//  Mask = (ClusterId == NvBlCpuClusterId_LP) ?
//          NV_DRF_DEF(APBDEV_PMC, PWRGATE_STATUS, CELP, ON) :
//          NV_DRF_DEF(APBDEV_PMC, PWRGATE_STATUS, CRAIL, ON);
//
//  if (Reg & Mask)
//  {
//      return TRUE;
//  }
//
//  return FALSE;
//}

//VOID AvpRemoveCpuIoClamps(void)
//{
//  UINT32   Reg;        // Scratch reg
//
//  // Always remove the clamps on the LP CPU I/O signals.
//  Reg = NV_DRF_DEF(APBDEV_PMC, REMOVE_CLAMPING_CMD, CELP, ENABLE);
//  NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);
//
//  // If booting the G CPU cluster...
//      // ... remove the clamps on the G CPU0 I/O signals as well.
//      Reg = NV_DRF_DEF(APBDEV_PMC, REMOVE_CLAMPING_CMD, CRAIL, ENABLE);
//     NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);
//
//  // Give I/O signals time to stabilize.
//  NvBlAvpStallUs(20);  // !!!FIXME!!! THIS TIME HAS NOT BEEN CHARACTERIZED
//}

//VOID AvpPowerUpCpu(void)
//{
//  UINT32   Reg;        // Scratch reg
//
//  // Always power up the LP cluster
//  if (!AvpIsCpuPowered(NvBlCpuClusterId_LP))
//  {
//      // Toggle the LP CPU power state (OFF -> ON).
//      Reg = NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, PARTID, CELP)
//          | NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, START, ENABLE);
//      NV_PMC_REGW(PMC_PA_BASE, PWRGATE_TOGGLE, Reg);
//
//      // Wait for the power to come up.
//      while (!AvpIsCpuPowered(NvBlCpuClusterId_LP))
//      {
//          // Do nothing
//      }
//  }
//
//  // Are we booting to the G CPU cluster?
//      if (!AvpIsCpuPowered(NvBlCpuClusterId_G))
//      {
//          // Toggle the G CPU0 power state (OFF -> ON).
//          Reg = NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, PARTID, CRAIL)
//              | NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, START, ENABLE);
//          NV_PMC_REGW(PMC_PA_BASE, PWRGATE_TOGGLE, Reg);
//      }
//
//      // Wait for the power to come up.
//      while (!AvpIsCpuPowered(NvBlCpuClusterId_G))
//      {
//          // Do nothing
//      }
//
//  // Remove the I/O clamps from CPU power partition.
//  AvpRemoveCpuIoClamps();
//}
#endif
VOID AvpHaltAp20(void)
{
  UINT32   Reg;    // Scratch reg

  for (;;)
  {
      Reg = NV_DRF_DEF(FLOW_CTLR, HALT_COP_EVENTS, MODE, FLOW_MODE_STOP)
          | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, JTAG, 1)
          | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, LIC_IRQ, 1)
          | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, LIC_FIQ, 1);
      NV_FLOW_REGW(FLOW_PA_BASE, HALT_COP_EVENTS, Reg);
  }
}

VOID InitPllP(NvBootClocksOscFreq OscFreq)
{
	UINT32   Base, Misc;
    Base = NV_CAR_REGR(CLK_RST_PA_BASE, PLLP_BASE);
#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
	UINT32 OutA, OutB;
#endif
	Misc = Base = 0;
#if !defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
	if ((Base & NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, ENABLE)))
		{
			Misc = NV_CAR_REGR(CLK_RST_PA_BASE, PLLP_MISC);
			Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_LOCK_ENABLE, ENABLE, Misc);
			NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_MISC, Misc);
			return;
		}
#endif

    if (s_FpgaEmulation)
    {
        Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
             | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
             | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
             | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, DISABLE)
             | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
             | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, DEFAULT)
             | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, DEFAULT)
             | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, DEFAULT);
        Misc = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, DEFAULT);
    }
else {
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
  NvBlAvpStallUs(20);

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
	NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_OUTB, OutB);
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

//----------------------------------------------------------------------------------------------
static void InitPllX(NvBootClocksOscFreq OscFreq)
{
    NvU32               Base;       // PLL Base
    NvU32               Divm;       // Reference
    NvU32               Divn;       // Multiplier
    NvU32               Divp;       // Divider == Divp ^ 2
    NvU32               p;
    NvU32               Val;
    NvU32               OscFreqKhz, RateInKhz, VcoMin = 700000;

uart_post(0x0d);uart_post(0x0a);
    // Is PLL-X already running?
    Base = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_BASE);
    Base = NV_DRF_VAL(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, Base);
    if (Base == NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, ENABLE))
    {
  uart_post(' '); uart_post('r'); uart_post('t'); uart_post('n');
        return;
    }

    switch(OscFreq)
    {
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
    }

  uart_post('R'); uart_post('I'); uart_post('K'); uart_post('h');uart_post('z');
    RateInKhz = NvBlAvpQueryBootCpuFrequency() * 1000;

    if (s_FpgaEmulation)
    {
        OscFreqKhz = 13000; // 13 Mhz for FPGA
        RateInKhz = 600000; // 600 Mhz
    }

    /* Clip vco_min to exact multiple of OscFreq to avoid
       crossover by rounding */
    VcoMin = ((VcoMin + OscFreqKhz- 1) / OscFreqKhz)*OscFreqKhz;

    p = (VcoMin + RateInKhz - 1)/RateInKhz; // Rounding p
    Divp = p - 1;
    Divm = (OscFreqKhz> 19200)? 2 : 1; // cf_max = 19200 Khz
    Divn = (RateInKhz * p * Divm)/OscFreqKhz;

  uart_post('B'); uart_post('a'); uart_post('s'); uart_post('e');uart_post('X');
    Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, ENABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, DISABLE)
        | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_REF_DIS, REF_ENABLE)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVP, Divp)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVN, Divn)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_DIVM, Divm);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Base);

#if !NVBL_PLL_BYPASS
  uart_post('b'); uart_post('y'); uart_post('p'); uart_post('a');uart_post('s');
        Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, DISABLE, Base);
        NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Base);
#if NVBL_USE_PLL_LOCK_BITS
        {
  uart_post('M'); uart_post('i'); uart_post('s'); uart_post('c');uart_post('X');
            NvU32 Misc = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_MISC);
            Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_MISC, PLLX_LOCK_ENABLE, ENABLE, Misc);
            NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_MISC, Misc);
        }
#endif
#endif
    // Disable IDDQ
  uart_post('I'); uart_post('D'); uart_post('D'); uart_post('Q');uart_post('X');
    Val = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_MISC_3);
    Val = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, PLLX_MISC_3, PLLX_IDDQ, 0x0, Val);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_MISC_3, Val);

    // Wait 2 us
  uart_post('w'); uart_post('a'); uart_post('i'); uart_post('t');uart_post('2');
    NvBlAvpStallUs(2);

  uart_post('E'); uart_post('n'); uart_post('a'); uart_post('b');uart_post('X');
    Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, ENABLE, Base);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Base);
uart_post(0x0d);uart_post(0x0a);
}

#if 0
VOID InitPllX(NvBootClocksOscFreq OscFreq)
{
  UINT32               Base;       // PLL Base
  NvU32 			Misc; 	  // PLL Misc
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

  {
      Divm = 1;
      Divp = 0;
      Divn = NvBlAvpQueryBootCpuFrequency() / 1000;

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

  NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Base);

  Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_ENABLE, ENABLE, Base);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Base);

  Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_BYPASS, DISABLE, Base);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_BASE, Base);

  Misc = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_MISC);
  Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLX_MISC, PLLX_LOCK_ENABLE, ENABLE, Misc);
  NV_CAR_REGW(CLK_RST_PA_BASE, PLLX_MISC, Misc);
}
#endif

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
     && (!NV_DRF_VAL(FUSE, SKU_DIRECT_CONFIG, DISABLE_ALL, Fuse)))
    {
        return NvBlCpuClusterId_G;	//_Fast
    }

    // Is there a CPU present in the slow CPU complex?
    if (!NV_DRF_VAL(CLK_RST_CONTROLLER, BOND_OUT_V, BOND_OUT_CPULP, Reg))
    {
        return NvBlCpuClusterId_LP;	//_Slow
    }

    return NvBlCpuClusterId_Unknown;
}
//NvBlCpuClusterId NvBlAvpQueryBootCpuClusterId(void)
//{
//	return NvBlCpuClusterId_G;
//}

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
  const NvBootInfoTable*  pBootInfo;      // Boot Information Table pointer
  NvBootClocksOscFreq     OscFreq;        // Oscillator frequency
  UINT32                   Reg;            // Temporary register
  NvBlCpuClusterId        CpuClusterId;   // Boot CPU cluster id
  NvU32                   UsecCfg;        // Usec timer configuration register
  NvU32                   PllRefDiv;      // PLL reference divider

uart_post(0x0d);uart_post(0x0a);
  // Get a pointer to the Boot Information Table.
  uart_post('p'); uart_post('B'); uart_post('I'); uart_post('T');
  pBootInfo = (const NvBootInfoTable*)(T11X_BASE_PA_BOOT_INFO);

  // Get the oscillator frequency.
  uart_post('O'); uart_post('s'); uart_post('c'); uart_post('F');
    Reg = NV_CAR_REGR(CLK_RST_PA_BASE, OSC_CTRL);
    OscFreq = (NvBootClocksOscFreq)NV_DRF_VAL(CLK_RST_CONTROLLER, OSC_CTRL, OSC_FREQ, Reg);

  // For most oscillator frequencies, the PLL reference divider is 1.
  // Frequencies that require a different reference divider will set
  // it below.
  PllRefDiv = CLK_RST_CONTROLLER_OSC_CTRL_0_PLL_REF_DIV_DIV1;

  // Set up the oscillator dependent registers.
  // NOTE: Don't try to replace this switch statement with an array lookup.
  //	   Can't use global arrays here because the data segment isn't valid yet.

  switch (OscFreq)
  {
	  default:
  uart_post('F'); uart_post('R'); uart_post('Q'); uart_post('d');uart_post('f');
		  // Fall Through -- this is what the boot ROM does.
	  case NvBootClocksOscFreq_13:
  uart_post('F'); uart_post('R'); uart_post('Q'); uart_post('1');uart_post('3');
		  UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (1-1))
				  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (13-1));
		  break;

	  case NvBootClocksOscFreq_19_2:
  uart_post('F'); uart_post('R'); uart_post('Q'); uart_post('1');uart_post('9');
		  UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (5-1))
				  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (96-1));
		  break;

	  case NvBootClocksOscFreq_12:
  uart_post('F'); uart_post('R'); uart_post('Q'); uart_post('1');uart_post('2');
		  UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (1-1))
				  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (12-1));
		  break;

	  case NvBootClocksOscFreq_26:
  uart_post('F'); uart_post('R'); uart_post('Q'); uart_post('2');uart_post('6');
		  UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (1-1))
				  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (26-1));
		  break;

	  case NvBootClocksOscFreq_16_8:
  uart_post('F'); uart_post('R'); uart_post('Q'); uart_post('1');uart_post('6');
		  UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (5-1))
				  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (84-1));
		  break;

	  case NvBootClocksOscFreq_38_4:
  uart_post('F'); uart_post('R'); uart_post('Q'); uart_post('3');uart_post('8');
		  UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (5-1))
				  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (192-1));
		  PllRefDiv = CLK_RST_CONTROLLER_OSC_CTRL_0_PLL_REF_DIV_DIV2;
		  break;

	  case NvBootClocksOscFreq_48:
  uart_post('F'); uart_post('R'); uart_post('Q'); uart_post('4');uart_post('8');
		  UsecCfg = NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVIDEND, (1-1))
				  | NV_DRF_NUM(TIMERUS_USEC, CFG, USEC_DIVISOR, (48-1));
		  PllRefDiv = CLK_RST_CONTROLLER_OSC_CTRL_0_PLL_REF_DIV_DIV4;
		  break;
}

  // Find out which CPU cluster we're booting to.
  uart_post('C'); uart_post('l'); uart_post('t'); uart_post('I');uart_post('D');
  CpuClusterId = NvBlAvpQueryBootCpuClusterId();

  Reg = NV_FLOW_REGR(FLOW_PA_BASE, CLUSTER_CONTROL);
  // Are we booting to LP CPU complex?
  if (CpuClusterId == NvBlCpuClusterId_LP)
  {
  uart_post('C'); uart_post('l'); uart_post('t'); uart_post('L');uart_post('P');
      // Set active CPU cluster to LP.
      Reg = NV_FLD_SET_DRF_DEF(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, LP, Reg);
  }
  else
  {
  uart_post('C'); uart_post('l'); uart_post('t'); uart_post('=');uart_post('G');
      // Set active CPU cluster to G.
      Reg = NV_FLD_SET_DRF_DEF(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, G, Reg);
  }

  NV_FLOW_REGW(FLOW_PA_BASE, CLUSTER_CONTROL, Reg);

  // Running on an FPGA?
  if (s_FpgaEmulation)
  {
  uart_post('s'); uart_post('F'); uart_post('p'); uart_post('g');uart_post('a');
	  // Increase the reset delay time to maximum for FPGAs to avoid
	  // race conditions between WFI and reset propagation due to
	  // delays introduced by serializers.
	  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CPU_SOFTRST_CTRL);
	  Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CPU_SOFTRST_CTRL,
			  CPU_SOFTRST_LEGACY_WIDTH, 0xF0, Reg);
	  NV_CAR_REGW(CLK_RST_PA_BASE, CPU_SOFTRST_CTRL, Reg);
  }
#if 0
  // Get Oscillator Drive Strength Setting
  OscStrength = NvBlGetOscillatorDriveStrength();

  if (!IsChipInitialized)
  {
	  // Program the microsecond scaler.
	  NV_TIMERUS_REGW(TIMERUS_PA_BASE, USEC_CFG, UsecCfg);

	  // Program the oscillator control register.
	  OscCtrl = NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, OSC_FREQ, (int)OscFreq)
			  | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, PLL_REF_DIV, PllRefDiv)
			  | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, OSCFI_SPARE, 0)
			  | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XODS, 0)
			  | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOFS, OscStrength)
			  | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOBP, 0)
			  | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOE, 1);
	  NV_CAR_REGW(CLK_RST_PA_BASE, OSC_CTRL, OscCtrl);
  }
  else
  {
	  // Change the oscillator drive strength
	  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, OSC_CTRL);
	  Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOFS,
		  OscStrength, Reg);
	  NV_CAR_REGW(CLK_RST_PA_BASE, OSC_CTRL, Reg);
  }
#endif
  //-------------------------------------------------------------------------
  // If the boot ROM hasn't performed the PLLM initialization, we have to
  // do it here and always reconfigure PLLP.
  //-------------------------------------------------------------------------

#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ) && (!s_FpgaEmulation)
	/* Move AVP clock to CLM temporarily. Reset to PLLP_OUT4 later */
  uart_post('s'); uart_post('a'); uart_post('c'); uart_post('2');uart_post('c');
	set_avp_clock_to_clkm();
#endif

  // Initialize PLL-P.
  uart_post('I'); uart_post('n'); uart_post('P'); uart_post('L');uart_post('P');
  InitPllP(OscFreq);

  //-------------------------------------------------------------------------
  // Switch system clock to PLLP_out 4 (108 MHz) MHz, AVP will now run at 108 MHz.
  // This is glitch free as only the source is changed, no special precaution needed.
  //-------------------------------------------------------------------------

  // *((volatile long *)0x60006028) = 0x20002222 ;// SCLK_BURST_POLICY
  uart_post('S'); uart_post('C'); uart_post('L'); uart_post('K');uart_post('B');
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
  uart_post('S'); uart_post('u'); uart_post('p'); uart_post('e');uart_post('r');
  Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_ENB, 0x1)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIS_FROM_COP_FIQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIS_FROM_CPU_FIQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIS_FROM_COP_IRQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIS_FROM_CPU_IRQ, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIVIDEND, 0x0)
      | NV_DRF_NUM(CLK_RST_CONTROLLER, SUPER_SCLK_DIVIDER, SUPER_SDIV_DIVISOR, 0x0);
  NV_CAR_REGW(CLK_RST_PA_BASE, SUPER_SCLK_DIVIDER, Reg);

  // Initialize PLL-X.
  uart_post('I'); uart_post('n'); uart_post('P'); uart_post('L');uart_post('X');
  InitPllX(OscFreq);

  // *((volatile long *)0x60006030) = 0x00000010 ;// CLK_SYSTEM_RATE
  uart_post('C'); uart_post('l'); uart_post('k'); uart_post('S');uart_post('R');
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

  uart_post('E'); uart_post('c'); uart_post('t'); uart_post('p');uart_post('L');
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

  uart_post('E'); uart_post('c'); uart_post('t'); uart_post('p');uart_post('H');
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

  uart_post('E'); uart_post('c'); uart_post('t'); uart_post('p');uart_post('U');
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

  uart_post('E'); uart_post('c'); uart_post('t'); uart_post('p');uart_post('V');
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

  uart_post('E'); uart_post('c'); uart_post('t'); uart_post('p');uart_post('W');
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
  uart_post('M'); uart_post('S'); uart_post('E'); uart_post('L');uart_post('T');
  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_SOURCE_MSELECT);
  Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_MSELECT,
          MSELECT_CLK_SRC, PLLP_OUT0, Reg);
	/* Switch MSELECT clock divisor to 2, nvbug 881184 as per Joseph Lo */
//TCW	Reg &= 0xFFFFFF00;
//TCW	Reg |= 0x00000002;
//TCW  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_MSELECT, Reg);
//TCW
    // Clock divider request for 102MHz would setup MSELECT clock as 108MHz for PLLP base 216MHz
    // and 102MHz for PLLP base 408MHz.
    Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_MSELECT,
            MSELECT_CLK_DIVISOR, CLK_DIVIDER(NVBL_PLLP_KHZ, 102000), Reg);
    NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_MSELECT, Reg);
//TCW

  uart_post('P'); uart_post('M'); uart_post('U'); uart_post('I');uart_post('G');
  Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_SOURCE_I2C5);
  Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_I2C5,
          I2C5_CLK_DIVISOR, 0x10, Reg);
  NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_I2C5, Reg);

  // Give clocks time to stabilize.
  uart_post('1'); uart_post('m'); uart_post('s'); uart_post('e');uart_post('c');
  NvBlAvpStallMs(1);

uart_post(0x0d);uart_post(0x0a);
  // Take required peripherals out of reset.
  uart_post('T'); uart_post('p'); uart_post('o'); uart_post('r');uart_post('L');
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

  uart_post('T'); uart_post('p'); uart_post('o'); uart_post('r');uart_post('H');
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

  uart_post('T'); uart_post('p'); uart_post('o'); uart_post('r');uart_post('U');
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

  uart_post('T'); uart_post('p'); uart_post('o'); uart_post('r');uart_post('V');
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

  uart_post('T'); uart_post('p'); uart_post('o'); uart_post('r');uart_post('W');
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

  uart_post('D'); uart_post('o'); uart_post('n'); uart_post('e');uart_post('!');

  t11x_WriteDebugString("\nAVP will halt, and CPU should start!\n");
}

VOID NvBlStartCpu_T11x(UINT32 ResetVector)
{
  // Enable VDD_CPU
  uart_post('E'); uart_post('C'); uart_post('P'); uart_post('R');
  AvpEnableCpuPowerRail();

  // Hold the CPUs in reset.
  uart_post('A'); uart_post('R'); uart_post('C');
  AvpResetCpu(TRUE);

  // Disable the CPU clock.
  uart_post('A'); uart_post('E'); uart_post('C'); uart_post('C'); uart_post('0');
  AvpEnableCpuClock(FALSE);

  // Enable CoreSight.
  uart_post('A'); uart_post('C'); uart_post('E'); uart_post('C');
  AvpClockEnableCorsight(TRUE);

  // Set the entry point for CPU execution from reset, if it's a non-zero value.
  if (ResetVector)
  {
  uart_post('A'); uart_post('S'); uart_post('C'); uart_post('R'); uart_post('V');
      AvpSetCpuResetVector(ResetVector);
  }
  // Power up the CPU.
  uart_post('A'); uart_post('P'); uart_post('U'); uart_post('C');
  AvpPowerUpCpu();

  // Enable the CPU clock.
  uart_post('A'); uart_post('E'); uart_post('C'); uart_post('C');uart_post('1');
  AvpEnableCpuClock(TRUE);

  // Take the CPU out of reset.
  uart_post('A'); uart_post('R'); uart_post('C'); uart_post('0');
  AvpResetCpu(FALSE);

uart_post(0x0d);uart_post(0x0a);
}

void start_cpu_t11x(u32 reset_vector)
{
#if 0 //need it for A15 remove tmp dependency

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
#endif
  uart_post('c'); uart_post('i'); uart_post('t'); uart_post('x');
	ClockInitT11x();
uart_post(0x0d);uart_post(0x0a);
	NvBlStartCpu_T11x(reset_vector);
}

#if	0
/*
 * Copyright (c) 2009-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include "bootloader_t11x.h"
#include "nvassert.h"
#include "nverror.h"
#include "aos.h"
#include "nvuart.h"
#include "nvodm_services.h"

#pragma arm

#define EEPROM_I2C_SLAVE_ADDRESS  0xAC
#define EEPROM_BOARDID_LSB_ADDRESS 0x4
#define EEPROM_BOARDID_MSB_ADDRESS 0x5

extern NvBool s_FpgaEmulation;

#define PMC_CLAMPING_CMD(x)   \
    (NV_DRF_DEF(APBDEV_PMC, REMOVE_CLAMPING_CMD, x, ENABLE))
#define PMC_PWRGATE_STATUS(x) \
    (NV_DRF_DEF(APBDEV_PMC, PWRGATE_STATUS, x, ON))
#define PMC_PWRGATE_TOGGLE(x) \
    (NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, PARTID, x) | \
     NV_DRF_DEF(APBDEV_PMC, PWRGATE_TOGGLE, START, ENABLE))

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
     && (!NV_DRF_VAL(FUSE, SKU_DIRECT_CONFIG, DISABLE_ALL, Fuse)))
    {
        return NvBlCpuClusterId_Fast;
    }

    // Is there a CPU present in the slow CPU complex?
    if (!NV_DRF_VAL(CLK_RST_CONTROLLER, BOND_OUT_V, BOND_OUT_CPULP, Reg))
    {
        return NvBlCpuClusterId_Slow;
    }

    return NvBlCpuClusterId_Unknown;
}

static NvBool NvBlIsValidBootRomSignature(const NvBootInfoTable*  pBootInfo)
{
    // Look at the beginning of IRAM to see if the boot ROM placed a Boot
    // Information Table (BIT) there.
    // !!!FIXME!!! CHECK FOR CORRECT BOOT ROM VERSION IDS
    if (((pBootInfo->BootRomVersion == NVBOOT_VERSION(0x35,0x01))
      || (pBootInfo->BootRomVersion == NVBOOT_VERSION(0x35,0x02)))
    &&   (pBootInfo->DataVersion == NVBOOT_VERSION(0x35,0x01))
    &&   (pBootInfo->RcmVersion == NVBOOT_VERSION(0x35,0x01))
    &&   (pBootInfo->BootType == NvBootType_Cold)
    &&   (pBootInfo->PrimaryDevice == NvBootDevType_Irom))
    {
        // There is a valid Boot Information Table.
        return NV_TRUE;
    }

    return NV_FALSE;
}

static NvBool NvBlAvpIsChipInitialized( void )
{
    // Pointer to the start of IRAM which is where the Boot Information Table
    // will reside (if it exists at all).
    const NvBootInfoTable*  pBootInfo = (const NvBootInfoTable*)(T11X_BASE_PA_BOOT_INFO);

    // Look at the beginning of IRAM to see if the boot ROM placed a Boot
    // Information Table (BIT) there.
    if (NvBlIsValidBootRomSignature(pBootInfo))
    {
        // There is a valid Boot Information Table.
        // Return status the boot ROM gave us.
        return pBootInfo->DevInitialized;
    }

    return NV_FALSE;
}

static NvBool NvBlAvpIsChipInRecovery( void )
{
    // Pointer to the start of IRAM which is where the Boot Information Table
    // will reside (if it exists at all).
    const NvBootInfoTable*  pBootInfo = (const NvBootInfoTable*)(T11X_BASE_PA_BOOT_INFO);

    // Look at the beginning of IRAM to see if the boot ROM placed a Boot
    // Information Table (BIT) there.
    // !!!FIXME!!! CHECK FOR CORRECT BOOT ROM VERSION IDS
    if (((pBootInfo->BootRomVersion == NVBOOT_VERSION(0x35,0x01))
      || (pBootInfo->BootRomVersion == NVBOOT_VERSION(0x35,0x02)))
    &&   (pBootInfo->DataVersion == NVBOOT_VERSION(0x35,0x01))
    &&   (pBootInfo->RcmVersion == NVBOOT_VERSION(0x35,0x01))
    &&   (pBootInfo->BootType == NvBootType_Recovery)
    &&   (pBootInfo->PrimaryDevice == NvBootDevType_Irom))
    {
        return NV_TRUE;
    }

    return NV_FALSE;
}

static NvU32 NvBlGetOscillatorDriveStrength( void )
{
    //This function is pretty hacky. Ideally,
    //you want to get this from the ODM, but
    //I didn't want to compile the ODM into nvml.
    //Keeping the default value of POR
    //Refer Bug - 1044151 for more details
    return 0x07;
}

static NvU32  NvBlAvpQueryBootCpuFrequency( void )
{
    NvU32   frequency;

    if (s_FpgaEmulation)
    {
        frequency = NvBlAvpQueryOscillatorFrequency() / 1000;
    }
    else
    {
        #if     NVBL_PLL_BYPASS
            // In bypass mode we run at the oscillator frequency.
            frequency = NvBlAvpQueryOscillatorFrequency();
        #else
            // !!!FIXME!!! ADD SKU VARIANT FREQUENCIES
            if (NvBlAvpQueryFlowControllerClusterId() == NvBlCpuClusterId_Fast)
            {
                frequency = 700;
            }
            else
            {
                frequency = 350;
            }
        #endif
    }

    return frequency;
}

static NvU32  NvBlAvpQueryOscillatorFrequency( void )
{
    NvU32               Reg;
    NvU32               Oscillator;
    NvBootClocksOscFreq Freq;

    Reg = NV_CAR_REGR(CLK_RST_PA_BASE, OSC_CTRL);
    Freq = (NvBootClocksOscFreq)NV_DRF_VAL(CLK_RST_CONTROLLER, OSC_CTRL, OSC_FREQ, Reg);

    switch (Freq)
    {
        case NvBootClocksOscFreq_13:
            Oscillator = 13000;
            break;

        case NvBootClocksOscFreq_19_2:
            Oscillator = 19200;
            break;

        case NvBootClocksOscFreq_12:
            Oscillator = 12000;
            break;

        case NvBootClocksOscFreq_26:
            Oscillator = 26000;
            break;

        case NvBootClocksOscFreq_16_8:
            Oscillator = 16800;
            break;

        case NvBootClocksOscFreq_38_4:
            Oscillator = 38400;
            break;

        case NvBootClocksOscFreq_48:
            Oscillator = 48000;
            break;

        default:
            NV_ASSERT(0);
            Oscillator = 0;
            break;
    }

    return Oscillator;
}

static NvBootClocksOscFreq NvBlAvpOscillatorCountToFrequency(NvU32 Count)
{
    if ((Count >= NVBOOT_CLOCKS_MIN_CNT_12) && (Count <= NVBOOT_CLOCKS_MAX_CNT_12))
    {
        return NvBootClocksOscFreq_12;
    }

    if ((Count >= NVBOOT_CLOCKS_MIN_CNT_13) && (Count <= NVBOOT_CLOCKS_MAX_CNT_13))
    {
        return NvBootClocksOscFreq_13;
    }

    if ((Count >= NVBOOT_CLOCKS_MIN_CNT_19_2) && (Count <= NVBOOT_CLOCKS_MAX_CNT_19_2))
    {
        return NvBootClocksOscFreq_19_2;
    }

    if ((Count >= NVBOOT_CLOCKS_MIN_CNT_26) && (Count <= NVBOOT_CLOCKS_MAX_CNT_26))
    {
        return NvBootClocksOscFreq_26;
    }

    if ((Count >= NVBOOT_CLOCKS_MIN_CNT_16_8) && (Count <= NVBOOT_CLOCKS_MAX_CNT_16_8))
    {
        return NvBootClocksOscFreq_16_8;
    }

    if ((Count >= NVBOOT_CLOCKS_MIN_CNT_38_4) && (Count <= NVBOOT_CLOCKS_MAX_CNT_38_4))
    {
        return NvBootClocksOscFreq_38_4;
    }

    if ((Count >= NVBOOT_CLOCKS_MIN_CNT_48) && (Count <= NVBOOT_CLOCKS_MAX_CNT_48))
    {
        return NvBootClocksOscFreq_48;
    }

    return NvBootClocksOscFreq_Unknown;
}

static void NvBlAvpEnableCpuClock(NvBool enable)
{
    NvU32   Reg;        // Scratch reg
    NvU32   Clk;        // Scratch reg

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
        #if NVBL_USE_PLL_LOCK_BITS

            // Wait for PLL-X to lock.
            do
            {
                Reg = NV_CAR_REGR(CLK_RST_PA_BASE, PLLX_BASE);
            } while (!NV_DRF_VAL(CLK_RST_CONTROLLER, PLLX_BASE, PLLX_LOCK, Reg));

        #else

            // Give PLLs time to stabilize.
            NvBlAvpStallUs(NVBOOT_CLOCKS_PLL_STABILIZATION_DELAY);

        #endif

        // *((volatile long *)0x60006020) = 0x20008888 ;// CCLK_BURST_POLICY
        Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_STATE, 0x2)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, COP_AUTO_CWAKEUP_FROM_FIQ, 0x0)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_AUTO_CWAKEUP_FROM_FIQ, 0x0)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, COP_AUTO_CWAKEUP_FROM_IRQ, 0x0)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CPU_AUTO_CWAKEUP_FROM_IRQ, 0x0)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_FIQ_SOURCE, PLLX_OUT0_LJ)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_IRQ_SOURCE, PLLX_OUT0_LJ)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_RUN_SOURCE, PLLX_OUT0_LJ)
            | NV_DRF_DEF(CLK_RST_CONTROLLER, CCLK_BURST_POLICY, CWAKEUP_IDLE_SOURCE, PLLX_OUT0_LJ);
        NV_CAR_REGW(CLK_RST_PA_BASE, CCLK_BURST_POLICY, Reg);
    }

    // *((volatile long *)0x60006024) = 0x80000000 ;// SUPER_CCLK_DIVIDER
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

static void NvBlAvpResetCpu(NvBool reset)
{
    NvU32   Reg;    // Scratch reg
    NvU32   Cpu;    // Scratch reg

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
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_WDRESET1,  1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CPURESET2, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DBGRESET2, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DERESET2,  1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_WDRESET2,  1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CPURESET3, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DBGRESET3, 1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DERESET3,  1)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_WDRESET3,  1);
    NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPU_CMPLX_SET, Cpu);

    if (reset)
    {
        // Place CPU0 and the non-CPU partition into reset.
        Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CPURESET0, 1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DBGRESET0, 1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_DERESET0,  1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CORERESET0,1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_CXRESET0,  1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_SET, SET_NONCPURESET, 1);
        NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPU_CMPLX_SET, Cpu);

        // Enable master CPU reset.
        Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_DEV_L_SET, SET_CPU_RST, 1);
        NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEV_L_SET, Reg);
    }
    else
    {
        // Take the slow non-CPU partition out of reset.
        Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPULP_CMPLX_CLR, CLR_NONCPURESET, 1);
        NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPULP_CMPLX_CLR, Cpu);

        // Take the fast non-CPU partition out of reset.
        Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_NONCPURESET, 1);
        NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPUG_CMPLX_CLR, Cpu);

        // Wait until current non-CPU partition is out of reset.
        do
        {
            Cpu = NV_CAR_REGR(CLK_RST_PA_BASE, CPU_CMPLX_STATUS);
        } while (NV_DRF_VAL(CLK_RST_CONTROLLER, CPU_CMPLX_STATUS, NONCPURESET, Cpu));


        if (s_FpgaEmulation)
        {
             // make SOFTRST_CTRL2 equal to SOFTRST_CTRL1, this gurantees C0NC/C1NC is
             // always removed out of reset earlier than CE0/CELP
             Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CPU_SOFTRST_CTRL2);
             NV_CAR_REGW(CLK_RST_PA_BASE, CPU_SOFTRST_CTRL1, Reg);
        }

        // Clear software controlled reset of slow cluster
        Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPULP_CMPLX_CLR, CLR_CPURESET0, 1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPULP_CMPLX_CLR, CLR_DBGRESET0, 1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPULP_CMPLX_CLR, CLR_DERESET0,  1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPULP_CMPLX_CLR, CLR_CORERESET0,1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPULP_CMPLX_CLR, CLR_CXRESET0,  1);
        NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPULP_CMPLX_CLR, Reg);

        // Clear software controlled reset of fast cluster
        Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CPURESET0, 1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_DBGRESET0, 1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_DERESET0,  1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CORERESET0,1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPUG_CMPLX_CLR, CLR_CXRESET0,  1);
        NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPUG_CMPLX_CLR, Cpu);

        // Clear software controlled reset of current cluster
        Cpu = NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_CPURESET0, 1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_DBGRESET0, 1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_DERESET0,  1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_CORERESET0,1)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, RST_CPU_CMPLX_CLR, CLR_CXRESET0,  1);
        NV_CAR_REGW(CLK_RST_PA_BASE, RST_CPU_CMPLX_CLR, Cpu);
    }
}

static void NvBlAvpClockEnableCorsight(NvBool enable)
{
    NvU32   Reg;        // Scratch register

    if (enable)
    {
        // Is this an FPGA?
        if (s_FpgaEmulation)
        {
            // Leave CoreSight on the default clock source (oscillator).
        }
        else
        {
            // Program the CoreSight clock source and divider (but not at the same time).
            // Note that CoreSight has a fractional divider (LSB == .5).
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_SOURCE_CSITE);

            // Clock divider request for 136MHz would setup CSITE clock as
            // 144MHz for PLLP base 216MHz and 136MHz for PLLP base 408MHz.
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_CSITE,
                    CSITE_CLK_DIVISOR, CLK_DIVIDER(NVBL_PLLP_KHZ, 136000), Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_CSITE, Reg);

            NvBlAvpStallUs(3);

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

static void SetAvpClockToClkM()
{
    NvU32 RegData;

    RegData = NV_CAR_REGR(CLK_RST_PA_BASE, SCLK_BURST_POLICY);
    RegData = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY,
                    SYS_STATE, RUN, RegData);
    RegData = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY,
                    SWAKEUP_RUN_SOURCE, CLKM, RegData);
    NV_CAR_REGW(CLK_RST_PA_BASE, SCLK_BURST_POLICY, RegData);
    NvBlAvpStallUs(3);
}

static void SetAvpClockToPllP()
{
    NvU32 RegData;

    RegData = NV_CAR_REGR(CLK_RST_PA_BASE, SCLK_BURST_POLICY);
    RegData = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY,
                    SYS_STATE, RUN, RegData);
    RegData = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, SCLK_BURST_POLICY,
                  SWAKEUP_RUN_SOURCE, PLLP_OUT4, RegData);
    NV_CAR_REGW(CLK_RST_PA_BASE, SCLK_BURST_POLICY, RegData);
}

static void InitPllP(NvBootClocksOscFreq OscFreq)
{
    NvU32   Base;
    NvU32   Misc;
    NvU32   Reg;

    Base = NV_CAR_REGR(CLK_RST_PA_BASE, PLLP_BASE);
    if ( (!CONFIG_PLLP_BASE_AS_408MHZ) &&
         (Base & NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, ENABLE)))
    {
#if NVBL_USE_PLL_LOCK_BITS
        Misc = NV_CAR_REGR(CLK_RST_PA_BASE, PLLP_MISC);
        Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_LOCK_ENABLE, ENABLE, Misc);
        NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_MISC, Misc);
#endif
        return;
    }

    if (s_FpgaEmulation)
    {
        Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
             | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
             | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
             | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, DISABLE)
             | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
             | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, DEFAULT)
             | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, DEFAULT)
             | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, DEFAULT);
        Misc = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, DEFAULT);
    }
    else
    {
        SetAvpClockToClkM();
        // DIVP/DIVM/DIVN values taken from arclk_rst.h table for fixed 216 MHz operation.
        switch (OscFreq)
        {
            case NvBootClocksOscFreq_13:
                #if NVBL_PLL_BYPASS
                    Base = 0x80000D0D;
                #else
                Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, ENABLE)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
#if CONFIG_PLLP_BASE_AS_408MHZ
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x0)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0x198)
#else
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0xD8)
#endif
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0xD);
                #endif
                Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 8);
                break;

            case NvBootClocksOscFreq_38_4:
                // Fall through -- 38.4 MHz is identical to 19.2 MHz except
                // that the PLL reference divider has been previously set
                // to divide by 2 for 38.4 MHz.

            case NvBootClocksOscFreq_19_2:
                #if NVBL_PLL_BYPASS
                    Base = 0x80001313;
                #else
                Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, ENABLE)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0xB4)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0x16);
                #endif
                Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 4);
                break;

            case NvBootClocksOscFreq_12:
                #if NVBL_PLL_BYPASS
                    Base = 0x80000C0C;
                #else
                Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, ENABLE)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
#if CONFIG_PLLP_BASE_AS_408MHZ
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x0)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0x198)
#else
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0xD8)
#endif
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0x0C);
                #endif
                Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 8);
                break;

            case NvBootClocksOscFreq_26:
                #if NVBL_PLL_BYPASS
                    Base = 0x80001A1A;
                #else
                Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BASE_OVRRIDE, ENABLE)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
#if CONFIG_PLLP_BASE_AS_408MHZ
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x0)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0x198)
#else
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0xD8)
#endif
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0x1A);
                #endif
                Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 8);
                break;

            case NvBootClocksOscFreq_16_8:
                #if NVBL_PLL_BYPASS
                    Base = 0x80001111;
                #else
                Base = NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, DISABLE)
                     | NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_REF_DIS, REF_ENABLE)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, 0x0)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVP, 0x1)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVN, 0xB4)
                     | NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_DIVM, 0x14);
                #endif
                Misc = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_CPCON, 4);
                break;

            default:
                Misc = 0; /* Suppress warning */
                NV_ASSERT(!"Bad frequency");
        }
    }

    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_MISC, Misc);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Base);

    // Assert OUTx_RSTN for pllp_out1,2,3,4 before PLLP enable
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

    Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_ENABLE, ENABLE, Base);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Base);

#if !NVBL_PLL_BYPASS
    Base = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, DISABLE, Base);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_BASE, Base);

#if CONFIG_PLLP_BASE_AS_408MHZ
    // Set pllp_out1,2,3,4 to frequencies 48MHz, 48MHz, 102MHz, 102MHz.
    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_RATIO, 83) | // 9.6MHz
          NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_OVRRIDE, ENABLE) |
          NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_CLKEN, ENABLE) |
          NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_RATIO, 15) |  // 48MHz
          NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_OVRRIDE, ENABLE) |
          NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_CLKEN, ENABLE);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_OUTA, Reg);

    Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_RATIO, 6) |  // 102MHz
          NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_OVRRIDE, ENABLE) |
          NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_CLKEN, ENABLE) |
          NV_DRF_NUM(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_RATIO, 6) | // 102MHz
          NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_OVRRIDE, ENABLE) |
          NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_CLKEN, ENABLE);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_OUTB, Reg);
#endif

#if NVBL_USE_PLL_LOCK_BITS
    Misc = NV_CAR_REGR(CLK_RST_PA_BASE, PLLP_MISC);
    Misc = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_MISC, PLLP_LOCK_ENABLE, ENABLE, Misc);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_MISC, Misc);
#endif
#endif
}

//----------------------------------------------------------------------------------------------
static void NvBlAvpClockInit(NvBool IsChipInitialized, NvBool IsLoadedByScript)
{
    const NvBootInfoTable*  pBootInfo;      // Boot Information Table pointer
    NvBootClocksOscFreq     OscFreq;        // Oscillator frequency
    NvU32                   Reg;            // Temporary register
    NvU32                   OscCtrl;        // Oscillator control register
    NvU32                   OscStrength;    // Oscillator Drive Strength
    NvU32                   UsecCfg;        // Usec timer configuration register
    NvU32                   PllRefDiv;      // PLL reference divider
    NvBlCpuClusterId        CpuClusterId;   // Boot CPU id

    // Get a pointer to the Boot Information Table.
    pBootInfo = (const NvBootInfoTable*)(T11X_BASE_PA_BOOT_INFO);

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

    // Find out which CPU we're booting to.
    CpuClusterId = NvBlAvpQueryBootCpuClusterId();

    Reg = NV_FLOW_REGR(FLOW_PA_BASE, CLUSTER_CONTROL);
    // Are we booting to slow CPU complex?
    if (CpuClusterId == NvBlCpuClusterId_Slow)
    {
        // Set active cluster to the slow cluster.
        Reg = NV_FLD_SET_DRF_DEF(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, LP, Reg);
    }
    else
    {
        // Set active cluster to the fast cluster.
        Reg = NV_FLD_SET_DRF_DEF(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, G, Reg);
    }
    NV_FLOW_REGW(FLOW_PA_BASE, CLUSTER_CONTROL, Reg);

    // Running on an FPGA?
    if (s_FpgaEmulation)
    {
        // Increase the reset delay time to maximum for FPGAs to avoid
        // race conditions between WFI and reset propagation due to
        // delays introduced by serializers.
        Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CPU_SOFTRST_CTRL);
        Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CPU_SOFTRST_CTRL,
                CPU_SOFTRST_LEGACY_WIDTH, 0xF0, Reg);
        NV_CAR_REGW(CLK_RST_PA_BASE, CPU_SOFTRST_CTRL, Reg);
    }

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

    if (!IsChipInitialized)
    {
        // Program the microsecond scaler.
        NV_TIMERUS_REGW(TIMERUS_PA_BASE, USEC_CFG, UsecCfg);

        // Program the oscillator control register.
        OscCtrl = NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, OSC_FREQ, (int)OscFreq)
                | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, PLL_REF_DIV, PllRefDiv)
                | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, OSCFI_SPARE, 0)
                | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XODS, 0)
                | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOFS, OscStrength)
                | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOBP, 0)
                | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOE, 1);
        NV_CAR_REGW(CLK_RST_PA_BASE, OSC_CTRL, OscCtrl);
    }
    else
    {
        // Change the oscillator drive strength
        Reg = NV_CAR_REGR(CLK_RST_PA_BASE, OSC_CTRL);
        Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, OSC_CTRL, XOFS, OscStrength, Reg);
        NV_CAR_REGW(CLK_RST_PA_BASE, OSC_CTRL, Reg);
        // Update same value in PMC_OSC_EDPD_OVER XOFS field for warmboot
        Reg = NV_PMC_REGR(PMC_PA_BASE, OSC_EDPD_OVER);
        Reg = NV_FLD_SET_DRF_NUM(APBDEV_PMC, OSC_EDPD_OVER, XOFS, OscStrength, Reg);
        NV_PMC_REGW(PMC_PA_BASE, OSC_EDPD_OVER, Reg);
    }

    // Initialize PLL-P.
    InitPllP(OscFreq);

#if NVBL_USE_PLL_LOCK_BITS

    // Wait for PLL-P to lock.
    do
    {
        Reg = NV_CAR_REGR(CLK_RST_PA_BASE, PLLP_BASE);
    } while (!NV_DRF_VAL(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_LOCK, Reg));

#else

    // Give PLLs time to stabilize.
    NvBlAvpStallUs(NVBOOT_CLOCKS_PLL_STABILIZATION_DELAY);

#endif

    /* Deassert OUTx_RSTN for pllp_out1,2,3,4 after lock bit
     * or waiting for Stabilization Delay (S/W Bug 954659)
     */
    Reg = NV_CAR_REGR(CLK_RST_PA_BASE,PLLP_OUTA);
    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT2_RSTN,
              RESET_DISABLE,Reg);
    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTA, PLLP_OUT1_RSTN,
              RESET_DISABLE,Reg);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_OUTA, Reg);

    Reg = NV_CAR_REGR(CLK_RST_PA_BASE,PLLP_OUTB);
    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT4_RSTN,
              RESET_DISABLE,Reg);
    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB, PLLP_OUT3_RSTN,
              RESET_DISABLE,Reg);
    NV_CAR_REGW(CLK_RST_PA_BASE, PLLP_OUTB, Reg);

    SetAvpClockToPllP();

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
#if NVBL_PLL_BYPASS
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, AHB_RATE, 0x0)
#else
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, AHB_RATE, 0x1)
#endif
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, PCLK_DIS, 0x0)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SYSTEM_RATE, APB_RATE, 0x0);
    NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SYSTEM_RATE, Reg);

    //-------------------------------------------------------------------------
    // If the boot ROM hasn't already enabled the clocks to the memory
    // controller we have to do it here.
    //-------------------------------------------------------------------------

    if (!IsChipInitialized)
    {
        // *((volatile long *)0x6000619C) = 0x03000000 ;//  CLK_SOURCE_EMC
        Reg = NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_EMC, EMC_2X_CLK_SRC, 0x0)
            | NV_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_EMC, EMC_2X_CLK_DIVISOR, 0x0);
        NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_EMC, Reg);
    }

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
    // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, CLK_ENB_HSI, ENABLE, Reg);
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
    // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, CLK_ENB_XUSB_DEV, ENABLE, Reg);
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
    // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_V, CLK_ENB_EXTPERIPH1, ENABLE, Reg);
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

    Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_SOURCE_I2C5);
    Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_SOURCE_I2C5,
            I2C5_CLK_DIVISOR, 0x10, Reg);
    NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_I2C5, Reg);

    // Give clocks time to stabilize.
    NvBlAvpStallMs(1);

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
    if (s_FpgaEmulation)
    {
        Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U, SWR_EMUCIF_RST, DISABLE, Reg);
    }
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
    Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_MSELECT_RST, DISABLE, Reg);
    // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_3D2_RST, ENABLE, Reg);
    // Reg = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_V, SWR_CPULP_RST, ENABLE, Reg);
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
    NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_W, Reg);
}

static void NvBlAvpClockDeInit(void)
{
    // !!!FIXME!!! Need to implement this.
}


static NvBootClocksOscFreq NvBlAvpMeasureOscFreq(void)
{
    NvU32 Reg;
    NvU32 Count;

    // Start measurement, window size uses n-1 coding
    Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, OSC_FREQ_DET, OSC_FREQ_DET_TRIG, ENABLE)
        | NV_DRF_NUM(CLK_RST_CONTROLLER, OSC_FREQ_DET, REF_CLK_WIN_CFG, (1-1));
    NV_CAR_REGW(CLK_RST_PA_BASE, OSC_FREQ_DET, Reg);

    // wait until the measurement is done
    do
    {
        Reg = NV_CAR_REGR(CLK_RST_PA_BASE, OSC_FREQ_DET_STATUS);
    }
    while (NV_DRF_VAL(CLK_RST_CONTROLLER, OSC_FREQ_DET_STATUS, OSC_FREQ_DET_BUSY, Reg));

    Count = NV_DRF_VAL(CLK_RST_CONTROLLER, OSC_FREQ_DET_STATUS, OSC_FREQ_DET_CNT, Reg);

    // Convert the measured count to a frequency.
    return NvBlAvpOscillatorCountToFrequency(Count);
}

static void NvBlMemoryControllerInit(NvBool IsChipInitialized)
{
    NvU32   Reg;                // Temporary register
    NvU32   NorSize = 0;        // Size of NOR
    NvBool  IsSdramInitialized; // Nonzero if SDRAM already initialize

    // Get a pointer to the Boot Information Table.
    const NvBootInfoTable*  pBootInfo = (const NvBootInfoTable*)(T11X_BASE_PA_BOOT_INFO);

    // Get SDRAM initialization state.
    IsSdramInitialized = pBootInfo->SdramInitialized;

    //-------------------------------------------------------------------------
    // EMC/MC (Memory Controller) Initialization
    //-------------------------------------------------------------------------

    // Has the boot rom initialized the memory controllers?
    if (!IsSdramInitialized)
    {
        NV_ASSERT(0);   // NOT SUPPORTED
    }

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

#if !NV_TEST_LOADER // Test loader does not support ODM query
    {
        // Get the amount of NOR memory present.
        NvOdmOsOsInfo OsInfo;       // OS information
        NvOsAvpMemset(&OsInfo, 0, sizeof(OsInfo));
        OsInfo.OsType = NvOdmOsOs_Linux;
        NorSize = NvOdmQueryOsMemSize(NvOdmMemoryType_Nor, &OsInfo);
    }
#endif

    if (NorSize != 0)
    {
        Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, NOR_A1, MMIO, Reg);
        if (NorSize > 0x01000000)
        {
            Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, NOR_A2, MMIO, Reg);
            if (NorSize > 0x03000000)
            {
                Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, GLB_AMAP_CFG, NOR_A3, MMIO, Reg);
            }
        }
    }
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
}

NvBool NvBlAvpInit_T11x(NvBool IsRunningFromSdram)
{
    NvU32               Reg;
    NvBool              IsChipInitialized;
    NvBool              IsLoadedByScript = NV_FALSE;
    NvBootInfoTable*    pBootInfo = (NvBootInfoTable*)(T11X_BASE_PA_BOOT_INFO);

    // Upon entry, IsRunningFromSdram reflects whether or not we're running out
    // of NOR (== NV_FALSE) or SDRAM (== NV_TRUE).
    //
    // (1)  If running out of SDRAM, whoever put us there may or may not have
    //      initialized everything. It is possible that this code was put here
    //      by a direct download over JTAG into SDRAM.
    // (2)  If running out of NOR, the chip may or may not have been initialized.
    //      In the case of secondary boot from NOR, the boot ROM will have already
    //      set up everything properly but we'll be executing from NOR. In that
    //      situation, the bootloader must not attempt to reinitialize the modules
    //      the boot ROM has already initialized.
    //
    // In either case, if the chip has not been initialized (as indicated by no
    // valid signature in the Boot Information Table) we must take steps to do
    // it ourself.

    // See if the boot ROM initialized us.
    IsChipInitialized = NvBlAvpIsChipInitialized();

    // Has the boot ROM done it's part of chip initialization?
    if (!IsChipInitialized)
    {
        // Is this the special case of the boot loader being directly deposited
        // in SDRAM by a debugger script?
        if ((pBootInfo->BootRomVersion == ~0)
        &&  (pBootInfo->DataVersion    == ~0)
        &&  (pBootInfo->RcmVersion     == ~0)
        &&  (pBootInfo->BootType       == ~0)
        &&  (pBootInfo->PrimaryDevice  == ~0))
        {
            // The script must have initialized everything necessary to get
            // the chip up so don't try to reinitialize.
            IsChipInitialized = NV_TRUE;
            IsLoadedByScript  = NV_TRUE;
        }

        // Was the bootloader placed in SDRAM by a USB Recovery Mode applet?
        if (IsRunningFromSdram && NvBlAvpIsChipInRecovery())
        {
            // Keep the Boot Information Table setup by that applet
            // but mark the SDRAM and device as initialized.
            pBootInfo->SdramInitialized = NV_TRUE;
            pBootInfo->DevInitialized   = NV_TRUE;
            IsChipInitialized           = NV_TRUE;
        }
        else
        {
            // We must be doing primary boot from NOR or a script boot.
            // Build a fake BIT structure in IRAM so that the rest of
            // the code can proceed as if the boot ROM were present.
            NvOsAvpMemset(pBootInfo, 0, sizeof(NvBootInfoTable));

            // Fill in the parts we need.
            pBootInfo->PrimaryDevice   = NvBootDevType_Nor;
            pBootInfo->SecondaryDevice = NvBootDevType_Nor;
            pBootInfo->OscFrequency    = NvBlAvpMeasureOscFreq();
            pBootInfo->BootType        = NvBootType_Cold;

            // Did a script load this code?
            if (IsLoadedByScript)
            {
                // Scripts must initialize the whole world.
                pBootInfo->DevInitialized   = NV_TRUE;
                pBootInfo->SdramInitialized = NV_TRUE;
            }
        }
    }

    // Enable the boot clocks.
    NvBlAvpClockInit(IsChipInitialized, IsLoadedByScript);
#if __GNUC__
    // Enable Uart after PLLP init
    NvAvpUartInit(PLLP_FIXED_FREQ_KHZ_408000);
    NvOsAvpDebugPrintf("Bootloader AVP Init\n");
#endif
    // Disable all module clocks except which are required to boot.
    NvBlAvpClockDeInit();

    // Initialize memory controller.
    NvBlMemoryControllerInit(IsChipInitialized);

    // Set power gating timer multiplier -- see bug 966323
    Reg = NV_PMC_REGR(PMC_PA_BASE, PWRGATE_TIMER_MULT);
    Reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, PWRGATE_TIMER_MULT, MULT, EIGHT, Reg);
    NV_PMC_REGW(PMC_PA_BASE, PWRGATE_TIMER_MULT, Reg);

    return IsChipInitialized;
}

static NvBlCpuClusterId NvBlAvpQueryFlowControllerClusterId(void)
{
    NvBlCpuClusterId    ClusterId = NvBlCpuClusterId_Fast; // Active CPU id
    NvU32               Reg;                            // Scratch register

    Reg = NV_FLOW_REGR(FLOW_PA_BASE, CLUSTER_CONTROL);
    Reg = NV_DRF_VAL(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, Reg);

    if (Reg == NV_DRF_DEF(FLOW_CTLR, CLUSTER_CONTROL, ACTIVE, LP))
    {
        ClusterId = NvBlCpuClusterId_Slow;
    }

    return ClusterId;
}

static NvBool NvBlAvpIsPartitionPowered(NvU32 Mask)
{
    // Get power gate status.
    NvU32   Reg = NV_PMC_REGR(PMC_PA_BASE, PWRGATE_STATUS);

    if ((Reg & Mask) == Mask)
    {
        return NV_TRUE;
    }

    return NV_FALSE;
}

static void NvBlAvpPowerPartition(NvU32 status, NvU32 toggle)
{
    // Is the partition already on?
    if (!NvBlAvpIsPartitionPowered(status))
    {
        // No, toggle the partition power state (OFF -> ON).
        NV_PMC_REGW(PMC_PA_BASE, PWRGATE_TOGGLE, toggle);

        // Wait for the power to come up.
        while (!NvBlAvpIsPartitionPowered(status))
        {
            // Do nothing
        }
    }
}

static void NvBlAvpRemoveCpuIoClamps(void)
{
    NvU32               Reg;        // Scratch reg

    // If booting to the fast cluster ...
    if (NvBlAvpQueryFlowControllerClusterId() == NvBlCpuClusterId_Fast)
    {
        // ... remove the clamps on the fast rail partition I/O signals ...
        Reg = PMC_CLAMPING_CMD(CRAIL);
        NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);

        // ... and the fast non-CPU I/O signals ...
        Reg = PMC_CLAMPING_CMD(C0NC);
        NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);

        // ...and the fast CPU0 I/O signals.
        Reg = PMC_CLAMPING_CMD(CE0);
        NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);
    }
    else
    {
        // Remove the clamps on the slow non-CPU I/O signals ...
        Reg = PMC_CLAMPING_CMD(C1NC);
        NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);

        // ...and the CPU0 I/O signals.
        Reg = PMC_CLAMPING_CMD(CELP);
        NV_PMC_REGW(PMC_PA_BASE, REMOVE_CLAMPING_CMD, Reg);
    }

    // Give I/O signals time to stabilize.
    NvBlAvpStallUs(20);  // !!!FIXME!!! THIS TIME HAS NOT BEEN CHARACTERIZED
}

#define POWER_PARTITION(x)  \
    NvBlAvpPowerPartition(PMC_PWRGATE_STATUS(x), PMC_PWRGATE_TOGGLE(x))

static void  NvBlAvpRamRepair(void)
{
    NvU32   Reg;        // Scratch reg

    // Can only be performed on fast cluster
    NV_ASSERT(NvBlAvpQueryFlowControllerClusterId() == NvBlCpuClusterId_Fast);

    // Request RAM repair
    Reg = NV_DRF_DEF(FLOW_CTLR, RAM_REPAIR, REQ, ENABLE);
    NV_FLOW_REGW(FLOW_PA_BASE, RAM_REPAIR, Reg);

    // Wait for completion
    do
    {
        Reg = NV_FLOW_REGR(FLOW_PA_BASE, RAM_REPAIR);
    } while (!NV_DRF_VAL(FLOW_CTLR, RAM_REPAIR, STS, Reg));
}

static void  NvBlAvpPowerUpCpu(void)
{
    // Are we booting to the fast cluster?
    if (NvBlAvpQueryFlowControllerClusterId() == NvBlCpuClusterId_Fast)
    {
        // TODO: Set CPU Power Good Timer

        // Power up the fast cluster rail partition.
        POWER_PARTITION(CRAIL);

        // Do RAM repair.
        NvBlAvpRamRepair();

        // Power up the fast cluster non-CPU partition.
        POWER_PARTITION(C0NC);

        // Power up the fast cluster CPU0 partition.
        POWER_PARTITION(CE0);
    }
    else
    {
        // Power up the slow cluster non-CPU partition.
        POWER_PARTITION(C1NC);

        // Power up the slow cluster CPU partition.
        POWER_PARTITION(CELP);
    }

    // Remove the I/O clamps from CPU power partition.
    NvBlAvpRemoveCpuIoClamps();
}
#endif	//0

static void NvBlGen1I2cStatusPoll(void)
{
    NvU32 status = 0;
    NvU32 timeout = 0;

#define I2C_POLL_TIMEOUT_MSEC   1000
#define I2C_POLL_STEP_MSEC  1

    while (timeout < I2C_POLL_TIMEOUT_MSEC)
    {
        status = NV_GEN1I2C_REGR(GEN1I2C_PA_BASE, I2C_STATUS);
        if (status)
            NvBlAvpStallMs(I2C_POLL_STEP_MSEC);
        else
            return ;

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

    if (!gen1initialised)
    {
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
//tcw Show board ID
	uart_post('I'); uart_post('D'); uart_post('=');
	NvBlPrintU32(Ret);
	uart_post(0x0d);uart_post(0x0a);
//tcw
    return Ret;
}

#define NV_PWRI2C_REGR(pPwrI2c, reg)        NV_READ32( (((NvUPtr)(pPwrI2c)) + I2C_##reg##_0))
#define NV_PWRI2C_REGW(pPwrI2c, reg, val)   NV_WRITE32((((NvUPtr)(pPwrI2c)) + I2C_##reg##_0), (val))

static void NvBlAvpI2cStatusPoll(void)
{
    NvU32 status = 0;
    NvU32 timeout = 0;
  t11x_WriteDebugString("\nPWRI2C status poll\n");

#define I2C_POLL_TIMEOUT_MSEC   1000
#define I2C_POLL_STEP_MSEC  1

    while (timeout < I2C_POLL_TIMEOUT_MSEC)
    {
	t11x_WriteDebugString("\nPWRI2C status poll:before 1st read\n");
        status = NV_PWRI2C_REGR(PWRI2C_PA_BASE, I2C_STATUS);
	t11x_WriteDebugString("\nPWRI2C status poll:status read OK\n");
        if (status) {
		t11x_WriteDebugString("\nPWRI2C status poll: stalling\n");
            NvBlAvpStallMs(I2C_POLL_STEP_MSEC);
	}
        else
            return ;

        timeout += I2C_POLL_STEP_MSEC;
    }

#undef  I2C_POLL_STEP_MSEC
#undef  I2C_POLL_TIMEOUT_MSEC
}

static void NvBlAvpI2cWrite(NvU32 Addr, NvU32 Data)
{
	t11x_WriteDebugString("\nPWRI2C write\n");
    NV_PWRI2C_REGW(PWRI2C_PA_BASE, I2C_CMD_ADDR0, Addr);
//NV_WRITE32(0x7000D0004, Addr);
	t11x_WriteDebugString("\nPWRI2C write:Addr done\n");
    NV_PWRI2C_REGW(PWRI2C_PA_BASE, I2C_CMD_DATA1, Data);
//NV_WRITE32(0x7000D000c, Data);
	t11x_WriteDebugString("\nPWRI2C write:Data done\n");
    NV_PWRI2C_REGW(PWRI2C_PA_BASE, I2C_CNFG, 0xA02);
//NV_WRITE32(0x7000D0000, 0xA02);
	t11x_WriteDebugString("\nPWRI2C write:CNFG done, going to poll\n");
    NvBlAvpI2cStatusPoll();
}

//------------------------------------------------------------------------------
void NvBlAvpEnableCpuPowerRail(void)
{
    NvU32   Reg;        // Scratch reg
    NvU32 BoardId;

#define DALMORE_E1613_BOARDID 0x64D
#define DALMORE_E1611_BOARDID 0x64B		//tcw
#define PLUTO_E1580_BOARDID   0x62C

	BoardId = NvBlAvpI2cReadBoardId(EEPROM_I2C_SLAVE_ADDRESS);

    /*
     * Program PMIC TPS51632
     *
     * slave address 7b 1000011 (3 LSB selected by SLEWA which is 0.8v on
     * Dalmore E1613.
     *
     * The cold boot voltage is selected by BRAMP  which is 0v on Dalmore,
     * so we need to program VSR to 1v
     */

#define DALMORE_TPS51632_I2C_ADDR   0x86
#define PLUTO_TPS65913_I2C_ADDR     0xB0
#define PLUTO_CPUPWRREQ_EN          1

	/* set pinmux for PWR_I2C SCL and SDA */
	NV_WRITE32(PINMUX_BASE + PINMUX_AUX_PWR_I2C_SCL_0, 0x60);
	NV_WRITE32(PINMUX_BASE + PINMUX_AUX_PWR_I2C_SDA_0, 0x60);

        if (BoardId == DALMORE_E1613_BOARDID) {
	uart_post('E'); uart_post('1'); uart_post('6'); uart_post('1'); uart_post('1');
            NvBlAvpI2cWrite(DALMORE_TPS51632_I2C_ADDR, 0x5400);
	}
        else if (BoardId == PLUTO_E1580_BOARDID)
        {
	uart_post('E'); uart_post('1'); uart_post('5'); uart_post('8'); uart_post('0');
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

	/*
	 * Hack for enabling necessary rails for display, Need to be removed
	 * once PWR I2C driver is fixed for CPU side.
	 */
	if (BoardId == DALMORE_E1611_BOARDID) {
		NvBlAvpI2cWrite(0xB0,  0x0154);	/* LDO3_CTRL */
		NvBlAvpI2cWrite(0xB0,  0x0755);	/* LDO3_VOLTAGE */
						/* ENABLED_MIPI_RAIL */
		NvBlAvpI2cWrite(0x90, 0x030F);	/* VOUT1 (FET1):VDD_LCD_BL_0 */
		NvBlAvpI2cWrite(0x90, 0x0312);	/* VOUT4 (FET4) : AVDD_LCD */
	}

    if (BoardId == DALMORE_E1613_BOARDID || BoardId == PLUTO_E1580_BOARDID)
    {
	uart_post('G'); uart_post('M'); uart_post('I'); uart_post('C'); uart_post('S');uart_post('1');
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
    uart_post('P'); uart_post('W'); uart_post('R'); uart_post('G'); uart_post('D');
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
    * Set CLK_RST_CONTROLLER_CPU_SOFTRST_CTRL2_0_CAR2PMC_CPU_ACK_WIDTH to 0x960
    * to improve power gating/ungating reliablity. This is about 8us when
    * system clock runs at 300MHz.
    */
    Reg = NV_CAR_REGR(CLK_RST_PA_BASE,CPU_SOFTRST_CTRL2);
    Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CPU_SOFTRST_CTRL2, CAR2PMC_CPU_ACK_WIDTH, 0x960, Reg);
    Reg = NV_CAR_REGW(CLK_RST_PA_BASE,CPU_SOFTRST_CTRL2, Reg);
}

#if	0	//tcw
//------------------------------------------------------------------------------
void NvBlStartCpu_T11x(NvU32 ResetVector)
{
    // Enable VDD_CPU
    NvBlAvpEnableCpuPowerRail();

    // Hold the CPUs in reset.
    NvBlAvpResetCpu(NV_TRUE);

    // Disable the CPU clock.
    NvBlAvpEnableCpuClock(NV_FALSE);

    // Enable CoreSight.
    NvBlAvpClockEnableCorsight(NV_TRUE);

    // Set the entry point for CPU execution from reset, if it's a non-zero value.
    if (ResetVector)
    {
        NvBlAvpSetCpuResetVector(ResetVector);
    }

    // Power up the CPU.
    NvBlAvpPowerUpCpu();

    // Enable the CPU clock.
    NvBlAvpEnableCpuClock(NV_TRUE);

    // Take the CPU out of reset.
    NvBlAvpResetCpu(NV_FALSE);

}

//------------------------------------------------------------------------------
NvBool NvBlQueryGetNv3pServerFlag_T11x(void)
{
    // Pointer to the start of IRAM which is where the Boot Information Table
    // will reside (if it exists at all).
    NvBootInfoTable*  pBootInfo = (NvBootInfoTable*)(NV_BIT_ADDRESS);
    NvU32 * SafeStartAddress = 0;
    NvU32 Nv3pSignature = NV3P_SIGNATURE;
    static NvS8 isNv3pSignatureSet = -1;

    if (isNv3pSignatureSet == 0 || isNv3pSignatureSet == 1)
        return isNv3pSignatureSet;

    // Yes, is there a valid BCT?
    if (pBootInfo->BctValid)
    {
        // Is the address valid?
        if (pBootInfo->SafeStartAddr)
        {
            // Get the address where the signature is stored. (Safe start - 4)
            SafeStartAddress =
                (NvU32 *)(pBootInfo->SafeStartAddr - sizeof(NvU32));
            // compare signature...if this is coming from miniloader

            isNv3pSignatureSet = 0;
            if (*(SafeStartAddress) == Nv3pSignature)
            {
                //if yes then modify the safe start address
                //with correct start address (Safe address -4)
                isNv3pSignatureSet = 1;
                (pBootInfo->SafeStartAddr) -= sizeof(NvU32);
                return NV_TRUE;
            }

        }
    }

    // return false on any failure.
    return NV_FALSE;
}

void NvBlConfigHsicPad_T11x(void)
{
    NvU32 Reg;
    Reg = NV_READ32(USB2_PA_BASE + USB2_UHSIC_PADS_CFG0_0);
    Reg = NV_FLD_SET_DRF_NUM(USB2, UHSIC_PADS_CFG0, UHSIC_TX_SLEWP, 0x0, Reg);
    Reg = NV_FLD_SET_DRF_NUM(USB2, UHSIC_PADS_CFG0, UHSIC_TX_SLEWN, 0x0, Reg);
    NV_WRITE32(USB2_PA_BASE + USB2_UHSIC_PADS_CFG0_0, Reg);

    /* FPGA doesn't accomodate USB3 becuase of space constraints */
    /* This check can be removed if FPGA has USB3 */
    if (s_FpgaEmulation != NV_TRUE)
    {
        Reg = NV_READ32(USB3_PA_BASE + USB3_UHSIC_PADS_CFG0_0);
        Reg = NV_FLD_SET_DRF_NUM(USB3, UHSIC_PADS_CFG0, UHSIC_TX_SLEWP, 0x0, Reg);
        Reg = NV_FLD_SET_DRF_NUM(USB3, UHSIC_PADS_CFG0, UHSIC_TX_SLEWN, 0x0, Reg);
        NV_WRITE32(USB3_PA_BASE + USB3_UHSIC_PADS_CFG0_0, Reg);
    }
}

void NvBlConfigJtagAccess_T11x(void)
{
    NvBool SecureJtagControl = NV_FALSE;
    NvU32 reg = 0;

    const NvBootInfoTable*  pBootInfo = (const NvBootInfoTable*)(T11X_BASE_PA_BOOT_INFO);

    // Leave JTAG untouched in NvProd mode
    if (NV_FUSE_REGR(FUSE_PA_BASE, SECURITY_MODE))
    {
        if (NvBlAvpIsChipInitialized() || NvBlAvpIsChipInRecovery())
        {
            SecureJtagControl = pBootInfo->BctPtr->SecureJtagControl;
            if (SecureJtagControl) // JTAG is enabled by BootROM
            {
                reg  = AOS_REGR(SECURE_BOOT_PA_BASE + NVBL_SECURE_BOOT_PFCFG_OFFSET);
                // Maximum level of debuggability is decided by what
                // NVBL_SECURE_BOOT_JTAG_VAL is going to get during build time.
                // It depends on the existence of SecureOs and requirement to
                // enable profiling/debugging.
                reg &= NVBL_SECURE_BOOT_JTAG_CLEAR;
                reg |= NVBL_SECURE_BOOT_JTAG_VAL;
                AOS_REGW(SECURE_BOOT_PA_BASE + NVBL_SECURE_BOOT_PFCFG_OFFSET, reg);

                reg = AOS_REGR(MISC_PA_BASE + APB_MISC_PP_CONFIG_CTL_0);
                reg = NV_FLD_SET_DRF_DEF(APB_MISC,PP_CONFIG_CTL,JTAG,ENABLE,reg);
                AOS_REGW(MISC_PA_BASE + APB_MISC_PP_CONFIG_CTL_0, reg);
            }
            else // JTAG is disabled by BootROM
            {
                // Must write '0' to SPNIDEN, SPIDEN, DBGEN and untouch NIDEN
                reg  = AOS_REGR(SECURE_BOOT_PA_BASE + NVBL_SECURE_BOOT_PFCFG_OFFSET);
                // NOTE : There are 6 event counters in the A15 PMUs (1 per processor)
                // NVBL_JTAG_DISABLE should get 0xFFFFFFF0 if they are not required
                reg &= NVBL_JTAG_DISABLE;
                AOS_REGW(SECURE_BOOT_PA_BASE + NVBL_SECURE_BOOT_PFCFG_OFFSET, reg);

                // Set '1' to JTAG_STS of APBDEV_PMC_STICKY_BITS_0
                reg = AOS_REGR(PMC_PA_BASE + APBDEV_PMC_STICKY_BITS_0);
                reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC,STICKY_BITS,JTAG_STS,DISABLE,reg);
                AOS_REGW(PMC_PA_BASE + APBDEV_PMC_STICKY_BITS_0, reg);
            }
        }
        else
        {
            //Reset the chip (Chip is neither initialized nor in recovery)
            reg = NV_READ32(PMC_PA_BASE + APBDEV_PMC_CNTRL_0);
            reg = NV_FLD_SET_DRF_DEF(APBDEV_PMC, CNTRL, MAIN_RST, ENABLE, reg);
            NV_WRITE32(PMC_PA_BASE + APBDEV_PMC_CNTRL_0, reg);
            while(1);
        }
    }
    return;
}

//------------------------------------------------------------------------------
#if !__GNUC__
__asm void NvBlStartUpAvp_T11x( void )
{
    CODE32
    PRESERVE8

    IMPORT NvBlAvpInit_T11x
    IMPORT NvBlStartCpu_T11x
    IMPORT NvBlAvpHalt

    //------------------------------------------------------------------
    // Initialize the AVP, clocks, and memory controller.
    //------------------------------------------------------------------

    // The SDRAM is guaranteed to be on at this point
    // in the nvml environment. Set r0 = 1.
    MOV     r0, #1
    BL      NvBlAvpInit_T11x

    //------------------------------------------------------------------
    // Start the CPU.
    //------------------------------------------------------------------

    LDR     r0, =ColdBoot_T11x               //; R0 = reset vector for CPU
    BL      NvBlStartCpu_T11x

    //------------------------------------------------------------------
    // Transfer control to the AVP code.
    //------------------------------------------------------------------

    BL      NvBlAvpHalt

    //------------------------------------------------------------------
    // Should never get here.
    //------------------------------------------------------------------
    B       .
}

// we're hard coding the entry point for all AOS images

// this function inits the MPCORE and then jumps to the to the MPCORE
// executable at NV_AOS_ENTRY_POINT
__asm void ColdBoot_T11x( void )
{
    CODE32
    PRESERVE8

    MSR     CPSR_c, #PSR_MODE_SVC _OR_ PSR_I_BIT _OR_ PSR_F_BIT

    //------------------------------------------------------------------
    // Check current processor: CPU or AVP?
    // If AVP, go to AVP boot code, else continue on.
    //------------------------------------------------------------------

    LDR     r0, =PG_UP_PA_BASE
    LDRB    r2, [r0, #PG_UP_TAG_0]
    CMP     r2, #PG_UP_TAG_0_PID_CPU _AND_ 0xFF //; are we the CPU?
    LDR     sp, =NVAP_LIMIT_PA_IRAM_CPU_EARLY_BOOT_STACK
    // leave in some symbols for release debugging
    LDR     r3, =0xDEADBEEF
    STR     r3, [sp, #-4]!
    STR     r3, [sp, #-4]!
#ifdef BUILD_FOR_COSIM
    //always jump to etry point. AVP is not modeled in COSIM
    B       NV_AOS_ENTRY_POINT                   //; yep, we are the CPU
#else
    BEQ     NV_AOS_ENTRY_POINT                   //; yep, we are the CPU
#endif

    //==================================================================
    // AVP Initialization follows this path
    //==================================================================

    LDR     sp, =NVAP_LIMIT_PA_IRAM_AVP_EARLY_BOOT_STACK
    // leave in some symbols for release debugging
    LDR     r3, =0xDEADBEEF
    STR     r3, [sp, #-4]!
    STR     r3, [sp, #-4]!
    B       NvBlStartUpAvp_T11x
}
#endif

#endif	//0
