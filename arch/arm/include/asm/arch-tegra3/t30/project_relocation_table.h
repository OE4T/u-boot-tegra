//
// Copyright (c) 2011 NVIDIA Corporation. 
// All rights reserved. 
// 
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met: 
// 
// Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer. 
// 
// Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution. 
// 
// Neither the name of the NVIDIA Corporation nor the names of its contributors 
// may be used to endorse or promote products derived from this software 
// without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE. 
//

#ifndef PROJECT_RELOCATION_TABLE_SPEC_INC
#define PROJECT_RELOCATION_TABLE_SPEC_INC

// ------------------------------------------------------------
//    hw nvdevids
// ------------------------------------------------------------
// Memory Aperture: Internal Memory
#define NV_DEVID_IMEM                            1

// Memory Aperture: External Memory
#define NV_DEVID_EMEM                            2

// Memory Aperture: TCRAM
#define NV_DEVID_TCRAM                           3

// Memory Aperture: IRAM
#define NV_DEVID_IRAM                            4

// Memory Aperture: NOR FLASH
#define NV_DEVID_NOR                             5

// Memory Aperture: EXIO
#define NV_DEVID_EXIO                            6

// Memory Aperture: GART
#define NV_DEVID_GART                            7

// Device Aperture: Graphics Host (HOST1X)
#define NV_DEVID_HOST1X                          8

// Device Aperture: ARM PERIPH registers
#define NV_DEVID_ARM_PERIPH                      9

// Device Aperture: MSELECT
#define NV_DEVID_MSELECT                         10

// Device Aperture: memory controller
#define NV_DEVID_MC                              11

// Device Aperture: external memory controller
#define NV_DEVID_EMC                             12

// Device Aperture: video input
#define NV_DEVID_VI                              13

// Device Aperture: encoder pre-processor
#define NV_DEVID_EPP                             14

// Device Aperture: video encoder
#define NV_DEVID_MPE                             15

// Device Aperture: 3D engine
#define NV_DEVID_GR3D                            16

// Device Aperture: 2D + SBLT engine
#define NV_DEVID_GR2D                            17

// Device Aperture: Image Signal Processor
#define NV_DEVID_ISP                             18

// Device Aperture: DISPLAY
#define NV_DEVID_DISPLAY                         19

// Device Aperture: UPTAG
#define NV_DEVID_UPTAG                           20

// Device Aperture - SHR_SEM
#define NV_DEVID_SHR_SEM                         21

// Device Aperture - ARB_SEM
#define NV_DEVID_ARB_SEM                         22

// Device Aperture - ARB_PRI
#define NV_DEVID_ARB_PRI                         23

// Obsoleted for AP15
#define NV_DEVID_PRI_INTR                        24

// Obsoleted for AP15
#define NV_DEVID_SEC_INTR                        25

// Device Aperture: Timer Programmable
#define NV_DEVID_TMR                             26

// Device Aperture: Clock and Reset
#define NV_DEVID_CAR                             27

// Device Aperture: Flow control
#define NV_DEVID_FLOW                            28

// Device Aperture: Event
#define NV_DEVID_EVENT                           29

// Device Aperture: AHB DMA
#define NV_DEVID_AHB_DMA                         30

// Device Aperture: APB DMA
#define NV_DEVID_APB_DMA                         31

// Obsolete - use AVP_CACHE
#define NV_DEVID_CC                              32

// Device Aperture: COP Cache Controller
#define NV_DEVID_AVP_CACHE                       32

// Device Aperture: SYS_REG
#define NV_DEVID_SYS_REG                         32

// Device Aperture: System Statistic monitor
#define NV_DEVID_STAT                            33

// Device Aperture: GPIO
#define NV_DEVID_GPIO                            34

// Device Aperture: Vector Co-Processor 2
#define NV_DEVID_VCP                             35

// Device Aperture: Arm Vectors
#define NV_DEVID_VECTOR                          36

