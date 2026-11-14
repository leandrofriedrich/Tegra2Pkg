// SPDX-License-Identifier: BSD-2-Clause
/** @file
 *
 *  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
 *
 *  This program and the accompanying materials
 *  are licensed and made available under the terms and conditions of the BSD
 *License which accompanies this distribution.  The full text of the license may
 *be found at http://opensource.org/licenses/bsd-license.php
 *
 *  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR
 *IMPLIED.
 *
 **/

#include <PiPei.h>

#include <Library/ArmMmuLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

// This varies by device
#include <Device/MemoryMap.h>

VOID BuildMemoryTypeInformationHob(VOID);

STATIC
RETURN_STATUS InitMmu(IN ARM_MEMORY_REGION_DESCRIPTOR *MemoryTable)
{
  VOID         *TranslationTableBase;
  UINTN         TranslationTableSize;
  RETURN_STATUS Status;

  // MMU translation tables are allocated from the UEFI pool.
  Status = ArmConfigureMmu(
      MemoryTable, &TranslationTableBase, &TranslationTableSize);

  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Error: Failed to enable MMU: %r\n", Status));
  }

  return Status;
}

STATIC
VOID AddHob(PARM_MEMORY_REGION_DESCRIPTOR_EX Desc)
{
  BuildResourceDescriptorHob(
      Desc->ResourceType, Desc->ResourceAttribute, Desc->Address, Desc->Length);

  if (Desc->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY ||
      Desc->MemoryType == EfiRuntimeServicesData)
  {
    BuildMemoryAllocationHob(Desc->Address, Desc->Length, Desc->MemoryType);
  }
}

EFI_STATUS
EFIAPI
MemoryPeim(IN EFI_PHYSICAL_ADDRESS UefiMemoryBase, IN UINT64 UefiMemorySize)
{
  PARM_MEMORY_REGION_DESCRIPTOR_EX MemoryDescriptorEx =
      gDeviceMemoryDescriptorEx;
  ARM_MEMORY_REGION_DESCRIPTOR
        MemoryDescriptor[MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT];
  UINTN Index = 0;

  ASSERT(FixedPcdGet32(PcdSystemMemorySize) != 0);

  while (MemoryDescriptorEx->Length != 0) {
    switch (MemoryDescriptorEx->HobOption) {
    case AddMem:
    case AddDev:
    case HobOnlyNoCacheSetting:
      AddHob(MemoryDescriptorEx);
      break;
    case NoHob:
    default:
      goto update;
    }

    if (MemoryDescriptorEx->HobOption == HobOnlyNoCacheSetting) {
      MemoryDescriptorEx++;
      continue;
    }

  update:
    ASSERT(Index < MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT);

    MemoryDescriptor[Index].PhysicalBase = MemoryDescriptorEx->Address;
    MemoryDescriptor[Index].VirtualBase  = MemoryDescriptorEx->Address;
    MemoryDescriptor[Index].Length       = MemoryDescriptorEx->Length;
    MemoryDescriptor[Index].Attributes   = MemoryDescriptorEx->ArmAttributes;

    Index++;
    MemoryDescriptorEx++;
  }

  ASSERT(Index < MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT);
  MemoryDescriptor[Index].PhysicalBase = 0;
  MemoryDescriptor[Index].VirtualBase  = 0;
  MemoryDescriptor[Index].Length       = 0;
  MemoryDescriptor[Index].Attributes   = 0;

  DEBUG((EFI_D_INFO, "Configure MMU In\n"));
  if (EFI_ERROR(InitMmu(MemoryDescriptor))) {
    return EFI_OUT_OF_RESOURCES;
  }
  DEBUG((EFI_D_INFO, "Configure MMU Out\n"));

  if (FeaturePcdGet(PcdPrePiProduceMemoryTypeInformationHob)) {
    BuildMemoryTypeInformationHob();
  }

  return EFI_SUCCESS;
}
