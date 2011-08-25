/**
 * Copyright (c) 2010-2011 NVIDIA Corporation.  All Rights Reserved.
 *
.* NVIDIA Corporation and its licensors retain all intellectual property and
.* proprietary rights in and to this software and related documentation.  Any
.* use, reproduction, disclosure or distribution of this software and related
.* documentation without an express license agreement from NVIDIA Corporation
.* is strictly prohibited.
*/

#include <common.h>
#include <asm/arch/tegra.h>
#include <asm/arch/t30/nvcommon.h>
#include <asm/arch/t30/nverror.h>
#include "t30.h"
#include "t30_uart.h"

#include <asm/arch/t30/arapb_misc.h>
#include <asm/arch/t30/aruart.h>

#define NV_ASSERT(value) \
{ \
	if ((value) == 0) { \
		t30_WriteDebugString("collllld\n"); \
	} \
}

#define NV_ADDRESS_MAP_CAR_BASE	CLK_RST_PA_BASE
  
static NvU32 s_UartBaseAddress[] =
{
    0x70006000,
    0x70006040,
    0x70006200,
    0x70006300,
    0x70006400
};
enum UartPorts
{
    UARTA,
    UARTB,
    UARTC,
    UARTD,
    UARTE
};

#define TRISTATE_UART(reg) \
        TristateReg = NV_READ32(MISC_PA_BASE + PINMUX_AUX_##reg##_0); \
        TristateReg = NV_FLD_SET_DRF_DEF(PINMUX, AUX_##reg, \
        TRISTATE, NORMAL, TristateReg); \
        NV_WRITE32(MISC_PA_BASE + PINMUX_AUX_##reg##_0, TristateReg);


#define PINMUX_UART(reg, uart) \
        PinMuxReg = NV_READ32(MISC_PA_BASE + PINMUX_AUX_##reg##_0); \
        PinMuxReg = NV_FLD_SET_DRF_DEF(PINMUX, AUX_##reg, PM, \
            UART##uart, PinMuxReg); \
        NV_WRITE32(MISC_PA_BASE + PINMUX_AUX_##reg##_0, PinMuxReg);

#if 0 /* not in use, jz */
void aos_EnableUartT30(NvU32 port);

/**
 * Defines the UART pin-mux configurations.
 */
typedef enum
{
    NvOdmUartPinMap_Config1 = 1,
    NvOdmUartPinMap_Config2,
    NvOdmUartPinMap_Config3,
    NvOdmUartPinMap_Config4,
    NvOdmUartPinMap_Config5,
    NvOdmUartPinMap_Config6,
    NvOdmUartPinMap_Config7,

    /** Ignore -- Forces compilers to make 32-bit enums. */
    NvOdmUartPinMap_Force32 = 0x7FFFFFFF
} NvOdmUartPinMap;

static NvError aos_EnableUartA(NvU32 configuration)
{
    NvError retval = NvSuccess;
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (configuration)
    {
        case NvOdmUartPinMap_Config1:
            TRISTATE_UART(UART2_RTS_N);
            TRISTATE_UART(UART2_CTS_N);
            TRISTATE_UART(ULPI_DATA0);
            TRISTATE_UART(ULPI_DATA1);
            TRISTATE_UART(ULPI_DATA2);
            TRISTATE_UART(ULPI_DATA3);
            TRISTATE_UART(ULPI_DATA4);
            TRISTATE_UART(ULPI_DATA5);
            TRISTATE_UART(ULPI_DATA6);
            TRISTATE_UART(ULPI_DATA7);
            PINMUX_UART(UART2_RTS_N,A);
            PINMUX_UART(UART2_CTS_N,A);
            PINMUX_UART(ULPI_DATA0,A);
            PINMUX_UART(ULPI_DATA1,A);
            PINMUX_UART(ULPI_DATA2,A);
            PINMUX_UART(ULPI_DATA3,A);
            PINMUX_UART(ULPI_DATA4,A);
            PINMUX_UART(ULPI_DATA5,A);
            PINMUX_UART(ULPI_DATA6,A);
            PINMUX_UART(ULPI_DATA7,A);
            break;
        case NvOdmUartPinMap_Config2:
            TRISTATE_UART(SDMMC1_DAT0);
            TRISTATE_UART(SDMMC1_DAT1);
            TRISTATE_UART(SDMMC1_DAT2);
            TRISTATE_UART(SDMMC1_DAT3);
            TRISTATE_UART(SDMMC1_CMD);
            TRISTATE_UART(SDMMC1_CLK);
            TRISTATE_UART(GPIO_PU6);
            PINMUX_UART(SDMMC1_DAT0,A);
            PINMUX_UART(SDMMC1_DAT1,A);
            PINMUX_UART(SDMMC1_DAT2,A);
            PINMUX_UART(SDMMC1_DAT3,A);
            PINMUX_UART(SDMMC1_CMD,A);
            PINMUX_UART(SDMMC1_CLK,A);
            PINMUX_UART(GPIO_PU6,A);
            break;
        case NvOdmUartPinMap_Config3:
            TRISTATE_UART(UART2_RTS_N);
            TRISTATE_UART(UART2_CTS_N);
            TRISTATE_UART(UART2_RXD);
            TRISTATE_UART(UART2_TXD);
            TRISTATE_UART(GPIO_PU5);
            TRISTATE_UART(GPIO_PU4);
            PINMUX_UART(UART2_RTS_N,A);
            PINMUX_UART(UART2_CTS_N,A);
            PINMUX_UART(UART2_RXD,A);
            PINMUX_UART(UART2_TXD,A);
            PINMUX_UART(GPIO_PU5,A);
            PINMUX_UART(GPIO_PU4,A);
            break;
        case NvOdmUartPinMap_Config4:
            TRISTATE_UART(GPIO_PU0);
            TRISTATE_UART(GPIO_PU1);
            TRISTATE_UART(GPIO_PU2);
            TRISTATE_UART(GPIO_PU3);
            PINMUX_UART(GPIO_PU0,A);
            PINMUX_UART(GPIO_PU1,A);
            PINMUX_UART(GPIO_PU2,A);
            PINMUX_UART(GPIO_PU3,A);
            break;
        case NvOdmUartPinMap_Config5:
            TRISTATE_UART(SDMMC3_CMD);
            TRISTATE_UART(SDMMC3_CLK);
            PINMUX_UART(SDMMC3_CMD,A);
            PINMUX_UART(SDMMC3_CLK,A);
            break;
        default:
            retval = NvError_BadParameter;
            break;
    }
    return retval;
}

static NvError aos_EnableUartB(NvU32 configuration)
{
    NvError retval = NvSuccess;
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (configuration)
    {
        case NvOdmUartPinMap_Config1:
            TRISTATE_UART(UART2_CTS_N);
            TRISTATE_UART(UART2_RTS_N);
            TRISTATE_UART(UART2_RXD);
            TRISTATE_UART(UART2_TXD);
            PINMUX_UART(UART2_CTS_N,B);
            PINMUX_UART(UART2_RTS_N,B);
            PINMUX_UART(UART2_RXD,A);
            PINMUX_UART(UART2_TXD,A);
            break;
        default:
            retval = NvError_BadParameter;
        break;
    }
    return retval;
}

static NvError aos_EnableUartC(NvU32 configuration)
{
    NvError retval = NvSuccess;
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (configuration)
    {
        case NvOdmUartPinMap_Config1:
            TRISTATE_UART(UART3_CTS_N);
            TRISTATE_UART(UART3_RTS_N);
            TRISTATE_UART(UART3_TXD);
            TRISTATE_UART(UART3_RXD);
            PINMUX_UART(UART3_CTS_N,C);
            PINMUX_UART(UART3_RTS_N,C);
            PINMUX_UART(UART3_TXD,C);
            PINMUX_UART(UART3_RXD,C);
            break;
        default:
            retval = NvError_BadParameter;
            break;
   }
   return retval;
}

static NvError aos_EnableUartD(NvU32 configuration)
{
    NvError retval = NvSuccess;
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (configuration)
    {
        case NvOdmUartPinMap_Config1:
            TRISTATE_UART(ULPI_CLK);
            TRISTATE_UART(ULPI_DIR);
            TRISTATE_UART(ULPI_STP);
            TRISTATE_UART(ULPI_NXT);
            PINMUX_UART(ULPI_CLK,D);
            PINMUX_UART(ULPI_DIR,D);
            PINMUX_UART(ULPI_STP,D);
            PINMUX_UART(ULPI_NXT,D);
            break;
        case NvOdmUartPinMap_Config2:
            TRISTATE_UART(GMI_A16);
            TRISTATE_UART(GMI_A17);
            TRISTATE_UART(GMI_A18);
            TRISTATE_UART(GMI_A19);
            PINMUX_UART(GMI_A16,D);
            PINMUX_UART(GMI_A17,D);
            PINMUX_UART(GMI_A18,D);
            PINMUX_UART(GMI_A19,D);
            break;
        default:
            retval = NvError_BadParameter;
            break;
    }
    return retval;
}
static NvError aos_EnableUartE(NvU32 configuration)
{
    NvError retval = NvSuccess;
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (configuration)
    {
        case NvOdmUartPinMap_Config1:
            TRISTATE_UART(SDMMC1_DAT0);
            TRISTATE_UART(SDMMC1_DAT1);
            TRISTATE_UART(SDMMC1_DAT2);
            TRISTATE_UART(SDMMC1_DAT3);
            PINMUX_UART(SDMMC1_DAT0,E);
            PINMUX_UART(SDMMC1_DAT1,E);
            PINMUX_UART(SDMMC1_DAT2,E);
            PINMUX_UART(SDMMC1_DAT3,E);
            break;
        case NvOdmUartPinMap_Config2:
            TRISTATE_UART(SDMMC1_DAT0);
            TRISTATE_UART(SDMMC1_DAT1);
            TRISTATE_UART(SDMMC1_DAT2);
            TRISTATE_UART(SDMMC1_DAT3);
            PINMUX_UART(SDMMC1_DAT0,E);
            PINMUX_UART(SDMMC1_DAT1,E);
            PINMUX_UART(SDMMC1_DAT2,E);
            PINMUX_UART(SDMMC1_DAT3,E);
            break;
        default:
            retval = NvError_BadParameter;
            break;
    }
    return retval;
}

static void aos_CpuStallUsT30(NvU32 MicroSec);

void aos_EnableUartT30(NvU32 port)
{
//    NvU32 *pPinMuxConfigTable = NULL;
//    NvU32 Count = 0;
    NvU32 configuration;
#if 1  // jz
    configuration = NvOdmUartPinMap_Config1;
#else
    NvOdmQueryPinMux(NvOdmIoModule_Uart,
           (const NvU32 **)&pPinMuxConfigTable,
                      &Count);
    configuration = pPinMuxConfigTable[port];
#endif
    switch (port)
    {
        case UARTA:
            aos_EnableUartA(configuration);
            break;
        case UARTB:
            NV_ASSERT(0);
//            aos_EnableUartB(configuration);
            break;
        case UARTC:
            NV_ASSERT(0);
//            aos_EnableUartC(configuration);
            break;
        case UARTD:
            aos_EnableUartD(configuration);
            break;
        case UARTE:
            NV_ASSERT(0);
//            aos_EnableUartE(configuration);
            break;
        default:
            NV_ASSERT(0);
    }
}

static void aos_UartClockEnableT30(NvU32 port, NvBool Enable)
{
    NvU32 Reg;

    switch (port)
    {
        case UARTA:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_L);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_L,
                                 CLK_ENB_UARTA, Enable, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_L, Reg);
            break;
        case UARTB:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_L);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_L,
                                 CLK_ENB_UARTB, Enable, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_L, Reg);
            break;
        case UARTC:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_H);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_H,
                                 CLK_ENB_UARTC, Enable, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_H, Reg);
            break;
        case UARTD:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_U);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_U,
                                 CLK_ENB_UARTD, Enable, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_U, Reg);
            break;
        case UARTE:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_U);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_U,
                                 CLK_ENB_UARTE, Enable, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_U, Reg);
            break;
        default:
            NV_ASSERT(0);
    }
}

