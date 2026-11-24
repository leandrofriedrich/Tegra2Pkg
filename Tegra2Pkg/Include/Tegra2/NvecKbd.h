/** @file
  NVEC keyboard device path

  Copyright (c) 2026, Leandro Friedrich.
  SPDX-License-Identifier: GPL-2.0-or-later
**/

#ifndef __NVEC_KBD_H__
#define __NVEC_KBD_H__

#include <Uefi.h>

#include <Protocol/DevicePath.h>

#define NVEC_KBD_DEVICE_PATH_GUID \
  { 0x5e3f9d04, 0x53e5, 0x487d, { 0xaa, 0xe5, 0x9f, 0x5d, 0x1e, 0x3d, 0xd6, 0x78 } }

#pragma pack (1)

typedef struct {
  VENDOR_DEVICE_PATH        Vendor;
  EFI_DEVICE_PATH_PROTOCOL  End;
} NVEC_KBD_DEVICE_PATH;

#pragma pack ()

#endif
