/** @file
  Tegra 2 platform defines for ACPI tables

  Copyright (c) 2026, Leandro Friedrich.
  SPDX-License-Identifier: GPL-2.0-or-later
**/

#ifndef _PLATFORM_TEGRA2_H_
#define _PLATFORM_TEGRA2_H_

#include <IndustryStandard/Acpi50.h>
#include <IndustryStandard/DebugPort2Table.h>
#include <stddef.h>

//
// Platform specific definition
//
#define EFI_ACPI_OEM_TABLE_ID           SIGNATURE_64('O','V','M','F','E','D','K','2') // OEM table id 8 bytes long
#define EFI_ACPI_OEM_REVISION           0x02000820U
#define EFI_ACPI_CREATOR_ID             SIGNATURE_32('M','S','F','T')
#define EFI_ACPI_CREATOR_REVISION       0x00000097U

#define EFI_ACPI_OEM_ID                 {'M','S','F','T',' ',' '} // OEMID 6 bytes
#define EFI_ACPI_VENDOR_ID              SIGNATURE_32('N','V','D','A')
#define EFI_ACPI_DBG2_SIGNATURE         SIGNATURE_32('D','B','G','2')
#define EFI_ACPI_CSRT_REVISION          0x00000000U
#define EFI_ACPI_5_0_CSRT_REVISION      0x00000000U

#define UARTA_BASE                      0x70006000ULL
#define UARTA_LENGTH                    0x40U
#define UARTA_IRQ                       0x44U
#define UARTA_REG_SHIFT                 2U

#define DBG2_PORT_TYPE_SERIAL           EFI_ACPI_DBG2_PORT_TYPE_SERIAL
#define DBG2_SUBTYPE_NVIDIA             EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_NVIDIA_16550_UART
#define DBG2_GAS_ACCESS_DWORD           3U

#define ACPI_HEADER(Signature, Type, Revision) { \
  (Signature), sizeof(Type), (Revision), 0, \
  EFI_ACPI_OEM_ID, EFI_ACPI_OEM_TABLE_ID, \
  EFI_ACPI_OEM_REVISION, EFI_ACPI_CREATOR_ID, \
  EFI_ACPI_CREATOR_REVISION \
}

#pragma pack(push, 1)
//------------------------------------------------------------------------
// CSRT Resource Group header 24 bytes long
//------------------------------------------------------------------------
typedef struct {
  UINT32 Length;
  UINT32 VendorID;
  UINT32 SubVendorId;
  UINT16 DeviceId;
  UINT16 SubdeviceId;
  UINT16 Revision;
  UINT16 Reserved;
  UINT32 SharedInfoLength;
} EFI_ACPI_5_0_CSRT_RESOURCE_GROUP_HEADER;

//------------------------------------------------------------------------
// CSRT table = header + one resource group
//------------------------------------------------------------------------
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER Header;
  EFI_ACPI_5_0_CSRT_RESOURCE_GROUP_HEADER Group;
} EFI_ACPI_5_0_CSRT_TABLE;
#pragma pack(pop)

#endif // !_PLATFORM_TEGRA2_H_