static void aos_UartResetT30(NvU32 port, NvBool Assert)
{
    NvU32 Reg;

    switch (port)
    {
        case UARTA:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_L);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_L,
                                 SWR_UARTA_RST, Assert, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_L, Reg);
            break;
        case UARTB:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_L);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_L,
                                 SWR_UARTB_RST, Assert, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_L, Reg);
            break;
        case UARTC:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_H);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_H,
                                 SWR_UARTC_RST, Assert, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_H, Reg);
            break;
        case UARTD:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_U);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_U,
                                 SWR_UARTD_RST, Assert, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_U, Reg);
            break;
        case UARTE:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_U);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_U,
                                 SWR_UARTE_RST, Assert, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_U, Reg);
            break;
        default:
            NV_ASSERT(0);
    }
}

static void aos_SetUartClockSourceT30( NvU32 port )
{
    NvU32 Reg;
    switch (port)
    {
        case UARTA:
            Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_UARTA,
                             UARTA_CLK_SRC, PLLP_OUT0);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_UARTA, Reg);
            break;
        case UARTB:
            Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_UARTB,
                             UARTB_CLK_SRC, PLLP_OUT0);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_UARTB, Reg);
            break;
        case UARTC:
            Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_UARTC,
                             UARTC_CLK_SRC, PLLP_OUT0);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_UARTC, Reg);
            break;
        case UARTD:
            Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_UARTD,
                             UARTD_CLK_SRC, PLLP_OUT0);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_UARTD, Reg);
            break;
        case UARTE:
            Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_UARTE,
                             UARTE_CLK_SRC, PLLP_OUT0);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_UARTE, Reg);
            break;
        default:
            NV_ASSERT(0);
    }
}