// Device: MEM
#define NV_DEVID_MEM                             37

// Obsolete - use VDE
#define NV_DEVID_SXE                             38

// Device Aperture: video decoder
#define NV_DEVID_VDE                             38

// Obsolete - use VDE
#define NV_DEVID_BSEV                            39

// Obsolete - use VDE
#define NV_DEVID_MBE                             40

// Obsolete - use VDE
#define NV_DEVID_PPE                             41

// Obsolete - use VDE
#define NV_DEVID_MCE                             42

// Obsolete - use VDE
#define NV_DEVID_TFE                             43

// Obsolete - use VDE
#define NV_DEVID_PPB                             44

// Obsolete - use VDE
#define NV_DEVID_VDMA                            45

// Obsolete - use VDE
#define NV_DEVID_UCQ                             46

// Device Aperture: BSEA (now in AVP cluster)
#define NV_DEVID_BSEA                            47

// Obsolete - use VDE
#define NV_DEVID_FRAMEID                         48

// Device Aperture: Misc regs
#define NV_DEVID_MISC                            49

// Obsolete
#define NV_DEVID_AC97                            50

// Device Aperture: S/P-DIF
#define NV_DEVID_SPDIF                           51

// Device Aperture: I2S
#define NV_DEVID_I2S                             52

// Device Aperture: UART
#define NV_DEVID_UART                            53

// Device Aperture: VFIR
#define NV_DEVID_VFIR                            54

// Device Aperture: NAND Flash Controller
#define NV_DEVID_NANDCTRL                        55

// Obsolete - use NANDCTRL
#define NV_DEVID_NANDFLASH                       55

// Device Aperture: HSMMC
#define NV_DEVID_HSMMC                           56

// Device Aperture: XIO
#define NV_DEVID_XIO                             57

// Device Aperture: PWFM
#define NV_DEVID_PWFM                            58

// Device Aperture: MIPI
#define NV_DEVID_MIPI_HS                         59

// Device Aperture: I2C
#define NV_DEVID_I2C                             60

// Device Aperture: TWC
#define NV_DEVID_TWC                             61

// Device Aperture: SLINK
#define NV_DEVID_SLINK                           62

// Device Aperture: SLINK4B
#define NV_DEVID_SLINK4B                         63

// Obsolete - use DTV
#define NV_DEVID_SPI                             64

// Device Aperture: DVC
#define NV_DEVID_DVC                             65

// Device Aperture: RTC
#define NV_DEVID_RTC                             66

// Device Aperture: KeyBoard Controller
#define NV_DEVID_KBC                             67

// Device Aperture: PMIF
#define NV_DEVID_PMIF                            68

// Device Aperture: FUSE
#define NV_DEVID_FUSE                            69

// Device Aperture: L2 Cache Controller
#define NV_DEVID_CMC                             70

// Device Apertuer: NOR FLASH Controller
#define NV_DEVID_NOR_REG                         71

// Device Aperture: EIDE
#define NV_DEVID_EIDE                            72

// Device Aperture: USB
#define NV_DEVID_USB                             73

// Device Aperture: SDIO
#define NV_DEVID_SDIO                            74

// Device Aperture: TVO
#define NV_DEVID_TVO                             75

// Device Aperture: DSI
#define NV_DEVID_DSI                             76

// Device Aperture: HDMI
#define NV_DEVID_HDMI                            77

// Device Aperture: Third Interrupt Controller extra registers
#define NV_DEVID_TRI_INTR                        78

// Device Aperture: Common Interrupt Controller
#define NV_DEVID_ICTLR                           79

// Non-Aperture Interrupt: DMA TX interrupts
#define NV_DEVID_DMA_TX_INTR                     80

// Non-Aperture Interrupt: DMA RX interrupts
#define NV_DEVID_DMA_RX_INTR                     81

// Non-Aperture Interrupt: SW reserved interrupt
#define NV_DEVID_SW_INTR                         82

