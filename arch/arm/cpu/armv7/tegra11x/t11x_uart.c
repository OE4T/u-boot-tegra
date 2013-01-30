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
#include <asm/arch/t11x/nvcommon.h>
#include <asm/arch/t11x/nverror.h>
#include "t11x.h"
#include "t11x_uart.h"

#include <asm/arch/t11x/arapb_misc.h>
#include <asm/arch/t11x/aruart.h>

#define NV_ASSERT(value) \
{ \
	if ((value) == 0) { \
		t11x_WriteDebugString("t11x_uart:\n"); \
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

static void CpuStallUs(NvU32 MicroSec)
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

static void T11xUartWriteByte(volatile void *pUart, NvU8 ch)
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


static NvU8 T11xUartReadByte(volatile void *pUart)
{
    NvU8 ch;
    NvU32 LineStatusReg;
    // Wait for some time to get the data
    do
    {
        LineStatusReg = NV_UART_REGR(pUart, LSR);
    } while (!(LineStatusReg & NV_DRF_NUM(UART, LSR, RDR, 1)));

    // Read the character.
    ch = NV_UART_REGR(pUart, THR_DLAB_0);
    return ch;
}

/////////////////////////////////////////////////////////
//
// code added below for t11x bringup
//
// special UARTA init before pllp is ready
//
/////////////////////////////////////////////////////////
#define NV_ADDRESS_MAP_PPSB_CLK_RST_BASE 0x60006000
#define NV_ADDRESS_MAP_MISC_BASE 	 0x70000000

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

#if defined(CONFIG_TEGRA11X_DALMORE)
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

void Uart_Init_h(void)
{
    volatile void *pUart;

#if defined(CONFIG_TEGRA11X_DALMORE)
    pUart = (void*)s_UartBaseAddress[3];
#endif
    NvBlUartInitBase((NvU8 *)pUart);
}

void UartA_Init(void)
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
    // All alternate mappings of UARTA are set to select an input other than UARTA.
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

void UartC_Init(void)
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

void UartD_Init(void)
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

    // For Dalmore E1611 w/serial out on UARTD via USB on PM342 board
    //     GMI_AD16 => UARTD	input
    //     GMI_AD17 => UARTD	output
    //     GMI_AD18 => UARTD	output
    //     GMI_AD19 => UARTD	input
    //
    SET_PIN(GMI_A16,PM,UARTD);
    SET_PIN(GMI_A17,PM,UARTD);
    SET_PIN(GMI_A18,PM,UARTD);
    SET_PIN(GMI_A19,PM,UARTD);

    // Enable the pads.
    SET_PIN(GMI_A16, TRISTATE, NORMAL);
    SET_PIN(GMI_A19, TRISTATE, NORMAL);

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

void Uart_Init(void)
{

  // Enable Divisor Latch access.
  MmioWrite32 (UartBaseAddress + UART_LCR_REG, UART_LCR_DIV_EN_ENABLE);

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

void UartWriteByte(NvU8 ch)
{
	volatile void *pUart;

#if defined(CONFIG_TEGRA11X_DALMORE)
	pUart = (void*)s_UartBaseAddress[3];
#endif
	T11xUartWriteByte(pUart, ch);
}

NvU8 UartReadByte(void)
{
	volatile void *pUart;

#if defined(CONFIG_TEGRA11X_DALMORE)
	pUart = (void*)s_UartBaseAddress[3];
#endif
	return T11xUartReadByte(pUart);
}

void NvBlUartInit_t11x(void)
{
#if defined(TEGRA3_BOOT_TRACE)

#if defined(CONFIG_TEGRA11X_DALMORE)
	UartD_Init();
#endif
	Uart_Init();

	CpuStallMs(20);
#endif
}

#if defined(TEGRA3_BOOT_TRACE_MORE)
void t11x_WriteDebugString(const char* str)
{
    while (str && *str)
    {
        if (*str == '\n') {
            UartWriteByte(0x0D);
        }
        UartWriteByte(*str++);
    }
}
#else
void t11x_WriteDebugString(const char* str)
{
}
#endif

void CpuStallMs(NvU32 ms)
{
	CpuStallUs((ms * 1000));
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

static void NvBlPrintU4(NvU8 byte)
{
    uart_post(s_Hex2Char[byte & 0xF]);
}

void NvBlPrintU8(NvU8 byte)
{
    NvBlPrintU4((byte >> 4) & 0xF);
    NvBlPrintU4((byte >> 0) & 0xF);
}

void NvBlPrintU32(NvU32 word)
{
    NvBlPrintU8((word >> 24) & 0xFF);
    NvBlPrintU8((word >> 16) & 0xFF);
    NvBlPrintU8((word >>  8) & 0xFF);
    NvBlPrintU8((word >>  0) & 0xFF);
    uart_post(0x20);
}

void NvBlVprintf(const char *format, va_list ap)
{
    char msg[256];
    sprintf(msg, format, ap);
    NvBlUartWrite(msg);
}

void NvBlPrintf(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    NvBlVprintf(format, ap);
    va_end(ap);
}

void uart_post(char c)
{
#if defined(TEGRA3_BOOT_TRACE)
    UartWriteByte(c);
#endif
}

void PostCc(char c)
{
    uart_post(0x0d);
    uart_post(0x0a);
    uart_post(c);
    uart_post(c+0x20);

	CpuStallUs(2000);
}

void PostZz(void)
{
    PostCc('Z');
}

void PostYy(void)
{
	CpuStallMs(20);
	CpuStallMs(20);
    PostCc('Y');
}

void PostXx(void)
{
	CpuStallMs(20);
    PostCc('X');
    uart_post(0x0d);
    uart_post(0x0a);
	CpuStallMs(20);
}

int NvBlUartWrite(const void *ptr)
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