static void aos_UartResetPortT30(NvU32 portNumber)
{
    aos_UartClockEnableT30(portNumber, NV_TRUE);
    aos_UartResetT30(portNumber, NV_TRUE);
    aos_CpuStallUsT30(10);
    aos_UartResetT30(portNumber, NV_FALSE);
}

static NvError aos_UartSetBaudRateT30(NvU32 portNumber, NvU32 baud_rate)
{
    NvU32   BaudRateDivisor;
    NvU32   LineControlReg;
    volatile void* pUartRegs = 0;
    pUartRegs = (void*)s_UartBaseAddress[portNumber];
    // Compute the baudrate divisor
    // Prepare the divisor value.

#if 0 // jz
    if (GetMajorVersion() == 0)
        BaudRateDivisor = (13000 * 1000) / (baud_rate *16); //for emulation
    else
#endif
        BaudRateDivisor = (AOS_PLLP_FIXED_FREQ_KHZ * 1000) / (baud_rate *16);

    // Set the divisor latch bit to allow access to divisor registers (DLL/DLM)
    LineControlReg = NV_UART_REGR(pUartRegs, LCR);
    LineControlReg |= NV_DRF_NUM(UART, LCR, DLAB, 1);
    NV_UART_REGW(pUartRegs, LCR, LineControlReg);

    // First write the LSB of the baud rate divisor
    NV_UART_REGW(pUartRegs, THR_DLAB_0, BaudRateDivisor & 0xFF);

    // Now write the MSB of the baud rate divisor
    NV_UART_REGW(pUartRegs, IER_DLAB_0, (BaudRateDivisor >> 8) & 0xFF);

    // Now that the baud rate has been set turn off divisor latch
    // bit to allow access to receive/transmit holding registers
    // and interrupt enable register.
    LineControlReg &= ~NV_DRF_NUM(UART, LCR, DLAB, 1);
    NV_UART_REGW(pUartRegs, LCR, LineControlReg);

    return NvError_Success;
}

volatile void* aos_UartInitT30(NvU32 portNumber, NvU32 baud_rate)
{
    volatile void* pUart;
    NvU32   LineControlReg;
    NvU32   FifoControlReg;
    NvU32   ModemControlReg;
    pUart = (void*)s_UartBaseAddress[portNumber];
    aos_EnableUartT30(portNumber);
    aos_UartResetPortT30(portNumber);
    // Select the clock source for the UART.
    aos_SetUartClockSourceT30(portNumber);
    // Initialize line control register: no parity, 1 stop bit, 8 data bits.
    LineControlReg = NV_DRF_NUM(UART, LCR, PAR, 0)
         | NV_DRF_NUM(UART, LCR, STOP, 0)
         | NV_DRF_NUM(UART, LCR, WD_SIZE, 3);
    NV_UART_REGW(pUart, LCR, LineControlReg);

    // Setup baud rate
    if (aos_UartSetBaudRateT30(portNumber, baud_rate) != NvError_Success)
    {
        return 0;
    }
    // Clear and enable the transmit and receive FIFOs.
    FifoControlReg = NV_DRF_NUM(UART, IIR_FCR, TX_CLR, 1)
         | NV_DRF_NUM(UART, IIR_FCR, RX_CLR, 1)
         | NV_DRF_NUM(UART, IIR_FCR, FCR_EN_FIFO, 1);
    NV_UART_REGW(pUart, IIR_FCR, FifoControlReg);

    // Assert DTR (Data Terminal Ready) so that the other side knows we're ready.
    ModemControlReg  = NV_UART_REGR(pUart, MCR);
    ModemControlReg |= NV_DRF_NUM(UART, MCR, DTR, 1);
    NV_UART_REGW(pUart, MCR, ModemControlReg);
    return pUart;
}
#endif /* not in use, jz */


