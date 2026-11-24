/** @file
  Minimal Tegra 2 I2C slave controller driver

  Copyright (c) 2026, Leandro Friedrich.
  SPDX-License-Identifier: GPL-2.0-or-later
**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/I2cSlave.h>

#include <Tegra2/Tegra2Hw.h>

// I2C controller registers.
#define I2C_REG_CNFG            0x00
#define I2C_REG_SL_CNFG         0x20
#define I2C_REG_SL_RCVD         0x24
#define I2C_REG_SL_STATUS       0x28
#define I2C_REG_SL_ADDR1        0x2C
#define I2C_REG_SL_ADDR2        0x30
#define I2C_REG_SL_DELAY_COUNT  0x3C

#define I2C_CNFG_NEW_MASTER      BIT11
#define I2C_CNFG_PACKET_MODE     BIT10
#define I2C_CNFG_DEBOUNCE(c)     ((c) << 12)
#define I2C_SL_CNFG_NEWSL        BIT2

#define I2C_SL_DELAY_COUNT_VAL   0x1E

// I2C3 is peripheral 68, i.e. bit 4 of the U clock/reset registers.
#define TEGRA2_CAR_CLK_OUT_ENB_U  (TEGRA2_CAR_BASE + 0x18)
#define TEGRA2_CAR_RST_DEVICES_U  (TEGRA2_CAR_BASE + 0x0C)
#define TEGRA2_CAR_I2C3           BIT4

// I2C3 is on the DTF pin group (108).
#define TEGRA2_PINMUX_I2C3_MUX       (TEGRA2_PINMUX_MUX + 0x18)
#define TEGRA2_PINMUX_I2C3_MUX_MASK  (3U << 30)
#define TEGRA2_PINMUX_I2C3_TRI       (TEGRA2_PINMUX_TRISTATE + 0x0C)
#define TEGRA2_PINMUX_I2C3_TRI_BIT   BIT12

// The EC answers on the 8-bit address 0x8A; SL_ADDR1 takes the 7-bit form.
#define NVEC_EC_ADDRESS  0x45

STATIC
UINT32
EFIAPI
I2cSlaveStatus (
  IN I2C_SLAVE_PROTOCOL  *This
  )
{
  return MmioRead32 (TEGRA2_I2C3_BASE + I2C_REG_SL_STATUS);
}

STATIC
UINT32
EFIAPI
I2cSlaveReadData (
  IN I2C_SLAVE_PROTOCOL  *This
  )
{
  return MmioRead32 (TEGRA2_I2C3_BASE + I2C_REG_SL_RCVD);
}

STATIC
VOID
EFIAPI
I2cSlaveWriteData (
  IN I2C_SLAVE_PROTOCOL  *This,
  IN UINT32                     Value
  )
{
  MmioWrite32 (TEGRA2_I2C3_BASE + I2C_REG_SL_RCVD, Value);
  MmioRead32 (TEGRA2_I2C3_BASE + I2C_REG_SL_RCVD);
}

STATIC I2C_SLAVE_PROTOCOL  mI2cSlaveProtocol = {
  I2cSlaveStatus,
  I2cSlaveReadData,
  I2cSlaveWriteData
};

STATIC
VOID
I2cSlaveInit (
  VOID
  )
{
  UINT32  Value;

  MmioOr32 (TEGRA2_CAR_CLK_OUT_ENB_U, TEGRA2_CAR_I2C3);

  MmioOr32 (TEGRA2_CAR_RST_DEVICES_U, TEGRA2_CAR_I2C3);
  gBS->Stall (2);
  MmioAnd32 (TEGRA2_CAR_RST_DEVICES_U, ~TEGRA2_CAR_I2C3);
  gBS->Stall (1);
  MmioOr32 (TEGRA2_CAR_RST_DEVICES_U, TEGRA2_CAR_I2C3);
  gBS->Stall (2);
  MmioAnd32 (TEGRA2_CAR_RST_DEVICES_U, ~TEGRA2_CAR_I2C3);
  gBS->Stall (1);

  // Route DTF to I2C3 and take the pads out of tristate.
  Value  = MmioRead32 (TEGRA2_PINMUX_I2C3_MUX);
  Value &= ~TEGRA2_PINMUX_I2C3_MUX_MASK;
  MmioWrite32 (TEGRA2_PINMUX_I2C3_MUX, Value);
  MmioAnd32 (TEGRA2_PINMUX_I2C3_TRI, ~TEGRA2_PINMUX_I2C3_TRI_BIT);

  MmioWrite32 (
    TEGRA2_I2C3_BASE + I2C_REG_CNFG,
    I2C_CNFG_NEW_MASTER | I2C_CNFG_PACKET_MODE |
    I2C_CNFG_DEBOUNCE (2)
    );
  MmioWrite32 (TEGRA2_I2C3_BASE + I2C_REG_SL_CNFG, I2C_SL_CNFG_NEWSL);
  MmioWrite32 (TEGRA2_I2C3_BASE + I2C_REG_SL_DELAY_COUNT, I2C_SL_DELAY_COUNT_VAL);
  MmioWrite32 (TEGRA2_I2C3_BASE + I2C_REG_SL_ADDR1, NVEC_EC_ADDRESS);
  MmioWrite32 (TEGRA2_I2C3_BASE + I2C_REG_SL_ADDR2, 0);
}

EFI_STATUS
EFIAPI
I2cInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  (VOID)SystemTable;

  I2cSlaveInit ();

  return gBS->InstallMultipleProtocolInterfaces (
                &ImageHandle,
                &gI2cSlaveProtocolGuid,
                &mI2cSlaveProtocol,
                NULL
                );
}
