// SPDX-License-Identifier: BSD-2-Clause
// Pi.c: Entry point for SEC(Security).

#include "Pi.h"

#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <PiDxe.h>
#include <PiPei.h>

#include <Library/ArmLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/BaseLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/PrePiLib.h>
#include <Library/SerialPortLib.h>
#include <Library/FrameBufferConfigLib.h>

VOID EFIAPI ProcessLibraryConstructorList(VOID);

STATIC VOID UartInit(VOID)
{
  SerialPortInitialize();

  DEBUG((EFI_D_INFO, "\nTianoCore on Tegra 2 (ARM)\n"));
  DEBUG(
      (EFI_D_INFO, "Firmware version %s built %a %a\n\n",
       (CHAR16 *)PcdGetPtr(PcdFirmwareVersionString), __TIME__, __DATE__));
}

VOID Main(IN VOID *StackBase, IN UINTN StackSize)
{
  EFI_HOB_HANDOFF_INFO_TABLE *HobList;
  EFI_STATUS                  Status;
  UINTN                       UefiMemoryBase;
  UINTN                       UefiMemorySize;

  UINTN MemoryBase = 0;
  UINTN MemorySize = 0;

  // Architecture-specific initialization
  // Enable Floating Point
  ArmEnableVFP();

  /* Enable program flow prediction, if supported */
  ArmEnableBranchPrediction();

  // Ensure transfers completed
  ArmInstructionSynchronizationBarrier();
  ArmDataMemoryBarrier();

  // Reconfigure framebuffer
  FrameBufferConfigLibConfigurePixelFormat();

  // Initialize UART.
  UartInit();

  // Declare UEFI region
  MemoryBase     = FixedPcdGet32(PcdSystemMemoryBase);
  MemorySize     = FixedPcdGet32(PcdSystemMemorySize);
  UefiMemoryBase = FixedPcdGet32(PcdUefiMemPoolBase);
  UefiMemorySize = FixedPcdGet32(PcdUefiMemPoolSize);
  StackBase      = (VOID *)(UefiMemoryBase + UefiMemorySize - StackSize);

  DEBUG(
      (EFI_D_INFO | EFI_D_LOAD,
       "UEFI Memory Base = 0x%08x, Size = 0x%08x, Stack Base = 0x%08x, Stack "
       "Size = 0x%08x\n",
       UefiMemoryBase, UefiMemorySize, StackBase, StackSize));

  // Set up HOB
  HobList = HobConstructor(
      (VOID *)UefiMemoryBase, UefiMemorySize, (VOID *)UefiMemoryBase,
      StackBase);

  PrePeiSetHobList(HobList);

  // Invalidate cache
  InvalidateDataCacheRange(
      (VOID *)(UINTN)PcdGet64(PcdFdBaseAddress), PcdGet32(PcdFdSize));

  // Initialize MMU
  Status = MemoryPeim(UefiMemoryBase, UefiMemorySize);

  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Failed to configure MMU\n"));
    CpuDeadLoop();
  }

  DEBUG((EFI_D_LOAD | EFI_D_INFO, "MMU configured from device config\n"));

  // Add HOBs
  BuildStackHob((UINTN)StackBase, StackSize);

  // TODO: Call CpuPei as a library
  BuildCpuHob(ArmGetPhysicalAddressBits(), PcdGet8(PcdPrePiCpuIoSize));

  // Set the Boot Mode
  SetBootMode(BOOT_WITH_FULL_CONFIGURATION);

  // Initialize Platform HOBs (CpuHob and FvHob)
  Status = PlatformPeim();
  ASSERT_EFI_ERROR(Status);

  // SEC phase needs to run library constructors by hand.
  ProcessLibraryConstructorList();

  // Assume the FV that contains the PI (our code) also contains a compressed
  // FV.
  Status = DecompressFirstFv();
  ASSERT_EFI_ERROR(Status);

  // Load the DXE Core and transfer control to it
  Status = LoadDxeCoreFromFv(NULL, 0);
  ASSERT_EFI_ERROR(Status);

  // We should never reach here
  CpuDeadLoop();
}

VOID CEntryPoint(IN VOID *StackBase, IN UINTN StackSize)
{
  // Initialize the platform specific controllers
  ArmPlatformInitialize(ArmReadMpidr());

  // Goto Main
  Main(StackBase, StackSize);
}