static void aos_CpuStallUsT30(NvU32 MicroSec)
{
    NvU32 Reg;
    NvU32 Delay;
    NvU32 MaxUs;

    // Get the maxium delay per loop.
    MaxUs = NV_DRF_NUM(FLOW_CTLR, HALT_CPU_EVENTS, ZERO, 0xFFFFFFFF);
    MicroSec += 1;

    while (MicroSec)
    {
        Delay   = (MicroSec > MaxUs) ? MaxUs : MicroSec;
        NV_ASSERT(Delay != 0);
        NV_ASSERT(Delay <= MicroSec);
        MicroSec -= Delay;

        Reg = NV_DRF_NUM(FLOW_CTLR, HALT_CPU_EVENTS, ZERO, Delay)
         | NV_DRF_NUM(FLOW_CTLR, HALT_CPU_EVENTS, uSEC, 1)
         | NV_DRF_DEF(FLOW_CTLR, HALT_CPU_EVENTS, MODE, FLOW_MODE_WAITEVENT);
        NV_FLOW_REGW(FLOW_PA_BASE, HALT_CPU_EVENTS, Reg);
        do
        {
             Reg = NV_FLOW_REGR(FLOW_PA_BASE, HALT_CPU_EVENTS);
             Reg = NV_DRF_VAL(FLOW_CTLR, HALT_CPU_EVENTS, ZERO, Reg);
        }while (Reg);
    }
}

static void aos_UartWriteByteT30(volatile void* pUart, NvU8 ch)
{
    NvU32 LineStatusReg;
    // Wait for transmit buffer to be empty.
    do
    {
        LineStatusReg = NV_UART_REGR(pUart, LSR);
    } while (!(LineStatusReg & NV_DRF_NUM(UART, LSR, THRE, 1)));
    // Write the character.
    NV_UART_REGW(pUart, THR_DLAB_0, ch);
}


static NvU8 aos_UartReadByteT30(volatile void* pUart)
{
    NvU8 ch;
    NvU32 LineStatusReg;
    // Wait for some time to get the data
    do
    {
        LineStatusReg = NV_UART_REGR(pUart, LSR);
    } while (!(LineStatusReg & NV_DRF_NUM(UART, LSR, RDR, 1)));
//    } while (NV_DRF_VAL(UART, LSR, RDR, LineStatusReg) != UART_LSR_0_RDR_DATA_IN_FIFO);

    // Read the character.
//    ch = NV_UART_REGR(pUart, RBR);
    ch = NV_UART_REGR(pUart, THR_DLAB_0);
    return ch;
}

/////////////////////////////////////////////////////////
//
// code added below for t30 bringup
//
// special uart A init before pllp is ready
//
/////////////////////////////////////////////////////////
#define NV_ADDRESS_MAP_PPSB_CLK_RST_BASE 0x60006000
#define NV_ADDRESS_MAP_MISC_BASE 	 0x70000000
#if 0
#define NV_UARTA_LCR			(NV_ADDRESS_MAP_APB_UARTA_BASE + 0x0c)
#define NV_UARTA_THR			(NV_ADDRESS_MAP_APB_UARTA_BASE + 0x00)
#define NV_UARTA_IER			(NV_ADDRESS_MAP_APB_UARTA_BASE + 0x04)
#define NV_UARTA_FCR			(NV_ADDRESS_MAP_APB_UARTA_BASE + 0x08)
#define NV_UARTA_LSR			(NV_ADDRESS_MAP_APB_UARTA_BASE + 0x14)
#endif // jz, 6/27/11

#define UART1_BASE    0x70006000
#define UART2_BASE    0x70006040
#define UART3_BASE    0x70006200
#define UART4_BASE    0x70006300
#define UART5_BASE    0x70006400
#define UART_LEN      0x40
#define UART1_INT_ID  0x44
#define UART2_INT_ID  0x45
#define UART3_INT_ID  0x4E
#define UART4_INT_ID  0x7A
#define UART5_INT_ID  0x7B

#define UART_DLL_REG  (0x0000)
#define UART_RBR_REG  (0x0000)
#define UART_THR_REG  (0x0000)
#define UART_DLH_REG  (0x0004)
#define UART_FCR_REG  (0x0008)
#define UART_LCR_REG  (0x000C)
#define UART_MCR_REG  (0x0010)
#define UART_LSR_REG  (0x0014)

#define BIT0  0x00000001
#define BIT1  0x00000002
#define BIT2  0x00000004
#define BIT3  0x00000008
#define BIT4  0x00000010
#define BIT5  0x00000020
#define BIT6  0x00000040
#define BIT7  0x00000080
#define BIT8  0x00000100

#define UART_FCR_TX_FIFO_CLEAR          BIT2
#define UART_FCR_RX_FIFO_CLEAR          BIT1
#define UART_FCR_FIFO_ENABLE            BIT0

#define UART_LCR_DIV_EN_ENABLE          BIT7
#define UART_LCR_DIV_EN_DISABLE         (0UL << 7)
#define UART_LCR_CHAR_LENGTH_8          (BIT1 | BIT0)
#define UART_LCR_EVEN_PARITY            (BIT3 | BIT4)
#define UART_LCR_ODD_PARITY             BIT3
#define UART_LCR_NO_PARITY              0
#define UART_LCR_ONE_STOP_BIT           0
#define UART_LCR_TWO_STOP_BIT           BIT2

#define UART_MCR_RTS_FORCE_ACTIVE       BIT1
#define UART_MCR_DTR_FORCE_ACTIVE       BIT0
#define UART_MCR_FLOW_CONTROL           BIT5 | BIT6

