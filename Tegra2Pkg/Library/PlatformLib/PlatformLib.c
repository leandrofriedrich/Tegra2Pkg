/** @file
*
*  Copyright (c) 2018, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>

#include <Ppi/ArmMpCoreInfo.h>

ARM_CORE_INFO mTegra2InfoTable[] = {
  {
    // Cluster 0, Core 0
    0x0, 0x0,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (UINT64)0xFFFFFFFF
  },
};

EFI_BOOT_MODE
ArmPlatformGetBootMode (
  VOID
  )
{
  return BOOT_WITH_FULL_CONFIGURATION;
}

/***
  Enable cp10, cp11 and VFP, and the cycle counter
**/
STATIC
VOID
EnableCounters (
  VOID
  )
{
  UINT32 val;
  UINT32 en;

  __asm__ volatile("mrc	p15, 0, %0, c1, c0, 2" : "=r" (val));
  val |= (3<<22)|(3<<20);
  __asm__ volatile("mcr	p15, 0, %0, c1, c0, 2" :: "r" (val));

  ArmInstructionSynchronizationBarrier();
  ArmDataMemoryBarrier();

  __asm__ volatile("mrc  p10, 7, %0, c8, c0, 0" : "=r" (val));
  val |= (1<<30);
  __asm__ volatile("mcr  p10, 7, %0, c8, c0, 0" :: "r" (val));

  __asm__ volatile("mrc	p15, 0, %0, c9, c12, 0" : "=r" (en));
  en &= ~(1<<3);
  en |= 1;
  __asm__ volatile("mcr	p15, 0, %0, c9, c12, 0" :: "r" (en));

  en = (1<<31);
  __asm__ volatile("mcr	p15, 0, %0, c9, c12, 1" :: "r" (en));
}

RETURN_STATUS
ArmPlatformInitialize (
  IN  UINTN                     MpId
  )
{
  EnableCounters();
  return RETURN_SUCCESS;
}

EFI_STATUS
PrePeiCoreGetMpCoreInfo (
  OUT UINTN                   *CoreCount,
  OUT ARM_CORE_INFO           **ArmCoreTable
  )
{
  *CoreCount    = sizeof(mTegra2InfoTable) / sizeof(ARM_CORE_INFO);
  *ArmCoreTable = mTegra2InfoTable;
  return EFI_SUCCESS;
}

EFI_GUID mArmMpCoreInfoPpiGuid = ARM_MP_CORE_INFO_PPI_GUID;
ARM_MP_CORE_INFO_PPI mMpCoreInfoPpi = { PrePeiCoreGetMpCoreInfo };

EFI_PEI_PPI_DESCRIPTOR      gPlatformPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &mArmMpCoreInfoPpiGuid,
    &mMpCoreInfoPpi
  }
};

VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  *PpiListSize = sizeof(gPlatformPpiTable);
  *PpiList = gPlatformPpiTable;
}
