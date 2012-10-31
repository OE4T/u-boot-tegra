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

/**
 * nverrval.h is a header used for macro expansion of the errors defined for
 * the Nv methods & interfaces.
 *
 * This header is NOT protected from being included multiple times, as it is
 * used for C pre-processor macro expansion of error codes, and the
 * descriptions of those error codes.
 *
 * Each error code has a unique name, description and value to make it easier
 * for developers to identify the source of a failure.  Thus there are no
 * generic or unknown error codes.
 */

/**
* @defgroup nv_errors NVIDIA Error Codes
*
* Provides return error codes for functions.
*
* @{
*/

/** common error codes */
NVERROR(Success,                            0x00000000, "success")
NVERROR(NotImplemented,                     0x00000001, "method or interface is not implemented")
NVERROR(NotSupported,                       0x00000002, "requested operation is not supported")
NVERROR(NotInitialized,                     0x00000003, "method or interface is not initialized")
NVERROR(BadParameter,                       0x00000004, "bad parameter to method or interface")
NVERROR(Timeout,                            0x00000005, "not completed in the expected time")
NVERROR(InsufficientMemory,                 0x00000006, "insufficient system memory")
NVERROR(ReadOnlyAttribute,                  0x00000007, "cannot write a read-only attribute")
NVERROR(InvalidState,                       0x00000008, "module is in invalid state to perform the requested operation")
NVERROR(InvalidAddress,                     0x00000009, "invalid address")
NVERROR(InvalidSize,                        0x0000000A, "invalid size")
NVERROR(BadValue,                           0x0000000B, "illegal value specified for parameter")
NVERROR(AlreadyAllocated,                   0x0000000D, "resource has already been allocated")
NVERROR(Busy,                               0x0000000E, "busy, try again")
NVERROR(ModuleNotPresent,                   0x000a000E, "hw module is not peresent")
NVERROR(ResourceError,                      0x0000000F, "clock, power, or pinmux resource error")
NVERROR(CountMismatch,                      0x00000010, "Encounter Error on count mismatch")

/* surface specific error codes */
NVERROR(InsufficientVideoMemory,            0x00010000, "insufficient video memory")
NVERROR(BadSurfaceColorScheme,              0x00010001, "this surface scheme is not supported in the current controller")
NVERROR(InvalidSurface,                     0x00010002, "invalid surface")
NVERROR(SurfaceNotSupported,                0x00010003, "surface is not supported")

/* display specific error codes */
NVERROR(DispInitFailed,                     0x00020000, "display initialization failed")
NVERROR(DispAlreadyAttached,                0x00020001, "the display is already attached to a controller")
NVERROR(DispTooManyDisplays,                0x00020002, "the controller has too many displays attached")
NVERROR(DispNoDisplaysAttached,             0x00020003, "the controller does not have an attached display")
NVERROR(DispModeNotSupported,               0x00020004, "the mode is not supported by the display or controller")
NVERROR(DispNotFound,                       0x00020005, "the requested display was not found")
NVERROR(DispAttachDissallowed,              0x00020006, "the display cannot attach to the given controller")
NVERROR(DispTypeNotSupported,               0x00020007, "display type not supported")
NVERROR(DispAuthenticationFailed,           0x00020008, "display authentication failed")
NVERROR(DispNotAttached,                    0x00020009, "display not attached")
NVERROR(DispSamePwrState,                   0x0002000A, "display already in requested power state")
NVERROR(DispEdidFailure,                    0x0002000B, "edid read/parsing failure")