#define UART_LSR_TX_FIFO_E_MASK         BIT5
#define UART_LSR_TX_FIFO_E_NOT_EMPTY    (0UL << 5)
#define UART_LSR_TX_FIFO_E_EMPTY        BIT5
#define UART_LSR_RX_FIFO_E_MASK         BIT0
#define UART_LSR_RX_FIFO_E_NOT_EMPTY    BIT0
#define UART_LSR_RX_FIFO_E_EMPTY        (0UL << 0)


#define MmioWrite32	NV_WRITE32
#define MmioRead32	NV_READ32

/* cardhu */
#if defined(CONFIG_TEGRA3_CARDHU)
#define UartBaseAddress	UART1_BASE
#endif

/* oregon */
#if defined(CONFIG_TEGRA3_OREGON)
#define UartBaseAddress	UART3_BASE
#endif

/* waluigi */
#if defined(CONFIG_TEGRA3_WALUIGI)
#define UartBaseAddress	UART4_BASE
#endif

#define CLK_M		12000000

// Macro that simplifies pin settings.
#define SET_PIN(pin, f, v)                                                   \
    do {                                                                     \
       NvU32 RegData;                                                        \
       RegData = NV_READ32(NV_ADDRESS_MAP_MISC_BASE + PINMUX_AUX_##pin##_0); \
       RegData = NV_FLD_SET_DRF_DEF(PINMUX_AUX, pin, f, v, RegData);         \
       NV_WRITE32(NV_ADDRESS_MAP_MISC_BASE + PINMUX_AUX_##pin##_0, RegData); \
    } while (0);

static NV_INLINE NvU32 NvBlUartTxReady(NvU8 const * uart_base)
{
    return NV_READ8(uart_base + UART_LSR_0) & UART_LSR_0_THRE_FIELD;
}

NvU32 NvBlUartRxReady(NvU8 const * uart_base)
{
    return NV_READ8(uart_base + UART_LSR_0) & UART_LSR_0_RDR_FIELD;
}

static NV_INLINE void NvBlUartTx(NvU8 * uart_base, NvU8 c)
{
    NV_WRITE08(uart_base + UART_THR_DLAB_0_0, c);
}

NvU32 NvBlUartRx(NvU8 const * uart_base)
{
    return NV_READ8(uart_base + UART_THR_DLAB_0_0);
}

#if defined(CONFIG_SYS_PLLP_BASE_IS_408MHZ)
#define NVRM_PLLP_FIXED_FREQ_KHZ	(408000)
#else
#define NVRM_PLLP_FIXED_FREQ_KHZ	(216000)
#endif
#define NV_DEFAULT_DEBUG_BAUD		115200

void NvBlUartInitBase(NvU8 * uart_base)
{
    NvU32 divisor = (NVRM_PLLP_FIXED_FREQ_KHZ * 1000 /
		     NV_DEFAULT_DEBUG_BAUD / 16);

    // Set up UART parameters.
    NV_WRITE08(uart_base + UART_LCR_0,        0x80);
    NV_WRITE08(uart_base + UART_THR_DLAB_0_0, divisor);
    NV_WRITE08(uart_base + UART_IER_DLAB_0_0, 0x00);
    NV_WRITE08(uart_base + UART_LCR_0,        0x00);
    NV_WRITE08(uart_base + UART_IIR_FCR_0,    0x37);
    NV_WRITE08(uart_base + UART_IER_DLAB_0_0, 0x00);
    NV_WRITE08(uart_base + UART_LCR_0,        0x03); /* 8N1 */
    NV_WRITE08(uart_base + UART_MCR_0,        0x02);
    NV_WRITE08(uart_base + UART_MSR_0,        0x00);
    NV_WRITE08(uart_base + UART_SPR_0,        0x00);
    NV_WRITE08(uart_base + UART_IRDA_CSR_0,   0x00);
    NV_WRITE08(uart_base + UART_ASR_0,        0x00);

    NV_WRITE08(uart_base + UART_IIR_FCR_0,    0x31);

    // Flush any old characters out of the RX FIFO.
    while (NvBlUartRxReady(uart_base))
        (void)NvBlUartRx(uart_base);
}

void t30_Uart_Init_h(void)
{
    volatile void* pUart;
/* cardhu */
#if defined(CONFIG_TEGRA3_CARDHU)
    pUart = (void*)s_UartBaseAddress[0];
#endif

/* oregon */
#if defined(CONFIG_TEGRA3_OREGON)
    pUart = (void*)s_UartBaseAddress[2];
#endif

/* waluigi */
#if defined(CONFIG_TEGRA3_WALUIGI)
    pUart = (void*)s_UartBaseAddress[3];
#endif

	NvBlUartInitBase((NvU8 *)pUart);
}

void t30_UartA_Init(void)
{
    NvU32 RegData;

    // Initialization is responsible for correct pin mux configuration
    // It also needs to wait for the correct osc frequency to be known
    // IMPORTANT, there is some unspecified logic in the UART that always 
    // operate off PLLP_out3, mail from Robert Quan says that correct operation
    // of UART without starting PLLP requires
    // - to put PLLP in bypass
    // - to override the PLLP_ou3 divider to be 1 (or put in bypass)
    // This is done like that here to avoid any dependence on PLLP analog circuitry 
    // operating correctly

    RegData  = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_PLLP_BASE_0);
    RegData |= NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_PLLP_BASE_0, RegData);
    RegData  = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_PLLP_OUTB_0);
    RegData |= NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB,
        PLLP_OUT3_OVRRIDE, ENABLE);
    RegData |= NV_DRF_NUM(CLK_RST_CONTROLLER,
        PLLP_OUTB, PLLP_OUT3_RATIO, 0);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_PLLP_OUTB_0, RegData);

    // Set up the pinmuxes to bring UARTA out on ULPI_DATA0 and ULPI_DATA1
    // for T30.  All alternate mappings of UARTA are set to select an input other than UARTA.
    //
    //     UART2_RTS_N => Alternate 3 ( SPI4 instead of UA3_TXD)
    //     UART2_CTS_N => Alternate 3 ( SPI4 instead of UA3_TXD)
    //     ULPI_DATA0  =>Alternate 2 (UA3_TXD)
    //     ULPI_DATA1  =>Alternate 2 UA3_RXD)
    //     SDMMC1_DAT3 => Primary     (SDMMC1_DAT3 instead of UA3_TXD)
    //     SDMMC1_DAT2 => Primary     (SDMMC1_DAT2 instead of UA3_RXD)
    //     GPIO_PU0    => Alternate 2 (GMI_A6 instead of UA3_TXD)
    //     GPIO_PU1    => Alternate 2 (GMI_A7 instead of UA3_RXD)
    //     SDMMC3_CLK  => Alternate 2 (SDMMC3_SCLK instead of UA3_TXD)
    //     SDMMC3_CMD  => Alternate 2 (SDMMC3_CMD instead of UA3_RXD)
    //
    // Last reviewed on 08/27/2010
    SET_PIN(UART2_RTS_N,PM,SPI4) ;
    SET_PIN(UART2_CTS_N,PM,SPI4) ;
    SET_PIN(ULPI_DATA0,PM,UARTA) ;
    SET_PIN(ULPI_DATA1,PM,UARTA) ;
    SET_PIN(SDMMC1_DAT3,PM,SDMMC1) ;
    SET_PIN(SDMMC1_DAT2,PM,SDMMC1) ;
    SET_PIN(GPIO_PU0,PM,GMI) ;
    SET_PIN(GPIO_PU1,PM,GMI) ;
    SET_PIN(SDMMC3_CLK,  PM, SDMMC3);
    SET_PIN(SDMMC3_CMD,  PM, SDMMC3);
    // Enable the pads.
    SET_PIN(ULPI_DATA0, TRISTATE, NORMAL);
    SET_PIN(ULPI_DATA1, TRISTATE, NORMAL);

    // enable UART A clock, toggle reset  
    RegData = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_CLK_OUT_ENB_L_0);
    RegData |= NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_L, 
        CLK_ENB_UARTA, ENABLE);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
                       CLK_RST_CONTROLLER_CLK_OUT_ENB_L_0, RegData);
    RegData  = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_RST_DEVICES_L_0);
    RegData  = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L,
        SWR_UARTA_RST, ENABLE, RegData);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_RST_DEVICES_L_0, RegData);
    RegData  = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_RST_DEVICES_L_0);
    RegData  = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_L,
        SWR_UARTA_RST, DISABLE, RegData);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_RST_DEVICES_L_0, RegData);

    // Then there is the specific set up for UART itself, including the clock configuration.
    // configure at top for source & configure the divider, internal to uart for obscure reasons
    // when UARTA_DIV_ENB is enable DLL/DLLM programming is not required
    //.Refer to the CLK_SOURCE_UARTA reg description in arclk_rst
    // UARTA_DIV_ENB is disabled as new divisor logic is not enabled on FPGA Bug #739606 
    NV_WRITE32(NV_ADDRESS_MAP_CAR_BASE+
        CLK_RST_CONTROLLER_CLK_SOURCE_UARTA_0 ,
        (CLK_RST_CONTROLLER_CLK_SOURCE_UARTA_0_UARTA_CLK_SRC_CLK_M <<
        CLK_RST_CONTROLLER_CLK_SOURCE_UARTA_0_UARTA_CLK_SRC_SHIFT) |
        (CLK_RST_CONTROLLER_CLK_SOURCE_UARTA_0_UARTA_DIV_ENB_DISABLE <<
        CLK_RST_CONTROLLER_CLK_SOURCE_UARTA_0_UARTA_DIV_ENB_SHIFT) |
        0);

}