// Non-Aperture Interrupt: CPU PMU Interrupt
#define NV_DEVID_CPU_INTR                        83

// Device Aperture: Timer Free Running MicroSecond
#define NV_DEVID_TMRUS                           84

// Device Aperture: Interrupt Controller ARB_GNT Registers
#define NV_DEVID_ICTLR_ARBGNT                    85

// Device Aperture: Interrupt Controller DMA Registers
#define NV_DEVID_ICTLR_DRQ                       86

// Device Aperture: AHB DMA Channel
#define NV_DEVID_AHB_DMA_CH                      87

// Device Aperture: APB DMA Channel
#define NV_DEVID_APB_DMA_CH                      88

// Device Aperture: AHB Arbitration Controller
#define NV_DEVID_AHB_ARBC                        89

// Obsolete - use AHB_ARBC
#define NV_DEVID_AHB_ARB_CTRL                    89

// Device Aperture: AHB/APB Debug Bus Registers
#define NV_DEVID_AHPBDEBUG                       91

// Device Aperture: Secure Boot Register
#define NV_DEVID_SECURE_BOOT                     92

// Device Aperture: SPROM
#define NV_DEVID_SPROM                           93

// Non-Aperture Interrupt: External PMU interrupt
#define NV_DEVID_PMU_EXT                         95

// Device Aperture: AHB EMEM to MC Flush Register
#define NV_DEVID_PPCS                            96

// Device Aperture: MMU TLB registers for COP/AVP
#define NV_DEVID_MMU_TLB                         97

// Device Aperture: OVG engine
#define NV_DEVID_VG                              98

// Device Aperture: CSI
#define NV_DEVID_CSI                             99

// Device ID for COP
#define NV_DEVID_AVP                             100

// Device ID for MPCORE
#define NV_DEVID_CPU                             101

// Device Aperture: ULPI controller
#define NV_DEVID_ULPI                            102

// Device Aperture: ARM CONFIG registers
#define NV_DEVID_ARM_CONFIG                      103

// Device Aperture: ARM PL310 (L2 controller)
#define NV_DEVID_ARM_PL310                       104

// Device Aperture: PCIe
#define NV_DEVID_PCIE                            105

// Device Aperture: OWR (one wire)
#define NV_DEVID_OWR                             106

// Device Aperture: AVPUCQ
#define NV_DEVID_AVPUCQ                          107

// Device Aperture: AVPBSEA (obsolete)
#define NV_DEVID_AVPBSEA                         108

// Device Aperture: Sync NOR
#define NV_DEVID_SNOR                            109

// Device Aperture: SDMMC
#define NV_DEVID_SDMMC                           110

// Device Aperture: KFUSE
#define NV_DEVID_KFUSE                           111

// Device Aperture: CSITE
#define NV_DEVID_CSITE                           112

// Non-Aperture Interrupt: ARM Interprocessor Interrupt
#define NV_DEVID_ARM_IPI                         113

// Device Aperture: ARM Interrupts 0-31
#define NV_DEVID_ARM_ICTLR                       114

// Obsolete: use mod_IOBIST
#define NV_DEVID_IOBIST                          115

// Device Aperture: SPEEDO
#define NV_DEVID_SPEEDO                          116

// Device Aperture: LA
#define NV_DEVID_LA                              117

// Device Aperture: VS
#define NV_DEVID_VS                              118

// Device Aperture: VCI
#define NV_DEVID_VCI                             119

// Device Aperture: APBIF
#define NV_DEVID_APBIF                           120

// Device Aperture: AUDIO
#define NV_DEVID_AUDIO                           121

// Device Aperture: DAM
#define NV_DEVID_DAM                             122

// Device Aperture: TSENSOR
#define NV_DEVID_TSENSOR                         123

// Device Aperture: SE
#define NV_DEVID_SE                              124

// Device Aperture: TZRAM
#define NV_DEVID_TZRAM                           125

