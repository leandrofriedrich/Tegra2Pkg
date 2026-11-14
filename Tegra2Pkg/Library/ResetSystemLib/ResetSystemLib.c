/** @file
  ResetSystemLib implementation for the Tegra20 PMC.

  Copyright (c) 2026, Leandro Friedrich.
  SPDX-License-Identifier: GPL-2.0-or-later
**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/ResetSystemLib.h>
#include <Tegra2/Tegra2Hw.h>


STATIC
VOID
Tegra2Reset (VOID)
{
  MmioOr32 (TEGRA2_PMC_BASE, BIT4);
  CpuDeadLoop ();
}

VOID
EFIAPI
ResetCold (VOID)
{
  Tegra2Reset ();
}

VOID
EFIAPI
ResetWarm (VOID)
{
  Tegra2Reset ();
}

VOID
EFIAPI
ResetShutdown (VOID)
{
  // no shutdown yet, just reset
  Tegra2Reset ();
}

VOID
EFIAPI
ResetPlatformSpecific (IN UINTN DataSize, IN VOID *ResetData)
{
  Tegra2Reset ();
}

VOID
EFIAPI
ResetSystem (IN EFI_RESET_TYPE ResetType, IN EFI_STATUS ResetStatus, IN UINTN DataSize, IN VOID *ResetData OPTIONAL)
{
  switch (ResetType) {
    case EfiResetCold:
    case EfiResetWarm:
    case EfiResetShutdown:
    case EfiResetPlatformSpecific:
    default:
      Tegra2Reset ();
      break;
  }
}