void t30_UartC_Init(void)
{
    NvU32 RegData;

    // Initialization is responsible for correct pin mux configuration
    // It also needs to wait for the correct osc frequency to be known
    // IMPORTANT, there is some unspecified logic in the UART that always 
    // operate off PLLP_out3, mail from Robert Quan says that correct operation
    // of UART without starting PLLP requires
    // - to put PLLP in bypass
    // - to override the PLLP_ou3 divider to be 1 (or put in bypass)
    // This is done like that here to avoid any dependence on PLLP analog circuitry 
    // operating correctly

    RegData  = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_PLLP_BASE_0);
    RegData |= NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_PLLP_BASE_0, RegData);
    RegData  = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_PLLP_OUTB_0);
    RegData |= NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB,
        PLLP_OUT3_OVRRIDE, ENABLE);
    RegData |= NV_DRF_NUM(CLK_RST_CONTROLLER,
        PLLP_OUTB, PLLP_OUT3_RATIO, 0);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_PLLP_OUTB_0, RegData);

    // Set up the pinmuxes to bring UARTC out on uart3_txd and uart_rxd
    // for oregon T30.
    //
    //     UART3_TXD  =>primary 0 (UC3_TXD)
    //     UART3_RXD  =>primary 0(UC3_RXD)
    //
    SET_PIN(UART3_TXD,PM,UARTC) ;
    SET_PIN(UART3_RXD,PM,UARTC) ;