/* I/O devices */
NVERROR(SpiReceiveError,                    0x00040000, "spi receive error" )
NVERROR(SpiTransmitError,                   0x00040001, "spi transmit error" )
NVERROR(HsmmcCardNotPresent,                0x00041000, "hsmmc card not present")
NVERROR(HsmmcControllerBusy,                0x00041001, "hsmmc controller is busy")
NVERROR(HsmmcAutoDetectCard,                0x00041002, "auto detect the card in hsmmc slot")
NVERROR(SdioCardNotPresent,                 0x00042000, "sdio card not present")
NVERROR(SdioInstanceTaken,                  0x00042001, "Instance unavailable or in use")
NVERROR(SdioControllerBusy,                 0x00042002, "controller is busy")
NVERROR(SdioReadFailed,                     0x00042003, "read transaction has failed")
NVERROR(SdioWriteFailed,                    0x00042004, "write transaction has failed")
NVERROR(SdioBadBlockSize,                   0x00042005, "bad block size")
NVERROR(SdioClockNotConfigured,             0x00042006, "Clock is not configured")
NVERROR(SdioSdhcPatternIntegrityFailed,     0x00042007, "SDHC Check pattern integrity failed")
NVERROR(SdioCommandFailed,                  0x00042008, "command failed to execute")
NVERROR(SdioCardAlwaysPresent,              0x00042009, "sdio card is soldered")
NVERROR(SdioAutoDetectCard,                 0x0004200a, "auto detect the sd card")
NVERROR(UsbInvalidEndpoint,                 0x00043000, "usb invalid endpoint")
NVERROR(UsbfTxfrActive,                     0x00043001, "The endpoint has an active transfer in progress.")
NVERROR(UsbfTxfrComplete,                   0x00043002, "The endpoint has a completed transfer that has not been cleared.")
NVERROR(UsbfTxfrFail,                       0x00043003, "The endpoint transfer is failed.")
NVERROR(UsbfEpStalled,                      0x00043004, "The endpoint has been placed in a halted or stalled state.")
NVERROR(UsbfCableDisConnected,              0x00043005, "usb cable disconnected")
NVERROR(UartOverrun,                        0x00044000, "overrun occurred when receiving the data")
NVERROR(UartFraming,                        0x00044001, "data received had framing error")
NVERROR(UartParity,                         0x00044002, "data received had parity error")
NVERROR(UartBreakReceived,                  0x00044004, "received break signal")
NVERROR(I2cReadFailed,                      0x00045000, "Failed to read data through I2C")
NVERROR(I2cWriteFailed,                     0x00045001, "Failed to write data through I2C")
NVERROR(I2cDeviceNotFound,                  0x00045003, "Slave Device Not Found")
NVERROR(I2cInternalError,                   0x00045004, "The controller reports the error during transaction like fifo overrun, underrun")
NVERROR(I2cArbitrationFailed,               0x00045005, "Master does not able to get the control of bus")
NVERROR(IdeHwError,                         0x00046000, "Ide HW error")
NVERROR(IdeReadError,                       0x00046001, "Ide read error")
NVERROR(IdeWriteError,                      0x00046002, "Ide write error")

/* Nand error codes */
NVERROR(NandReadFailed,                     0x000B0000, "Nand Read failed")
NVERROR(NandProgramFailed,                  0x000B0001, "Nand Program failed")
NVERROR(NandEraseFailed,                    0x000B0002, "Nand Erase failed")
NVERROR(NandCopyBackFailed,                 0x000B0003, "Nand Copy back failed")
NVERROR(NandOperationFailed,                0x000B0004, "requested Nand operation failed")
NVERROR(NandBusy,                           0x000B0005, "Nand operation incomplete and is busy")
NVERROR(NandNotOpened,                      0x000B0006, "Nand driver not opened")
NVERROR(NandAlreadyOpened,                  0x000B0007, "Nand driver is already opened")
NVERROR(NandBadOperationRequest,            0x000B0008, "status for wrong nand operation is requested ")
NVERROR(NandCommandQueueError,              0x000B0009, "Command queue error occured ")
NVERROR(NandReadEccFailed,                  0x000B0010, "Read with ECC resulted in uncorrectable errors")
NVERROR(NandFlashNotSupported,              0x000B0011, "Nand flash on board is not supported by the ddk")
NVERROR(NandLockFailed,                     0x000B0012, "Nand flash lock feature failed")
NVERROR(NandErrorThresholdReached,          0x000B0013, "Ecc errors reached the threshold set")
NVERROR(NandWriteFailed,                    0x000B0014, "Nand Write failed")
NVERROR(NandBadBlock,                       0x000B0015, "Indicates a bad block on media")
NVERROR(NandBadState,                       0x000B0016, "Indicates an invalid state")
NVERROR(NandBlockIsLocked,                  0x000B0017, "Indicates the block is locked")
NVERROR(NandNoFreeBlock,                    0x000B0018, "Indicates there is no free block in the flash")
NVERROR(NandTTFailed,                       0x000B0019, "Nand TT Failure")
NVERROR(NandTLFailed,                       0x000B001A, "Nand TL Failure")
NVERROR(NandTLNoBlockAssigned,              0x000B001B, "Nand TL No Block Assigned")

