//  Programming tool for the 24Cxx serial EEPROMs using the Winchiphead ch34x IC
//
// (c) December 2011 asbokid <ballymunboy@gmail.com>
// (c) December 2023 aystarik <aystarik@gmail.com>
// (c) February 2026 Mikhail Medvedev <e-ink-reader@yandex.ru>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#ifndef __CH34x_I2C_H__
#define __CH34x_I2C_H__

#pragma once
#include <stdint.h>
#include <libusb.h>

#define ch341_BULK_WRITE_ENDPOINT         0x02        /* bEndpointAddress 0x02  EP 2 OUT (Bulk) */
#define ch341_BULK_READ_ENDPOINT          0x82        /* bEndpointAddress 0x82  EP 2 IN  (Bulk) */
#define ch347_BULK_WRITE_ENDPOINT         0x06        /* bEndpointAddress 0x02  EP 2 OUT (Bulk) */
#define ch347_BULK_READ_ENDPOINT          0x86        /* bEndpointAddress 0x82  EP 2 IN  (Bulk) */
#define DEFAULT_TIMEOUT             3000        // 300mS for USB timeouts

#define ch34x_CMD_I2C_STREAM		0xAA
#define	ch34x_CMD_I2C_STM_MS		0x50
#define	ch34x_CMD_I2C_STM_STA		0x74
#define	ch34x_CMD_I2C_STM_STO		0x75
#define	ch34x_CMD_I2C_STM_OUT		0x80
#define	ch34x_CMD_I2C_STM_IN		0xC0
#define	ch34x_CMD_I2C_STM_SET		0x60
#define	ch34x_CMD_I2C_STM_END		0x00

// Bits 3..1 of the device address byte (1010 A2 A1 A0 R/W) that carry memory address bits
static inline uint8_t i2cMemoryAddrBits(uint8_t algorithm)
{
    if (((algorithm & 0x0f) == 0x0a) || ((algorithm & 0x0f) == 0x0e)) return 0x08; //24LC1025, 24LC515: block select bit B0
    return (uint8_t)(((algorithm & 0xf0) >> 4) << 1) & 0x0e;
}

// chipAddr bits 7..4: device type identifier (0 - memory array 1010, 0x0b - registers / ID page 1011 of M24MxxE-U)
// chipAddr bits 3..1: chip enable bits (E2 E1 E0 pins or C2 C1 of the configurable device address register)
#define I2C_DEVTYPE_MEMORY  0xa0
#define I2C_DEVTYPE_REG     0xb0

// Chip enable bits set by user, without the bits used for memory addressing
static inline uint8_t i2cChipEnableBits(uint8_t algorithm, uint8_t chipAddr)
{
    return (uint8_t)(chipAddr & 0x0e & ~i2cMemoryAddrBits(algorithm));
}

// Device select code without memory address bits and R/W bit
static inline uint8_t i2cDeviceSelect(uint8_t algorithm, uint8_t chipAddr)
{
    uint8_t devType = (chipAddr & 0xf0) ? (uint8_t)(chipAddr & 0xf0) : I2C_DEVTYPE_MEMORY;
    return (uint8_t)(devType | i2cChipEnableBits(algorithm, chipAddr));
}

int ch34xi2cBlockRead(uint8_t * buf, uint32_t address, uint32_t blockSize, uint8_t algorithm, uint8_t progDevice, uint8_t chipAddr);
int ch34xi2cBlockWrite(uint8_t * buf, uint32_t address, uint32_t blockSize, uint32_t sectorSize, uint8_t algorithm, uint8_t progDevice, uint8_t chipAddr);

#endif /* __CH34x_I2C_H__ */