//    SET_PIN(UART3_CTS_N,PM,UARTC) ;
//    SET_PIN(UART3_RTS_N,PM,UARTC) ;

    // Enable the pads.
    SET_PIN(UART3_TXD, TRISTATE, NORMAL);
    SET_PIN(UART3_RXD, TRISTATE, NORMAL);

    // enable UART C clock, toggle reset  
    RegData = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_CLK_OUT_ENB_H_0);
    RegData |= NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_H, 
        CLK_ENB_UARTC, ENABLE);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
                       CLK_RST_CONTROLLER_CLK_OUT_ENB_H_0, RegData);
    RegData  = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_RST_DEVICES_H_0);
    RegData  = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H,
        SWR_UARTC_RST, ENABLE, RegData);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_RST_DEVICES_H_0, RegData);
    RegData  = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_RST_DEVICES_H_0);
    RegData  = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_H,
        SWR_UARTC_RST, DISABLE, RegData);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_RST_DEVICES_H_0, RegData);

    // Then there is the specific set up for UART itself, including the clock configuration.
    // configure at top for source & configure the divider, internal to uart for obscure reasons
    // when UARTC_DIV_ENB is enable DLL/DLLM programming is not required
    //.Refer to the CLK_SOURCE_UARTD reg description in arclk_rst
    // UARTD_DIV_ENB is disabled as new divisor logic is not enabled on FPGA Bug #739606 
#if 0
    NV_WRITE32(NV_ADDRESS_MAP_CAR_BASE+
        CLK_RST_CONTROLLER_CLK_SOURCE_UARTD_0 ,
        (CLK_RST_CONTROLLER_CLK_SOURCE_UARTD_0_UARTD_CLK_SRC_CLK_M <<
        CLK_RST_CONTROLLER_CLK_SOURCE_UARTD_0_UARTD_CLK_SRC_SHIFT) |
        (CLK_RST_CONTROLLER_CLK_SOURCE_UARTD_0_UARTD_DIV_ENB_DISABLE <<
        CLK_RST_CONTROLLER_CLK_SOURCE_UARTD_0_UARTD_DIV_ENB_SHIFT) |
        0);
#endif

}

void t30_UartD_Init(void)
{
    NvU32 RegData;

    // Initialization is responsible for correct pin mux configuration
    // It also needs to wait for the correct osc frequency to be known
    // IMPORTANT, there is some unspecified logic in the UART that always 
    // operate off PLLP_out3, mail from Robert Quan says that correct operation
    // of UART without starting PLLP requires
    // - to put PLLP in bypass
    // - to override the PLLP_ou3 divider to be 1 (or put in bypass)
    // This is done like that here to avoid any dependence on PLLP analog circuitry 
    // operating correctly

    RegData  = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_PLLP_BASE_0);
    RegData |= NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_BASE, PLLP_BYPASS, ENABLE);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_PLLP_BASE_0, RegData);
    RegData  = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_PLLP_OUTB_0);
    RegData |= NV_DRF_DEF(CLK_RST_CONTROLLER, PLLP_OUTB,
        PLLP_OUT3_OVRRIDE, ENABLE);
    RegData |= NV_DRF_NUM(CLK_RST_CONTROLLER,
        PLLP_OUTB, PLLP_OUT3_RATIO, 0);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_PLLP_OUTB_0, RegData);

    // Set up the pinmuxes to bring UARTD out on ULPI_clk and ULPI_dir
    // for waluigi T30.  All alternate mappings of UARTD are set to select an input other than UARTD.
    //
    //     GMI_AD16 => Alternate 1 ( SPI4 instead of UD3_TXD)
    //     GMI_AD17 => Alternate 1 ( SPI4 instead of UD3_RXD)
    //     ULPI_CLK  =>Alternate 2 (UD3_TXD)
    //     ULPI_DIR  =>Alternate 2 UD3_RXD)
    //
    // Last reviewed on 08/27/2010
    SET_PIN(GMI_A16,PM,SPI4) ;
    SET_PIN(GMI_A17,PM,SPI4) ;
    SET_PIN(ULPI_CLK,PM,UARTD) ;
    SET_PIN(ULPI_DIR,PM,UARTD) ;

    // Enable the pads.
    SET_PIN(ULPI_CLK, TRISTATE, NORMAL);
    SET_PIN(ULPI_DIR, TRISTATE, NORMAL);

    // enable UART D clock, toggle reset  
    RegData = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_CLK_OUT_ENB_U_0);
    RegData |= NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_OUT_ENB_U, 
        CLK_ENB_UARTD, ENABLE);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
                       CLK_RST_CONTROLLER_CLK_OUT_ENB_U_0, RegData);
    RegData  = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_RST_DEVICES_U_0);
    RegData  = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U,
        SWR_UARTD_RST, ENABLE, RegData);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_RST_DEVICES_U_0, RegData);
    RegData  = NV_READ32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_RST_DEVICES_U_0);
    RegData  = NV_FLD_SET_DRF_DEF(CLK_RST_CONTROLLER, RST_DEVICES_U,
        SWR_UARTD_RST, DISABLE, RegData);
    NV_WRITE32(NV_ADDRESS_MAP_PPSB_CLK_RST_BASE +
        CLK_RST_CONTROLLER_RST_DEVICES_U_0, RegData);

    // Then there is the specific set up for UART itself, including the clock configuration.
    // configure at top for source & configure the divider, internal to uart for obscure reasons
    // when UARTD_DIV_ENB is enable DLL/DLLM programming is not required
    //.Refer to the CLK_SOURCE_UARTD reg description in arclk_rst
    // UARTD_DIV_ENB is disabled as new divisor logic is not enabled on FPGA Bug #739606 
#if 0
    NV_WRITE32(NV_ADDRESS_MAP_CAR_BASE+
        CLK_RST_CONTROLLER_CLK_SOURCE_UARTD_0 ,
        (CLK_RST_CONTROLLER_CLK_SOURCE_UARTD_0_UARTD_CLK_SRC_CLK_M <<
        CLK_RST_CONTROLLER_CLK_SOURCE_UARTD_0_UARTD_CLK_SRC_SHIFT) |
        (CLK_RST_CONTROLLER_CLK_SOURCE_UARTD_0_UARTD_DIV_ENB_DISABLE <<
        CLK_RST_CONTROLLER_CLK_SOURCE_UARTD_0_UARTD_DIV_ENB_SHIFT) |
        0);
