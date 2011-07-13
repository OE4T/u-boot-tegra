#ifndef _T30_UART_H
#define _T30_UART_H

#define NV_CAR_REGR(pCar, reg)              NV_READ32( (((NvUPtr)(pCar)) + CLK_RST_CONTROLLER_##reg##_0))
#define NV_CAR_REGW(pCar, reg, val)         NV_WRITE32((((NvUPtr)(pCar)) + CLK_RST_CONTROLLER_##reg##_0), (val))
#define NV_UART_REGR(pBase, reg)                NV_READ32( (((NvUPtr)(pBase)) + UART_##reg##_0))
#define NV_UART_REGW(pBase, reg, val)           NV_WRITE32((((NvUPtr)(pBase)) + UART_##reg##_0), (val))
#define NV_FLOW_REGR(pFlow, reg)        NV_READ32((((NvUPtr)(pFlow)) + FLOW_CTLR_##reg##_0))
#define NV_FLOW_REGW(pFlow, reg, val)   NV_WRITE32((((NvUPtr)(pFlow)) + FLOW_CTLR_##reg##_0), (val))

#define DEBUG_BAUDRATE		57600
#define AOS_PLLP_FIXED_FREQ_KHZ	(216000)

void aos_EnableUartT30(NvU32 port);

//volatile void* aos_UartInitT30(NvU32 portNumber, NvU32 baud_rate);

void NvBlUartInit_t30(void);
void t30_UartWriteByte(NvU8 ch);
NvU8 t30_UartReadByte(void);
void t30_WriteDebugString(const char* str);
void CpuStallMsT30(NvU32 ms);

void uart_post(char c);
void NvBlPrintU32(NvU32);
void NvBlPrintU8(NvU8);
void NvBlPrintf(const char *format, ...);
void PostCc(char c);
void debug_trace(int i);

#define debug_pause()	t30_UartReadByte()

#endif  // _T30_UART_H