/* Block Driver error codes */
/* generic error codes */
NVERROR(BlockDriverIllegalIoctl,            0x00140001, "Block Driver illegal IOCTL invoked")
NVERROR(BlockDriverOverlappedPartition,     0x00140002, "Block Driver partition overlap")
NVERROR(BlockDriverNoPartition,             0x00140003, "Block Driver IOCTL call needs partition create")
NVERROR(BlockDriverIllegalPartId,           0x00140004, "Block Driver operation using illegal partition ID")
NVERROR(BlockDriverWriteVerifyFailed,       0x00140005, "Block Driver write data comparison failed")
/* Nand specific block driver errors */
NVERROR(NandBlockDriverEraseFailure,        0x00140011, "Nand Block Driver erase failed")
NVERROR(NandBlockDriverWriteFailure,        0x00140012, "Nand Block Driver write failed")
NVERROR(NandBlockDriverReadFailure,         0x00140013, "Nand Block Driver read failed")
NVERROR(NandBlockDriverLockFailure,         0x00140014, "Nand Block Driver lock failed")
NVERROR(NandRegionIllegalAddress,           0x00140015, "Nand Block Driver sector access illegal")
NVERROR(NandRegionTableOpFailure,           0x00140016, "Nand Block Driver region operation failed")
NVERROR(NandBlockDriverMultiInterleave,     0x00140017, "Nand Block Driver multiple interleave modes")
NVERROR(NandTagAreaSearchFailure,           0x0014001c, "Nand Block Driver tag area search failed")
/* EMMC specific block driver errors */
NVERROR(EmmcBlockDriverLockNotSupported,    0x00140101, "EMMC Block Driver Lock operation not supported")
NVERROR(EmmcBlockDriverLockUnaligned,       0x00140102, "EMMC Block Driver Lock area size or location unaligned")
NVERROR(EmmcBlockDriverIllegalStateRead,    0x00140103, "EMMC Block Driver Read when state is not TRANS")
NVERROR(EmmcBlockDriverIllegalStateWrite,   0x00140104, "EMMC Block Driver Write when state is not TRANS")
NVERROR(EmmcCommandFailed,                  0x00140105, "EMMC Block Driver command failed to execute")
NVERROR(EmmcReadFailed,                     0x00140106, "EMMC Block Driver Read failed")
NVERROR(EmmcWriteFailed,                    0x00140107, "EMMC Block Driver Write failed")
NVERROR(EmmcBlockDriverEraseFailure,        0x00140108, "Emmc Block Driver erase failed")
NVERROR(EmmcBlockDriverIllegalAddress,      0x00140109, "Emmc Block Driver address is illegal or misaligned")
NVERROR(EmmcBlockDriverLockFailure,         0x0014010A, "Emmc Block Driver lock failed")
NVERROR(EmmcBlockDriverBlockIsLocked,       0x0014010B, "Emmc Block Driver Accessed block is locked")

/** @} */
/* ^^^ ADD ALL NEW ERRORS RIGHT ABOVE HERE ^^^ */