#endif

}


void t30_Uart_Init(void)
{

  // Enable Divisor Latch access.
  MmioWrite32 (UartBaseAddress + UART_LCR_REG, UART_LCR_DIV_EN_ENABLE);
//return;
  // Programmable divisor
  MmioWrite32 (UartBaseAddress + UART_DLL_REG, (CLK_M / (DEBUG_BAUDRATE * 16))); // low divisor
  MmioWrite32 (UartBaseAddress + UART_DLH_REG,  0);      // high divisor
  // Enter into UART operational mode.
  MmioWrite32 (UartBaseAddress + UART_LCR_REG, UART_LCR_DIV_EN_DISABLE | UART_LCR_CHAR_LENGTH_8);
  // Force DTR and RTS output to active
  MmioWrite32 (UartBaseAddress + UART_MCR_REG, UART_MCR_RTS_FORCE_ACTIVE | UART_MCR_DTR_FORCE_ACTIVE);
  // Clear & enable fifos
  MmioWrite32 (UartBaseAddress + UART_FCR_REG, UART_FCR_TX_FIFO_CLEAR | UART_FCR_RX_FIFO_CLEAR | UART_FCR_FIFO_ENABLE);

}

void t30_UartWriteByte(NvU8 ch)
{
    volatile void* pUart;
/* cardhu */
#if defined(CONFIG_TEGRA3_CARDHU)
    pUart = (void*)s_UartBaseAddress[0];
#endif

/* oregon */
#if defined(CONFIG_TEGRA3_OREGON)
    pUart = (void*)s_UartBaseAddress[2];
#endif

/* waluigi */
#if defined(CONFIG_TEGRA3_WALUIGI)
    pUart = (void*)s_UartBaseAddress[3];
#endif

    aos_UartWriteByteT30(pUart, ch);
}

NvU8 t30_UartReadByte(void)
{
    volatile void* pUart;
/* cardhu */
#if defined(CONFIG_TEGRA3_CARDHU)
    pUart = (void*)s_UartBaseAddress[0];
#endif

/* oregon */
#if defined(CONFIG_TEGRA3_OREGON)
    pUart = (void*)s_UartBaseAddress[2];
#endif

/* waluigi */
#if defined(CONFIG_TEGRA3_WALUIGI)
    pUart = (void*)s_UartBaseAddress[3];
#endif
    return aos_UartReadByteT30(pUart);
}

void NvBlUartInit_t30(void)
{
#if defined(TEGRA3_BOOT_TRACE)

/* cardhu */
#if defined(CONFIG_TEGRA3_CARDHU)
	t30_UartA_Init();
#endif

/* oregon */
#if defined(CONFIG_TEGRA3_OREGON)
	t30_UartC_Init();
#endif

/* waluigi */
#if defined(CONFIG_TEGRA3_WALUIGI)
	t30_UartD_Init();
#endif

	t30_Uart_Init();

	aos_CpuStallUsT30(20000);	// 20 ms

#endif
}

#if defined(TEGRA3_BOOT_TRACE_MORE)
void t30_WriteDebugString(const char* str)
{
    while (str && *str)
    {
        if (*str == '\n') {
            t30_UartWriteByte(0x0D);
        }
        t30_UartWriteByte(*str++);
    }
}
#else 
void t30_WriteDebugString(const char* str)
{
}
#endif

void CpuStallMsT30(NvU32 ms)
{
	aos_CpuStallUsT30((ms * 1000));	
}


void NvBlPrintU32(NvU32 word);
int NvBlUartWrite(const void *ptr);
void NvBlPrintf(const char *format, ...);
void uart_post(char c);

static char s_Hex2Char[] =
{
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F'
};

static void
NvBlPrintU4(NvU8 byte)
{
    uart_post(s_Hex2Char[byte & 0xF]);
}

void
NvBlPrintU8(NvU8 byte)
{
    NvBlPrintU4((byte >> 4) & 0xF);
    NvBlPrintU4((byte >> 0) & 0xF);
}

void
NvBlPrintU32(NvU32 word)
{
    NvBlPrintU8((word >> 24) & 0xFF);
    NvBlPrintU8((word >> 16) & 0xFF);
    NvBlPrintU8((word >>  8) & 0xFF);
    NvBlPrintU8((word >>  0) & 0xFF);
    uart_post(0x20);
}

void
NvBlVprintf(const char *format, va_list ap)
{
    char msg[256];
    sprintf(msg, format, ap);
    NvBlUartWrite(msg);
}

void
NvBlPrintf(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    NvBlVprintf(format, ap);
    va_end(ap);
}

void uart_post(char c)
{
#if defined(TEGRA3_BOOT_TRACE)
    t30_UartWriteByte(c);
#endif
}

void PostCc(char c)
{
    uart_post(0x0d);
    uart_post(0x0a);
    uart_post(c);
    uart_post(c+0x20);

	aos_CpuStallUsT30((2000));	
}

void PostZz(void)
{
    PostCc('Z');
}

void PostYy(void)
{
	CpuStallMsT30(20);
	CpuStallMsT30(20);
	
    PostCc('Y');
}

void PostXx(void)
{
	CpuStallMsT30(20);
    PostCc('X');
    uart_post(0x0d);
    uart_post(0x0a);
	CpuStallMsT30(20);
}

int
NvBlUartWrite(const void *ptr)
{
    const NvU8 *p = ptr;

    while (*p)
    {
        if (*p == '\n') {
            uart_post(0x0D);
        }
        uart_post(*p);
        p++;
    }
    return 0;
}

void debug_trace(int i)
{
    uart_post(i+'a');
    uart_post('.');
    uart_post('.');
}