// Device Aperture: AUDIO_CLUSTER
#define NV_DEVID_AUDIO_CLUSTER                   126

// Device Aperture: HDA
#define NV_DEVID_HDA                             127

// Device Aperture: SATA
#define NV_DEVID_SATA                            128

// Device Aperture: ATOMICS
#define NV_DEVID_ATOMICS                         129

// Device Aperture: IPATCH
#define NV_DEVID_IPATCH                          130

// Device Aperture: Activity Monitor
#define NV_DEVID_ACTMON                          131

// Device Aperture: Watch Dog Timer
#define NV_DEVID_WDT                             132

// Device Aperture: DTV
#define NV_DEVID_DTV                             133

// Device Aperture: Shared Timer
#define NV_DEVID_TMR_SHARED                      134

// Device Aperture: Consumer Electronics Control
#define NV_DEVID_CEC                             135

// Device Aperture: HSI
#define NV_DEVID_HSI                             136

// Device Aperture: SATA_IOBIST
#define NV_DEVID_SATA_IOBIST                     137

// Device Aperture: HDMI_IOBIST
#define NV_DEVID_HDMI_IOBIST                     138

// Device Aperture: MIPI_IOBIST
#define NV_DEVID_MIPI_IOBIST                     139

// Device Aperture: LPDDR2_IOBIST
#define NV_DEVID_LPDDR2_IOBIST                   140

// Device Aperture: PCIE_X2_0_IOBIST
#define NV_DEVID_PCIE_X2_0_IOBIST                141

// Device Aperture: PCIE_X2_1_IOBIST
#define NV_DEVID_PCIE_X2_1_IOBIST                142

// Device Aperture: PCIE_X4_IOBIST
#define NV_DEVID_PCIE_X4_IOBIST                  143

// Device Aperture: SPEEDO_PMON
#define NV_DEVID_SPEEDO_PMON                     144

// ------------------------------------------------------------
//    hw powergroups
// ------------------------------------------------------------
// Always On
#define NV_POWERGROUP_AO                         0

// Main
#define NV_POWERGROUP_NPG                        1

// CPU related blocks
#define NV_POWERGROUP_CPU                        2

// 3D graphics
#define NV_POWERGROUP_TD                         3

// Video encode engine blocks
#define NV_POWERGROUP_VE                         4

// PCIe
#define NV_POWERGROUP_PCIE                       5

// Video decoder
#define NV_POWERGROUP_VDE                        6

// MPEG encoder
#define NV_POWERGROUP_MPE                        7

// SW define for Power Group maximum
#define NV_POWERGROUP_MAX                        8

// non-mapped power group
#define NV_POWERGROUP_INVALID                    0xffff

// SW table for mapping power group define to register enums (NV_POWERGROUP_INVALID = no mapping)
//  use as 'int table[NV_POWERGROUP_MAX] = { NV_POWERGROUP_ENUM_TABLE }'
#define NV_POWERGROUP_ENUM_TABLE                 NV_POWERGROUP_INVALID, NV_POWERGROUP_INVALID, 0, 1, 2, 3, 4, 6

