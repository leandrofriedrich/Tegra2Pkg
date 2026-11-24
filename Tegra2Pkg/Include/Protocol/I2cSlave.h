/** @file
  Minimal Tegra 2 I2C slave controller protocol

  Copyright (c) 2026, Leandro Friedrich.
  SPDX-License-Identifier: GPL-2.0-or-later
**/

#ifndef __TEGRA2_I2C_SLAVE_H__
#define __TEGRA2_I2C_SLAVE_H__

#include <Uefi.h>

#define I2C_SLAVE_PROTOCOL_GUID \
  { 0x83f0d024, 0xf2a2, 0x4a75, { 0x81, 0xc0, 0x41, 0x64, 0x76, 0x32, 0xe6, 0x0c } }

//
// SL_STATUS bits.
//
#define I2C_SL_RNW        BIT1
#define I2C_SL_RCVD       BIT2
#define I2C_SL_IRQ        BIT3
#define I2C_SL_END_TRANS  BIT4

typedef struct _I2C_SLAVE_PROTOCOL I2C_SLAVE_PROTOCOL;

typedef UINT32 (EFIAPI *I2C_SLAVE_STATUS) (
  IN I2C_SLAVE_PROTOCOL  *This
  );

typedef UINT32 (EFIAPI *I2C_SLAVE_READ_DATA) (
  IN I2C_SLAVE_PROTOCOL  *This
  );

typedef VOID (EFIAPI *I2C_SLAVE_WRITE_DATA) (
  IN I2C_SLAVE_PROTOCOL  *This,
  IN UINT32                     Value
  );

//
// The controller is an I2C slave, so the EC is the bus master. SL_RCVD
// carries the byte received from the master or the byte returned to it,
// depending on the direction of the transfer.
//
struct _I2C_SLAVE_PROTOCOL {
  I2C_SLAVE_STATUS      Status;
  I2C_SLAVE_READ_DATA   ReadData;
  I2C_SLAVE_WRITE_DATA  WriteData;
};

extern EFI_GUID  gI2cSlaveProtocolGuid;

#endif
