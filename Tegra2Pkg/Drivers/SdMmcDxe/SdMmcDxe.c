/** @file
  tegra20 sdmmcdxe

  Copyright (c) 2026, Leandro Friedrich.
  SPDX-License-Identifier: GPL-2.0-or-later
**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/NonDiscoverableDevice.h>
#include <Protocol/SdMmcOverride.h>

#include <Tegra2/Tegra2Hw.h>

#define TEGRA2_SDMMC1_CLK_SRC       (TEGRA2_CAR_BASE + 0x150)
#define TEGRA2_SDMMC_MMIO_SIZE      0x200
#define TEGRA2_PLLP_RATE_HZ         216000000ULL

#define SDHCI_CAP_SLOT_TYPE_MASK      (0x3ULL << 30)
#define SDHCI_CAP_SLOT_TYPE_EMBEDDED  (0x1ULL << 30)
#define SDHCI_CAP_ADMA2               (1ULL << 19)

STATIC
EFI_STATUS
EFIAPI
SdMmcCapability (
  IN      EFI_HANDLE  ControllerHandle,
  IN      UINT8       Slot,
  IN OUT  VOID        *SdMmcHcSlotCapability,
  IN OUT  UINT32      *BaseClkFreq
  );

STATIC EDKII_SD_MMC_OVERRIDE  mSdMmcOverride = {
  EDKII_SD_MMC_OVERRIDE_PROTOCOL_VERSION,
  SdMmcCapability,
  NULL
};

STATIC
EFI_STATUS
EFIAPI
SdMmcCapability (
  IN      EFI_HANDLE  ControllerHandle,
  IN      UINT8       Slot,
  IN OUT  VOID        *SdMmcHcSlotCapability,
  IN OUT  UINT32      *BaseClkFreq
  )
{
  UINT64  Capability;
  UINT32  ClockSource;
  UINT32  Divider;

  (VOID)ControllerHandle;

  if ((SdMmcHcSlotCapability == NULL) || (BaseClkFreq == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Slot != 0) {
    return EFI_NOT_FOUND;
  }

  Capability  = ReadUnaligned64 (SdMmcHcSlotCapability);
  Capability &= ~SDHCI_CAP_SLOT_TYPE_MASK;
  Capability |= SDHCI_CAP_SLOT_TYPE_EMBEDDED;
  Capability &= ~SDHCI_CAP_ADMA2;
  WriteUnaligned64 (SdMmcHcSlotCapability, Capability);

  ClockSource = MmioRead32 (TEGRA2_SDMMC1_CLK_SRC);
  Divider     = ClockSource & 0xFF;
  if ((ClockSource >> 30) == 0) {
    *BaseClkFreq = (UINT32)(((TEGRA2_PLLP_RATE_HZ * 2) / (Divider + 2)) / 1000000);
  } else {
    *BaseClkFreq = (UINT32)(TEGRA2_PLLP_RATE_HZ / 1000000);
  }

  if (*BaseClkFreq == 0) {
    *BaseClkFreq = 1;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SdMmcInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  (VOID)SystemTable;

  Status = gBS->InstallProtocolInterface (&ImageHandle, &gEdkiiSdMmcOverrideProtocolGuid, EFI_NATIVE_INTERFACE, &mSdMmcOverride);

  Status = RegisterNonDiscoverableMmioDevice (NonDiscoverableDeviceTypeSdhci, NonDiscoverableDeviceDmaTypeNonCoherent, NULL, NULL, 1, (UINTN)TEGRA2_SDMMC_SD_BASE, (UINTN)TEGRA2_SDMMC_MMIO_SIZE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcDxe: failed to register SDHCI: %r\n", Status));
    gBS->UninstallProtocolInterface (
           ImageHandle,
           &gEdkiiSdMmcOverrideProtocolGuid,
           &mSdMmcOverride
           );
  }

  return Status;
}