// ------------------------------------------------------------
//    relocation table data (stored in boot rom)
// ------------------------------------------------------------
// relocation table pointer stored at NV_RELOCATION_TABLE_OFFSET
#define NV_RELOCATION_TABLE_PTR_OFFSET 64
#define NV_RELOCATION_TABLE_SIZE  803
#define NV_RELOCATION_TABLE_INIT \
          0x00000001,\
          0x00711010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00531010, 0x00000000, 0x00000000, \
          0x00691050, 0x00000000, 0x40000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x005f1010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00531010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00531010, 0x00000000, 0x00000000, \
          0x00711010, 0x00000000, 0x00000000, \
          0x00531010, 0x00000000, 0x00000000, \
          0x00521010, 0x00000000, 0x00000000, \
          0x00041110, 0x40000000, 0x00010000, \
          0x00041110, 0x40010000, 0x00010000, \
          0x00041110, 0x40020000, 0x00010000, \
          0x00041110, 0x40030000, 0x00010000, \
          0x00051010, 0x48000000, 0x08000000, \
          0x00081010, 0x50000000, 0x00024000, \
          0x00201010, 0x50040000, 0x00002000, \
          0x00091020, 0x50040000, 0x00002000, \
          0x00721020, 0x50041000, 0x00001000, \
          0x000a1020, 0x50042000, 0x00001000, \
          0x00681020, 0x50043000, 0x00001000, \
          0x000f1240, 0x54040000, 0x00040000, \
          0x000d1140, 0x54080000, 0x00040000, \
          0x00631240, 0x54080000, 0x00040000, \
          0x000e1040, 0x540c0000, 0x00040000, \
          0x00121140, 0x54100000, 0x00040000, \
          0x00111110, 0x54140000, 0x00040000, \
          0x00101330, 0x54180000, 0x00040000, \
          0x00621010, 0x541c0000, 0x00040000, \
          0x00131410, 0x54200000, 0x00040000, \
          0x00131410, 0x54240000, 0x00040000, \
          0x004d1310, 0x54280000, 0x00040000, \
          0x004b1110, 0x542c0000, 0x00040000, \
          0x004c1110, 0x54300000, 0x00040000, \
          0x00761010, 0x54340000, 0x00040000, \
          0x00771010, 0x54380000, 0x00040000, \
          0x004c1110, 0x54400000, 0x00040000, \
          0x00141010, 0x60000000, 0x00001000, \
          0x00151010, 0x60001000, 0x00001000, \
          0x00161010, 0x60002000, 0x00001000, \
          0x00171010, 0x60003000, 0x00001000, \
          0x004f1010, 0x60004000, 0x00000040, \
          0x00551010, 0x60004040, 0x000000c0, \
          0x004f1110, 0x60004100, 0x00000040, \
          0x00561010, 0x60004140, 0x00000008, \
          0x00561110, 0x60004148, 0x00000008, \
          0x004f1210, 0x60004200, 0x00000040, \
          0x004f1310, 0x60004300, 0x00000040, \
          0x004f1410, 0x60004400, 0x00000040, \
          0x001a1010, 0x60005000, 0x00000008, \
          0x001a1010, 0x60005008, 0x00000008, \
          0x00541010, 0x60005010, 0x00000040, \
          0x001a1010, 0x60005050, 0x00000008, \
          0x001a1010, 0x60005058, 0x00000008, \
          0x001a1010, 0x60005060, 0x00000008, \
          0x001a1010, 0x60005068, 0x00000008, \
          0x001a1010, 0x60005070, 0x00000008, \
          0x001a1010, 0x60005078, 0x00000008, \
          0x001a1010, 0x60005080, 0x00000008, \
          0x001a1010, 0x60005088, 0x00000008, \
          0x00841010, 0x60005100, 0x00000020, \
          0x00841010, 0x60005120, 0x00000020, \
          0x00841010, 0x60005140, 0x00000020, \
          0x00841010, 0x60005160, 0x00000020, \
          0x00841010, 0x60005180, 0x00000020, \
          0x00861010, 0x600051a0, 0x00000020, \
          0x001b1210, 0x60006000, 0x00001000, \
          0x001c1010, 0x60007000, 0x00001000, \
          0x001e1110, 0x60008000, 0x00002000, \
          0x00571010, 0x60009000, 0x00000020, \
          0x00571010, 0x60009020, 0x00000020, \
          0x00571010, 0x60009040, 0x00000020, \
          0x00571010, 0x60009060, 0x00000020, \
          0x001f1110, 0x6000a000, 0x00002000, \
          0x00581010, 0x6000b000, 0x00000020, \
          0x00581010, 0x6000b020, 0x00000020, \
          0x00581010, 0x6000b040, 0x00000020, \
          0x00581010, 0x6000b060, 0x00000020, \
          0x00581010, 0x6000b080, 0x00000020, \
          0x00581010, 0x6000b0a0, 0x00000020, \
          0x00581010, 0x6000b0c0, 0x00000020, \
          0x00581010, 0x6000b0e0, 0x00000020, \
          0x00581010, 0x6000b100, 0x00000020, \
          0x00581010, 0x6000b120, 0x00000020, \
          0x00581010, 0x6000b140, 0x00000020, \
          0x00581010, 0x6000b160, 0x00000020, \
          0x00581010, 0x6000b180, 0x00000020, \
          0x00581010, 0x6000b1a0, 0x00000020, \
          0x00581010, 0x6000b1c0, 0x00000020, \
          0x00581010, 0x6000b1e0, 0x00000020, \
          0x00581010, 0x6000b200, 0x00000020, \
          0x00581010, 0x6000b220, 0x00000020, \
          0x00581010, 0x6000b240, 0x00000020, \
          0x00581010, 0x6000b260, 0x00000020, \
          0x00581010, 0x6000b280, 0x00000020, \
          0x00581010, 0x6000b2a0, 0x00000020, \
          0x00581010, 0x6000b2c0, 0x00000020, \
          0x00581010, 0x6000b2e0, 0x00000020, \
          0x00581010, 0x6000b300, 0x00000020, \
          0x00581010, 0x6000b320, 0x00000020, \
          0x00581010, 0x6000b340, 0x00000020, \
          0x00581010, 0x6000b360, 0x00000020, \
          0x00581010, 0x6000b380, 0x00000020, \
          0x00581010, 0x6000b3a0, 0x00000020, \
          0x00581010, 0x6000b3c0, 0x00000020, \
          0x00581010, 0x6000b3e0, 0x00000020, \
          0x00591010, 0x6000c000, 0x00000150, \
          0x00201010, 0x6000c000, 0x00000300, \
          0x005b1010, 0x6000c150, 0x000000b0, \
          0x005c1010, 0x6000c200, 0x00000100, \
          0x00211010, 0x6000c400, 0x00000400, \
          0x00831010, 0x6000c800, 0x00000400, \
          0x00222110, 0x6000d000, 0x00000100, \
          0x00222110, 0x6000d100, 0x00000100, \
          0x00222110, 0x6000d200, 0x00000100, \
          0x00222110, 0x6000d300, 0x00000100, \
          0x00222110, 0x6000d400, 0x00000100, \
          0x00222110, 0x6000d500, 0x00000100, \
          0x00222110, 0x6000d600, 0x00000100, \
          0x00222110, 0x6000d700, 0x00000100, \
          0x00231010, 0x6000e000, 0x00001000, \
          0x00241010, 0x6000f000, 0x00001000, \
          0x006b1010, 0x60010000, 0x00000100, \
          0x002f1210, 0x60011000, 0x00001000, \
          0x00261360, 0x6001a000, 0x00003c00, \
          0x00821010, 0x6001dc00, 0x00000400, \
          0x00061010, 0x68000000, 0x08000000, \
          0x00312110, 0x70000000, 0x00004000, \
          0x00351310, 0x70006000, 0x00000040, \
          0x00351310, 0x70006040, 0x00000040, \
          0x00361010, 0x70006100, 0x00000100, \
          0x00351310, 0x70006200, 0x00000100, \
          0x00351310, 0x70006300, 0x00000100, \
          0x00351310, 0x70006400, 0x00000100, \
          0x008a1010, 0x70006500, 0x00000100, \
          0x008b1010, 0x70006600, 0x00000100, \
          0x008c1010, 0x70006700, 0x00000100, \
          0x008d1010, 0x70006800, 0x00000100, \
          0x008e1010, 0x70006900, 0x00000100, \
          0x008f1010, 0x70006a00, 0x00000100, \
          0x00891010, 0x70006c00, 0x00000200, \
          0x00371310, 0x70008000, 0x00000200, \
          0x00383010, 0x70008500, 0x00000100, \
          0x00391010, 0x70008a00, 0x00000200, \
          0x006d1110, 0x70009000, 0x00001000, \
          0x003a1010, 0x7000a000, 0x00000100, \
          0x00881010, 0x7000b000, 0x00000100, \
          0x003c1310, 0x7000c000, 0x00000100, \
          0x003d1010, 0x7000c100, 0x00000100, \
          0x00851010, 0x7000c300, 0x00000100, \
          0x003c1310, 0x7000c400, 0x00000100, \
          0x003c1310, 0x7000c500, 0x00000100, \
          0x006a1110, 0x7000c600, 0x00000100, \
          0x003c1310, 0x7000c700, 0x00000100, \
          0x003c1310, 0x7000d000, 0x00000100, \
          0x003e1210, 0x7000d400, 0x00000200, \
          0x003e1210, 0x7000d600, 0x00000200, \
          0x003e1210, 0x7000d800, 0x00000200, \
          0x003e1210, 0x7000da00, 0x00000200, \
          0x003e1210, 0x7000dc00, 0x00000200, \
          0x003e1210, 0x7000de00, 0x00000200, \
          0x00421100, 0x7000e000, 0x00000100, \
          0x00431200, 0x7000e200, 0x00000100, \
          0x00441200, 0x7000e400, 0x00000400, \
          0x000b2010, 0x7000f000, 0x00000400, \
          0x000c1310, 0x7000f400, 0x00000400, \
          0x00451110, 0x7000f800, 0x00000400, \
          0x006f1010, 0x7000fc00, 0x00000400, \
          0x00751010, 0x70010000, 0x00002000, \
          0x007c1010, 0x70012000, 0x00002000, \
          0x007b1010, 0x70014000, 0x00001000, \
          0x00871010, 0x70015000, 0x00001000, \
          0x00811010, 0x70016000, 0x00002000, \
          0x00801010, 0x70020000, 0x00010000, \
          0x007f1010, 0x70030000, 0x00010000, \
          0x00701010, 0x70040000, 0x00040000, \
          0x007e1110, 0x70080000, 0x00010000, \
          0x00781110, 0x70080000, 0x00000200, \
          0x00791110, 0x70080200, 0x00000100, \
          0x00342110, 0x70080300, 0x00000100, \
          0x00342110, 0x70080400, 0x00000100, \
          0x00342110, 0x70080500, 0x00000100, \
          0x00342110, 0x70080600, 0x00000100, \
          0x00342110, 0x70080700, 0x00000100, \
          0x007a1110, 0x70080800, 0x00000100, \
          0x007a1110, 0x70080900, 0x00000100, \
          0x007a1110, 0x70080a00, 0x00000100, \
          0x00332010, 0x70080b00, 0x00000100, \
          0x00741010, 0x700c0000, 0x00000100, \
          0x00741010, 0x700c0100, 0x00000100, \
          0x00901010, 0x700c8000, 0x00000200, \
          0x00901010, 0x700c8200, 0x00000200, \
          0x006e1010, 0x78000000, 0x00000200, \
          0x006e1010, 0x78000200, 0x00000200, \
          0x006e1010, 0x78000400, 0x00000200, \
          0x006e1010, 0x78000600, 0x00000200, \
          0x00601010, 0x7c000000, 0x00010000, \
          0x007d1010, 0x7c010000, 0x00010000, \
          0x00492010, 0x7d000000, 0x00004000, \
          0x00492110, 0x7d004000, 0x00004000, \
          0x00492210, 0x7d008000, 0x00004000, \
          0x00020010, 0x80000000, 0x7ff00000, \
          0x00000000, \
          0x81f00008, \
          0x81f0010e, \
          0x81f00206, \
          0x81f0030f, \
          0x81f0040a, \
          0x81f00501, \
          0x81f00607, \
          0x81f00704, \
          0x81f0080b, \
          0x83d00911, \
          0x83c00a02, \
          0x83c00a03, \
          0x83c00a04, \
          0x81f00b0d, \
          0x81f00c00, \
          0x83b00d16, \
          0x81f00e03, \
          0x83d00f10, \
          0x81f01005, \
          0x81f01109, \
          0x81f01202, \
          0x83d01312, \
          0x81f0140c, \
          0x83d01513, \
          0x83b0161f, \
          0xc3b01c00, \
          0xa3b01c01, \
          0xc3b01c02, \
          0xa3b01c03, \
          0x83c01d05, \
          0x83b02204, \
          0x83b02305, \
          0x83b02506, \
          0x83b02607, \
          0x83b02708, \
          0x83b02a09, \
          0x83b02b0a, \
          0x83b02c0b, \
          0x83b02d0c, \
          0xa3603304, \
          0xc3603305, \
          0xc3603306, \
          0xa3603307, \
          0xa360341d, \
          0xc360341c, \
          0x8380391e, \
          0x83803a1f, \
          0x83603e00, \
          0x83603f01, \
          0x83804109, \
          0x8380420a, \
          0x83c04319, \
          0x83d04418, \
          0x83d04519, \
          0x83d0461a, \
          0x83d0471b, \
          0x83d0481c, \
          0x83c04e1a, \
          0x83c04e1b, \
          0x83c04e1c, \
          0xa380500b, \
          0xc380500c, \
          0xa360511b, \
          0xc380511d, \
          0xa360561a, \
          0xc380561c, \
          0x83807b16, \
          0x83807c0d, \
          0x83807d00, \
          0x83807e01, \
          0x83807f02, \
          0x83808003, \
          0x83808117, \
          0x83b08217, \
          0x83b08319, \
          0x83c0841d, \
          0x83608519, \
          0x83608712, \
          0x8360880b, \
          0x83608909, \
          0x8360890a, \
          0x8360890c, \
          0x83608908, \
          0x83608911, \
          0x83b08c10, \
          0x83b08c18, \
          0x83808d04, \
          0x83908d08, \
          0x83a08d08, \
          0x83808e05, \
          0x83908e09, \
          0x83a08e09, \
          0x83808f14, \
          0x83908f11, \
          0x83a08f11, \
          0x8380900e, \
          0x8390900a, \
          0x83a0900a, \
          0x83b0911a, \
          0x8390910e, \
          0x83a0910e, \
          0x83b0921b, \
          0x8390920f, \
          0x83a0920f, \
          0x83609a18, \
          0x83609b16, \
          0x83609c10, \
          0x83c09d00, \
          0x83809f0f, \
          0x8380a006, \
          0x8390a00c, \
          0x83a0a00c, \
          0x8380a108, \
          0x8390a10b, \
          0x83a0a10b, \
          0x8380a207, \
          0x8390a207, \
          0x83a0a207, \
          0x83b0a314, \
          0x8390a30d, \
          0x83a0a30d, \
          0x83b0a41c, \
          0x8390a410, \
          0x83a0a410, \
          0x8360a51e, \
          0x83c0a618, \
          0x8390a612, \
          0x83a0a612, \
          0x8380a715, \
          0x8390a713, \
          0x83a0a713, \
          0x8380a81b, \
          0x83b0a912, \
          0x83b0aa13, \
          0x83b0ab1d, \
          0x83b0ac1e, \
          0x83b0ad0f, \
          0x8360ae02, \
          0x83b0af15, \
          0x83b0b10d, \
          0x83b0b20e, \
          0x8380b61a, \
          0x83c0b706, \
          0x8360b803, \
          0x8360ba17, \
          0x8360ba0d, \
          0x83b0bb11, \
          0x83c0bd07, \
          0x8360cd0e, \
          0x8360ce0f, \
          0x8360cf13, \
          0x8360d01f, \
          0x8360d314, \
          0x8360d415, \
          0x83c0d501, \
          0x00000000
#endif
