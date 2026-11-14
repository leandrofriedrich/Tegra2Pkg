/* SPDX-License-Identifier: BSD-2-Clause */
/* SimpleFbDxe: Simple FrameBuffer */

#include <PiDxe.h>
#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FrameBufferBltLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>

#define FB_RED_MASK    0x0000F800
#define FB_GREEN_MASK  0x000007E0
#define FB_BLUE_MASK   0x0000001F

typedef struct {
  VENDOR_DEVICE_PATH       DisplayDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} DISPLAY_DEVICE_PATH;

STATIC DISPLAY_DEVICE_PATH mDisplayDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8),
      }
    },
    EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(sizeof (EFI_DEVICE_PATH_PROTOCOL)),
      0
    }
  }
};

STATIC FRAME_BUFFER_CONFIGURE  *mFrameBufferBltLibConfigure;
STATIC UINTN                    mFrameBufferBltLibConfigureSize;

STATIC
EFI_STATUS
EFIAPI
DisplayQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL        *This,
  IN  UINT32                              ModeNumber,
  OUT UINTN                               *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info
  )
{
  if (ModeNumber != 0) {
    return EFI_INVALID_PARAMETER;
  }

  *Info = AllocateCopyPool (
            sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION),
            This->Mode->Info
            );
  if (*Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
DisplaySetMode (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN UINT32                        ModeNumber
  )
{
  if (ModeNumber != 0) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
DisplayBlt (
  IN     EFI_GRAPHICS_OUTPUT_PROTOCOL      *This,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *BltBuffer,
  IN     EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN     UINTN                             SourceX,
  IN     UINTN                             SourceY,
  IN     UINTN                             DestinationX,
  IN     UINTN                             DestinationY,
  IN     UINTN                             Width,
  IN     UINTN                             Height,
  IN     UINTN                             Delta         OPTIONAL
  )
{
  RETURN_STATUS  Status;
  EFI_TPL        Tpl;

  //
  // Every frame buffer access has to be atomic with respect to the timer
  // driven cursor updates.
  //
  Tpl    = gBS->RaiseTPL (TPL_NOTIFY);
  Status = FrameBufferBlt (
             mFrameBufferBltLibConfigure,
             BltBuffer,
             BltOperation,
             SourceX,
             SourceY,
             DestinationX,
             DestinationY,
             Width,
             Height,
             Delta
             );
  gBS->RestoreTPL (Tpl);

  return EFI_ERROR (Status) ? EFI_INVALID_PARAMETER : EFI_SUCCESS;
}

STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL mDisplay = {
  DisplayQueryMode,
  DisplaySetMode,
  DisplayBlt,
  NULL
};

EFI_STATUS
EFIAPI
SimpleFbDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  DisplayHandle;
  UINT32      FrameBufferAddress;
  UINT32      Width;
  UINT32      Height;
  UINT32      BitsPerPixel;

  FrameBufferAddress = FixedPcdGet32 (PcdFrameBufferAddress);
  Width              = FixedPcdGet32 (PcdFrameBufferWidth);
  Height             = FixedPcdGet32 (PcdFrameBufferHeight);
  BitsPerPixel       = FixedPcdGet32 (PcdFrameBufferBitsPerPixel);

  DEBUG ((
    DEBUG_INFO,
    "SimpleFbDxe: framebuffer 0x%08x %ux%u %u bpp\n",
    FrameBufferAddress,
    Width,
    Height,
    BitsPerPixel
    ));

  if ((FrameBufferAddress == 0) || (Width == 0) || (Height == 0) ||
      ((BitsPerPixel != 16) && (BitsPerPixel != 32)))
  {
    DEBUG ((DEBUG_ERROR, "SimpleFbDxe: invalid framebuffer parameters\n"));
    return EFI_DEVICE_ERROR;
  }

  mDisplay.Mode = AllocateZeroPool (sizeof (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE));
  if (mDisplay.Mode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mDisplay.Mode->Info = AllocateZeroPool (
                          sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION)
                          );
  if (mDisplay.Mode->Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mDisplay.Mode->MaxMode       = 1;
  mDisplay.Mode->Mode          = 0;
  mDisplay.Mode->Info->Version = 0;

  mDisplay.Mode->Info->HorizontalResolution = Width;
  mDisplay.Mode->Info->VerticalResolution   = Height;
  mDisplay.Mode->Info->PixelsPerScanLine    = Width;

  if (BitsPerPixel == 16) {
    mDisplay.Mode->Info->PixelFormat                    = PixelBitMask;
    mDisplay.Mode->Info->PixelInformation.RedMask       = FB_RED_MASK;
    mDisplay.Mode->Info->PixelInformation.GreenMask     = FB_GREEN_MASK;
    mDisplay.Mode->Info->PixelInformation.BlueMask      = FB_BLUE_MASK;
    mDisplay.Mode->Info->PixelInformation.ReservedMask  = 0;
  } else {
    mDisplay.Mode->Info->PixelFormat = PixelBlueGreenRedReserved8BitPerColor;
  }

  mDisplay.Mode->SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  mDisplay.Mode->FrameBufferBase = (EFI_PHYSICAL_ADDRESS)FrameBufferAddress;
  mDisplay.Mode->FrameBufferSize = (UINTN)Width * Height * (BitsPerPixel / 8);

  //
  // Create the FrameBufferBltLib configuration. The first call returns the
  // required size; allocate it and call again.
  //
  mFrameBufferBltLibConfigure     = NULL;
  mFrameBufferBltLibConfigureSize = 0;

  Status = FrameBufferBltConfigure (
             (VOID *)(UINTN)mDisplay.Mode->FrameBufferBase,
             mDisplay.Mode->Info,
             mFrameBufferBltLibConfigure,
             &mFrameBufferBltLibConfigureSize
             );
  if (Status != RETURN_BUFFER_TOO_SMALL) {
    ASSERT_EFI_ERROR (Status);
    return EFI_DEVICE_ERROR;
  }

  mFrameBufferBltLibConfigure = AllocatePool (mFrameBufferBltLibConfigureSize);
  if (mFrameBufferBltLibConfigure == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = FrameBufferBltConfigure (
             (VOID *)(UINTN)mDisplay.Mode->FrameBufferBase,
             mDisplay.Mode->Info,
             mFrameBufferBltLibConfigure,
             &mFrameBufferBltLibConfigureSize
             );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  DisplayHandle = NULL;
  Status        = gBS->InstallMultipleProtocolInterfaces (
                         &DisplayHandle,
                         &gEfiDevicePathProtocolGuid,
                         &mDisplayDevicePath,
                         &gEfiGraphicsOutputProtocolGuid,
                         &mDisplay,
                         NULL
                         );

  ASSERT_EFI_ERROR (Status);
  return Status;
}
